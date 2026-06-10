#include "skred.h"
#include "skode.h"
#include "seq.h"
#include "miniwav.h"

#include "synth-types.h"
#include "synth.h"
#include "synth-state.h"
#include "synth-config.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

#include <unistd.h>
#include <dirent.h>

#include "ks_chan.h"
#include "kse.h"
#include "recorder.h"

int skode_puts(skode_t *ctx, const char *s) {
  if (!ctx || !s || ctx->log_enable == 0) return 0;
  size_t capacity = sizeof(ctx->log);
  size_t used = strnlen(ctx->log, capacity);
  if (used >= capacity) {
    ctx->log[capacity - 1] = '\0';
    ctx->log_len = (int)(capacity - 1);
    return 0;
  }
  int written = snprintf(ctx->log + used, capacity - used, "%s\n", s);
  if (written > 0) ctx->log_len = (int)strnlen(ctx->log, capacity);
  return 0;
}

int skode_printf(skode_t *ctx, const char *fmt, ...) {
  if (!ctx || !fmt || ctx->log_enable == 0) return 0;
  size_t capacity = sizeof(ctx->log);
  size_t used = strnlen(ctx->log, capacity);
  if (used >= capacity) {
    ctx->log[capacity - 1] = '\0';
    ctx->log_len = (int)(capacity - 1);
    return 0;
  }
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(ctx->log + used, capacity - used, fmt, ap);
  va_end(ap);
  if (len > 0) ctx->log_len = (int)strnlen(ctx->log, capacity);
  return 0;
}

int null_puts(const char *s) { (void)s; return 0; }
int null_printf(const char *fmt, ...) { (void)fmt; return 0; }

#define SAMPLES_TO_MSEC(n) ((double)(n) * (double)1000.0 / (double)MAIN_SAMPLE_RATE)

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
    #include <errno.h>
#endif

void sk_sleep(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
#endif
}

#include <stdio.h>
#include <stdint.h>
#include <math.h>

void voice_show(skode_t *ctx, int v, char c, int verbose) {
  char s[1024];
  char e[8] = "";
  if (c != ' ') sprintf(e, " # *");
  voice_format(v, s, sizeof(s), verbose);
  if (strlen(s)) ctx->printf(ctx, "%s%s\n", s, e);
}

int voice_show_all(skode_t *ctx, int voice, int verbose) {
  for (int i=0; i<synth_config.voice_max; i++) {
    if (sv.user_amp[i] <= SILENT) continue;
    char t = ' ';
    if (i == voice) t = '*';
    voice_show(ctx, i, t, verbose);
  }
  return 0;
}

#define SKODE_CTX_MAX (100)
static skode_t *skode_ctx[SKODE_CTX_MAX];

#define STRING_BUF_LEN (256)
#define STRING_BUF_IDX_MAX (128) // idea one macro per midi key?
static char _skode_extra[STRING_BUF_IDX_MAX][STRING_BUF_LEN];
static char _skode_extra_invalid[STRING_BUF_LEN];
static int skode_extra_valid(int n) { return n >= 0 && n < STRING_BUF_IDX_MAX; }
static char *skode_extra_ptr(int n) {
  if (skode_extra_valid(n)) return _skode_extra[n];
  _skode_extra_invalid[0] = '\0';
  return _skode_extra_invalid;
}

static int skode_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

static int skode_wave_valid(int wave) {
  return wave >= 0 && wave < WAVE_TABLE_MAX;
}

static int skode_double_to_int(double value, int *out) {
  if (!out || !isfinite(value) || value < INT_MIN || value > INT_MAX) return 0;
  *out = (int)value;
  return 1;
}

static int skode_seconds_to_samples(double seconds, uint64_t *out) {
  if (!out || !isfinite(seconds) || seconds < 0.0) return 0;
  long double samples = (long double)seconds * (long double)MAIN_SAMPLE_RATE;
  *out = samples >= (long double)UINT64_MAX ? UINT64_MAX : (uint64_t)samples;
  return 1;
}

static uint64_t skode_u64_add(uint64_t a, uint64_t b) {
  return a > UINT64_MAX - b ? UINT64_MAX : a + b;
}

static void skode_copy_string(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  snprintf(dst, dst_size, "%s", src ? src : "");
}
#define EXTRA_PTR(n) skode_extra_ptr(n)
#define EXTRA_INIT() { _skode_extra_invalid[0] = '\0'; for (int i=0; i<STRING_BUF_IDX_MAX; i++) EXTRA_PTR(i)[0] = '\0';}

int skode_hash(skode_t *ctx) {
  uintptr_t addr = (uintptr_t)ctx;
  addr *= 2654435769u; // knuth's multiplicitive hash (based on golden thingy?)
  return addr % SKODE_CTX_MAX;
}

#define DOT_NUM (3)

void skode_double_dump(skode_t *ctx, double *data, int data_len) {
    int flag = 1;
    int show_dots = 0;
    ctx->printf(ctx, "( ");
    for (int i = 0; i < data_len; i++) {
      if (i < DOT_NUM) {
        show_dots = 0;
        ctx->printf(ctx, "%.8g ", data[i]);
      } else if (i >= (data_len - DOT_NUM)) {
        show_dots = 0;
        ctx->printf(ctx, "%.8g ", data[i]);
      } else {
        show_dots = 1;
      }
      if (flag && show_dots) {
        flag = 0; // only once
        ctx->printf(ctx, " ... ");
      }
    }
    if (data_len <= 5) ctx->printf(ctx, ")\n");
    else ctx->printf(ctx, ") # |%d| %gms\n", data_len, SAMPLES_TO_MSEC(data_len));
}

void skode_show(skode_t *ctx) {
  if (ctx != NULL) {
    ctx->printf(ctx, "# v%d\n", ctx->voice);
    ctx->printf(ctx, "# y%d\n", ctx->pattern);
    ctx->printf(ctx, "# scratch [%s]\n", ands_string(ctx->parse));
    double *data = ands_data(ctx->parse);
    int data_len = ands_data_len(ctx->parse);
    skode_double_dump(ctx, data, data_len);
  }
  for (int i = 0; i < SKODE_CTX_MAX; i++) {
    if (skode_ctx[i]) {
      ctx->printf(ctx, "# ctx[%d] ", skode_hash(skode_ctx[i]));
      ctx->printf(ctx, " v%d", skode_ctx[i]->voice);
      ctx->printf(ctx, " y%d", skode_ctx[i]->pattern);
      ctx->printf(ctx, " x%d", skode_ctx[i]->step);
      ctx->printf(ctx, " .which=%d", skode_ctx[i]->which);
      ctx->printf(ctx, " .ip=%x", skode_ctx[i]->ip);
      ctx->printf(ctx, " .port=%x", skode_ctx[i]->port);
      ctx->printf(ctx, "\n");
    }
  }
}

#include "udp.h"

void system_show(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  ctx->printf(ctx, "# udp_port %d\n", udp_info());
}


static void opcode_arg_show(skode_t *ctx, const opcode_event_t *opcode,
    int n) {
  if (opcode->var_mask & (1U << n)) {
    ctx->printf(ctx, " $%d", (int)opcode->arg[n]);
  } else if (isnan(opcode->arg[n]) &&
      ((uint8_t)opcode->mode & (1U << n))) {
    ctx->printf(ctx, " -");
  } else {
    ctx->printf(ctx, " %g", opcode->arg[n]);
  }
}

static void opcode_show(skode_t *ctx, int index,
    const opcode_event_t *opcode) {
  ctx->printf(ctx, "#   %02d %s", index,
    skode_opcode_name(opcode->code));
  if (opcode->code == SKODE_OP_DELAY)
    ctx->printf(ctx, " %c", opcode->mode);
  for (int i = 0; i < opcode->argc; i++)
    opcode_arg_show(ctx, opcode, i);
  ctx->puts(ctx, "");
}

static int opcode_queue_show_cb(int n, uint64_t timestamp, uint64_t id,
    int tag, const event_t *event, void *user) {
  skode_t *ctx = user;
  uint64_t now = SAMPLE_COUNT_GET();
  double ms = timestamp >= now ?
    (double)(timestamp - now) * 1000.0 / MAIN_SAMPLE_RATE :
    -(double)(now - timestamp) * 1000.0 / MAIN_SAMPLE_RATE;
  ctx->printf(ctx, "# queue %02d id:%" PRIu64 " tag:%d at:%" PRIu64
    " %+.3fms voice:", n, id, tag, timestamp, ms);
  if (event->voice_var)
    ctx->printf(ctx, "$%u", (unsigned)event->voice_var - 1);
  else
    ctx->printf(ctx, "%d", event->voice);
  ctx->printf(ctx, " %s", skode_opcode_name(event->opcode.code));
  for (int i = 0; i < event->opcode.argc; i++)
    opcode_arg_show(ctx, &event->opcode, i);
  ctx->puts(ctx, "");
  return 0;
}

static void opcode_queue_show(skode_t *ctx) {
  ctx->printf(ctx, "# opcode queue size:%d\n", seq_queued());
  seq_foreach(opcode_queue_show_cb, ctx);
}

static void opcode_pattern_step_show(skode_t *ctx, int pattern, int step) {
  const event_program_t *program = &seq_program[pattern][step];
  ctx->printf(ctx, "# pattern:%d step:%d source:[%s]\n",
    pattern, step, seq_pattern[pattern][step]);
  if (program->count == 0) {
    ctx->puts(ctx, "#   (no-op)");
    return;
  }
  for (int i = 0; i < program->count; i++)
    opcode_show(ctx, i, &program->op[i].opcode);
}

static void opcode_pattern_show(skode_t *ctx, int pattern, int step) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) {
    ctx->printf(ctx, "# invalid opcode pattern:%d\n", pattern);
    return;
  }
  if (step >= 0) {
    if (step >= SEQ_STEPS_MAX) {
      ctx->printf(ctx, "# invalid opcode step:%d\n", step);
      return;
    }
    opcode_pattern_step_show(ctx, pattern, step);
    return;
  }
  ctx->printf(ctx, "# opcode pattern:%d length:%d\n",
    pattern, seq_pattern_length[pattern]);
  for (int s = 0; s < seq_pattern_length[pattern]; s++)
    opcode_pattern_step_show(ctx, pattern, s);
}

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <processthreadsapi.h>
#endif

void show_threads(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
#ifdef _WIN32
  DWORD processId = GetCurrentProcessId();
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) {
      return;
  }

  THREADENTRY32 te32;
  te32.dwSize = sizeof(THREADENTRY32);

  if (Thread32First(hSnapshot, &te32)) {
    do {
      if (te32.th32OwnerProcessID == processId) {
        HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te32.th32ThreadID);
        if (hThread) {
          PWSTR threadName = NULL;
          HRESULT hr = GetThreadDescription(hThread, &threadName);
          if (FAILED(hr)) {
            ctx->printf(ctx, "# %lu <GetThreadDescription failed>\n", te32.th32ThreadID);
          } else if (threadName == NULL || wcslen(threadName) == 0) {
            ctx->printf(ctx, "# %lu <unnamed>\n", te32.th32ThreadID);
          } else {
            char narrowName[256];
            WideCharToMultiByte(CP_UTF8, 0, threadName, -1, narrowName, sizeof(narrowName), NULL, NULL);
            ctx->printf(ctx, "# %lu %s\n", te32.th32ThreadID, narrowName);
            LocalFree(threadName);
          }
          CloseHandle(hThread);
        } else {
          ctx->printf(ctx, "# %lu <cannot open thread>\n", te32.th32ThreadID);
        }
      }
    } while (Thread32Next(hSnapshot, &te32));
  }

  CloseHandle(hSnapshot);
#else
#ifndef __APPLE__
  DIR* dir = opendir("/proc/self/task");
  struct dirent* entry;
  if (dir == NULL) {
    perror("# failed to open /proc/self/task");
    return;
  }

  // Iterate through each thread directory
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    char path[4096], name[4096];
    name[0] = '\0';
    snprintf(path, sizeof(path), "/proc/self/task/%s/comm", entry->d_name);
    FILE* f = fopen(path, "r");
    if (f) {
      if (fgets(name, sizeof(name), f)) {
        unsigned long n = strlen(name);
        if (name[n-1] == '\r' || name[n-1] == '\n') {
          name[n-1] = '\0';
        }
      }
      fclose(f);
    }
    ctx->printf(ctx, "# %s %s\n", entry->d_name, name);
  }

  closedir(dir);
#endif
#endif
}

uint64_t skode_ks_submit(skode_t *ctx, int writer, char *cmd, int len) {
  uint64_t seq = kse_submit(writer, cmd, len);
  if (seq) {
    ctx->ks_wait_writer = writer;
    ctx->ks_wait_seq = seq;
  }
  return seq;
}

int skode_ks_wait(skode_t *ctx, int timeout_ms) {
  if (ctx->ks_wait_seq == 0) return 0;
  return kse_wait(ctx->ks_wait_writer, ctx->ks_wait_seq, timeout_ms);
}

int skode_ks_result_to_data(skode_t *ctx, int writer) {
  size_t len = 0;
  uint64_t seq = 0;
  double *f = kse_result_copy(writer, &len, &seq);
  if (f && len) {
    int dlen = ands_data_cap(ctx->parse);
    if ((int)len > dlen) {
      ctx->printf(ctx, "# resize %d -> %d\n", dlen, (int)len);
      ands_data_resize(ctx->parse, (int)len);
    }
    double *g = ands_data(ctx->parse);
    for (int i=0; i<(int)len; i++) g[i] = f[i];
    ands_data_len_set(ctx->parse, (int)len);
  }
  kse_result_free(f);
  return (f && len) ? 1 : 0;
}

void ksynth_loader(skode_t *ctx, FILE *in, int writer, int verbose) {
  if (in) {
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL) {
      size_t len = strlen(line);
      if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
      if (verbose) ctx->printf(ctx, "  %s\n", line);
      skode_ks_submit(ctx, writer, line, (int)len);
    }
    fclose(in);
  } else {
    puts("NNNNOOOO");
  }
}

int ksynth_load_name(skode_t *ctx, int writer, char *file, int verbose) {
  FILE *in = fopen(file, "r");
  int r = 0;
  ksynth_loader(ctx, in, writer, verbose);
  return r;
}

int ksynth_load(skode_t *ctx, int writer, int n, int verbose) {
  char file[1024];
  sprintf(file, "%d.ks", n);
  FILE *in = fopen(file, "r");
  if (in == NULL) {
    sprintf(file, "sk/%d.ks", n);
    in = fopen(file, "r");
  }
  int r = 0;
  ksynth_loader(ctx, in, writer, verbose);
  return r;
}

int skode_load(skode_t *ctx, int voice, int n, int verbose) {
  (void)voice;
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  char file[1024];
  sprintf(file, "%d.sk", n);
  FILE *in = fopen(file, "r");
  if (in == NULL) {
    sprintf(file, "sk/%d.sk", n);
    in = fopen(file, "r");
  }
  int r = 0;
  if (in) {
    static skode_t wprime = SKODE_EMPTY();
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL) {
      size_t len = strlen(line);
      if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
      if (verbose) ctx->printf(ctx, "  %s\n", line);
      r = skode_consume(line, &wprime);
      if (r != 0) {
        ctx->printf(ctx, "# error in patch\n");
        break;
      }
    }
    fclose(in);
  }
  return r;
}

extern synth_sample_t sampling;

int rec_load(skode_t *ctx, int wave_slot, int one_shot, float offset) {
  ctx->printf(ctx, "# rec_load(ctx, %d, %d, %g)\n",
    wave_slot, one_shot, offset);
  if (wave_slot < 0 || wave_slot >= EXT_SAMPLE_999) {
    ctx->printf(ctx, "# invalid slot %d\n", wave_slot);
    return -1;
  }
  if (!sampling.where || sampling.offset < 0 || sampling.trim < 0 ||
      sampling.offset > sampling.len ||
      sampling.trim > sampling.len - sampling.offset) {
    ctx->printf(ctx, "# invalid recording bounds\n");
    return -1;
  }
  float *data = sampling.where + sampling.offset;
  int data_len = sampling.len - sampling.offset - sampling.trim;
  if (data == NULL) {
    ctx->printf(ctx, "# no data\n");
    return 100; // fix todo
  }
  if (data_len <= 0) {
    ctx->printf(ctx, "# no data len\n");
    return 100;
  }
  if (sw.readonly[wave_slot] == 1) {
    ctx->printf(ctx, "# cannot write to w%d r/o\n", wave_slot);
    return -1;
  }
  if (sw.refcount[wave_slot] > 0) {
    ctx->printf(ctx, "# cannot write to w%d ref > 0\n", wave_slot);
    return -1;
  } else {
    wave_free_one(wave_slot);
  }
  float *table = calloc(data_len, sizeof(float));
  if (!table) {
    ctx->printf(ctx, "# allocation failed\n");
    return -1;
  }
  for (int i=0; i<data_len; i++) table[i] = (float)data[i];
  normalize_preserve_zero(table, data_len);
  int len = data_len;
    snprintf(sw.name[wave_slot], WAVE_NAME_MAX, "data[%d]", data_len);
    sw.is_heap[wave_slot] = 1;
    sw.data[wave_slot] = table;
    sw.size[wave_slot] = len;
    sw.rate[wave_slot] = 44100;
    sw.one_shot[wave_slot] = (one_shot != 0);
    sw.loop_enabled[wave_slot] = 0;
    sw.loop_start[wave_slot] = 1;
    sw.loop_end[wave_slot] = len;
    if (offset > 0) {
      sw.offset_hz[wave_slot] = (float)len / 44100 * 440.0f;
      sw.midi_note[wave_slot] = 69;
    } else {
      sw.offset_hz[wave_slot] = 0.0f;
      sw.midi_note[wave_slot] = 0;
    }
    char *name = "data";
    int channels = 1;
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d)\n", data_len, name, wave_slot, channels);
  return 0;
}

int data_load(skode_t *ctx, int wave_slot, int one_shot, float rate, float offset) {
  if (ctx == NULL) return 100; // fix todo
  ctx->printf(ctx, "# data_load(ctx, %d, %d, %g, %g)\n", wave_slot, one_shot, rate, offset);
  if (wave_slot < 0 || wave_slot >= EXT_SAMPLE_999) {
    ctx->printf(ctx, "# invalid slot %d\n", wave_slot);
    return -1;
  }
  double *data = ands_data(ctx->parse);
  int data_len = ands_data_len(ctx->parse);
  if (data == NULL) {
    ctx->printf(ctx, "# no data\n");
    return 100; // fix todo
  }
  if (data_len <= 0) {
    ctx->printf(ctx, "# no data len\n");
    return 100;
  }
  if (sw.readonly[wave_slot] == 1) {
    ctx->printf(ctx, "# cannot write to w%d r/o\n", wave_slot);
    return -1;
  }
  if (!isfinite(rate) || rate <= 0) {
    ctx->printf(ctx, "# invalid rate %g > 0\n", rate);
    return -1;
  }
  if (sw.refcount[wave_slot] > 0) {
    ctx->printf(ctx, "# cannot write to w%d ref > 0\n", wave_slot);
    return -1;
  } else {
    wave_free_one(wave_slot);
  }
  float *table = calloc(data_len, sizeof(float));
  if (!table) {
    ctx->printf(ctx, "# allocation failed\n");
    return -1;
  }
  for (int i=0; i<data_len; i++) table[i] = (float)data[i];
  int len = data_len;
    snprintf(sw.name[wave_slot], WAVE_NAME_MAX, "data[%d]", data_len);
    sw.is_heap[wave_slot] = 1;
    sw.data[wave_slot] = table;
    sw.size[wave_slot] = len;
    sw.rate[wave_slot] = rate;
    sw.one_shot[wave_slot] = (one_shot != 0);
    sw.loop_enabled[wave_slot] = 0;
    sw.loop_start[wave_slot] = 1;
    sw.loop_end[wave_slot] = len;
    if (offset > 0) {
      sw.offset_hz[wave_slot] = (float)len / rate * 440.0f;
      sw.midi_note[wave_slot] = 69;
    } else {
      sw.offset_hz[wave_slot] = 0.0f;
      sw.midi_note[wave_slot] = 0;
    }
    char *name = "data";
    int channels = 1;
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%d)\n", len, name, wave_slot, channels, 44100);
  return 0;
}

int wave_load_string(skode_t *ctx, char *name, int wave_index, int ch, int normalize) {
  (void)normalize;
  if (ctx == NULL) return 100; // fix todo
  if (wave_index < EXT_SAMPLE_000 || wave_index >= EXT_SAMPLE_999) return -1;
  if (sw.readonly[wave_index] == 1) {
    ctx->printf(ctx, "# cannot write to w%d r/o\n", wave_index);
    return -1;
  }
  if (sw.refcount[wave_index] > 0) {
    ctx->printf(ctx, "# cannot write to w%d ref > 0\n", wave_index);
    return -1;
  } else {
    wave_free_one(wave_index);
  }
  FILE *in = fopen(name, "r");
  if (in) fclose(in);
  else {
    ctx->printf(ctx, "# cannot open %s\n", name);
    return -1;
  }
  wav_t wav;
  int len;
  char out[4096];
  float *table = mw_get_str(name, &len, &wav, ch, out, sizeof(out));
  if (table == NULL) {
    ctx->printf(ctx, "# can not read %s\n", name);
    return -1;
  } else {
    skode_copy_string(sw.name[wave_index], WAVE_NAME_MAX, name);
    sw.is_heap[wave_index] = 1;
    sw.data[wave_index] = table;
    sw.size[wave_index] = len;
    sw.rate[wave_index] = (float)wav.SamplesRate;
    sw.one_shot[wave_index] = 1;
    sw.loop_enabled[wave_index] = 0;
    sw.loop_start[wave_index] = 1;
    sw.loop_end[wave_index] = len;
    sw.midi_note[wave_index] = 69;
    sw.offset_hz[wave_index] = (float)len / (float)wav.SamplesRate * 440.0f;
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%d)\n",
      len, name, wave_index, wav.Channels, wav.SamplesRate);
    normalize_preserve_zero(table, len);
  }
  return 0;
}

int wave_try_open_number(int file_num, char *name, int len) {
  FILE *in = NULL;
  snprintf(name, len, "%d.wav", file_num);
  in = fopen(name, "r");
  if (in) { fclose(in); return 0; }
  snprintf(name, len, "%d.mp3", file_num);
  in = fopen(name, "r");
  if (in) { fclose(in); return 0; }
  snprintf(name, len, "wav/%d.wav", file_num);
  in = fopen(name, "r");
  if (in) { fclose(in); return 0; }
  snprintf(name, len, "wav/%d.mp3", file_num);
  in = fopen(name, "r");
  if (in) { fclose(in); return 0; }
  return -1;
}

int wave_load(skode_t *ctx, int file_num, int wave_index, int ch, int normalize) {
  (void)normalize;
  if (ctx == NULL) return 100; // fix todo
  if (wave_index < EXT_SAMPLE_000 || wave_index >= EXT_SAMPLE_999) return -1;
  if (sw.readonly[wave_index] == 1) {
    ctx->printf(ctx, "# cannot write to w%d r/o\n", wave_index);
    return -1;
  }
  if (sw.refcount[wave_index] > 0) {
    ctx->printf(ctx, "# cannot write to w%d ref > 0\n", wave_index);
    return -1;
  } else {
    wave_free_one(wave_index);
  }
  char name[1024];
  if (wave_try_open_number(file_num, name, sizeof(name)) < 0) {
    ctx->printf(ctx, "# cannot open %d.wav or wav/%d.wav\n", file_num, file_num);
    return -1;
  }
  wav_t wav;
  int len;
  char out[4096];
  float *table = mw_get_str(name, &len, &wav, ch, out, sizeof(out));
  if (table == NULL) {
    ctx->printf(ctx, "# can not read %s\n", name);
    return -1;
  } else {
    skode_copy_string(sw.name[wave_index], WAVE_NAME_MAX, name);
    sw.is_heap[wave_index] = 1;
    sw.data[wave_index] = table;
    sw.size[wave_index] = len;
    sw.rate[wave_index] = (float)wav.SamplesRate;
    sw.one_shot[wave_index] = 1;
    sw.loop_enabled[wave_index] = 0;
    sw.loop_start[wave_index] = 1;
    sw.loop_end[wave_index] = len;
    sw.midi_note[wave_index] = 69;
    sw.offset_hz[wave_index] = (float)len / (float)wav.SamplesRate * 440.0f;
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%d)\n",
      len, name, wave_index, wav.Channels, wav.SamplesRate);
    normalize_preserve_zero(table, len);
  }
  return 0;
}


// this is a mess i need to clean up


void pattern_show(skode_t *ctx, int pattern_pointer, int verbose) {
  if (pattern_pointer < 0 || pattern_pointer >= PATTERNS_MAX) return;
  int first = 1;
  for (int s = 0; s < SEQ_STEPS_MAX; s++) {
    char *line = seq_pattern[pattern_pointer][s];
    if (strlen(line) == 0) break;
    if (first) {
      ctx->printf(ctx, "y%d %%%d z%d ym%d",
        pattern_pointer,
        seq_modulo[pattern_pointer],
        seq_state[pattern_pointer],
        seq_mute[pattern_pointer]);
      if (seq_text[pattern_pointer][0] != '\0') ctx->printf(ctx, " [%s] yt", seq_text[pattern_pointer]);
      ctx->printf(ctx, "\n");
      first = 0;
      if (verbose == 0) break;
    }
    ctx->printf(ctx, "[%s] x%d", line, s);
    ctx->puts(ctx, "");
  }
}

void tempo_set(float m);

void downsample_block_average_min_max(
    const float *source, int source_len, float *dest, int dest_len,
    float *min, float *max) {

    if (source_len <= 0 || dest_len <= 0) return;

    // CASE 1: STRETCH (source is smaller than display)
    if (dest_len > source_len) {
        float step = (float)(source_len - 1) / (float)(dest_len - 1);
        for (int i = 0; i < dest_len; i++) {
            int src_idx = (int)(i * step);
            float val = source[src_idx];

            dest[i] = val;
            if (min) min[i] = val;
            if (max) max[i] = val;
        }
        return;
    }

    // CASE 2: DOWNSAMPLE (source is larger than display)
    float block_size = (float)source_len / (float)dest_len;

    for (int i = 0; i < dest_len; i++) {
        int start_idx = (int)(i * block_size);
        int end_idx = (int)((i + 1) * block_size);

        // Ensure we don't go out of bounds
        if (end_idx > source_len) end_idx = source_len;
        if (start_idx >= source_len) start_idx = source_len - 1;

        float sum = 0;
        int count = 0;
        float this_min = source[start_idx];
        float this_max = source[start_idx];

        for (int j = start_idx; j < end_idx; j++) {
            float val = source[j];
            sum += val;
            count++;
            if (val < this_min) this_min = val;
            if (val > this_max) this_max = val; // Fixed: was this_min
        }

        if (min) min[i] = this_min;
        if (max) max[i] = this_max;
        dest[i] = (count > 0) ? sum / (float)count : 0;
    }
}

void downsample_block_average(const float *source, int source_len, float *dest, int dest_len) {
  downsample_block_average_min_max(source, source_len, dest, dest_len, NULL, NULL);
}

#if 0
void scope_wave_update(const float *table, int size) {
  new_scope->wave_len = 0;
  downsample_block_average_min_max(table, size, new_scope->wave_data, SCOPE_WAVE_WIDTH, new_scope->wave_min, new_scope->wave_max);
  new_scope->wave_len = SCOPE_WAVE_WIDTH;
}
#endif


#include <stdio.h>

#include <stdio.h>
#include <math.h>

#define WAVE_DISPLAY_DEFAULT_WIDTH 60
#define WAVE_DISPLAY_DEFAULT_HEIGHT 12
#define WAVE_DISPLAY_MIN_WIDTH 8
#define WAVE_DISPLAY_MAX_WIDTH 160
#define WAVE_DISPLAY_MIN_HEIGHT 2
#define WAVE_DISPLAY_MAX_HEIGHT 40
#define WAVE_PEAK_ACCENT_MIN_DELTA 2

typedef struct {
    float min;
    float max;
    float peak;
    float dc;
    float rms;
    int zero_crossings;
    int clipped;
} wave_stats_t;

static int wave_display_dim(double value, int fallback, int min, int max) {
    if (!isfinite(value) || value <= 0.0) return fallback;
    if (value >= max) return max;
    int n = (int)value;
    if (n < min) return min;
    if (n > max) return max;
    return n;
}

static wave_stats_t wave_stats(float *data, int n) {
    wave_stats_t s = {0};
    if (!data || n <= 0) return s;

    s.min = data[0];
    s.max = data[0];
    double sum = 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        float v = data[i];
        float av = fabsf(v);
        if (v < s.min) s.min = v;
        if (v > s.max) s.max = v;
        if (av > s.peak) s.peak = av;
        if (av >= 0.999f) s.clipped++;
        if (i > 0 && ((data[i - 1] < 0.0f && v >= 0.0f) || (data[i - 1] > 0.0f && v <= 0.0f))) {
            s.zero_crossings++;
        }
        sum += v;
        sum_sq += (double)v * (double)v;
    }
    s.dc = (float)(sum / (double)n);
    s.rms = (float)sqrt(sum_sq / (double)n);
    return s;
}

static void print_wave_stats(skode_t *ctx, const char *label, float *data, int n, float rate) {
    wave_stats_t s = wave_stats(data, n);
    float ms = (rate > 0.0f) ? ((float)n / rate * 1000.0f) : 0.0f;
    ctx->printf(ctx,
        "# %s |%d| (%gms) min %+0.3f max %+0.3f peak %0.3f rms %0.3f dc %+0.4f zc %d",
        label, n, ms, s.min, s.max, s.peak, s.rms, s.dc, s.zero_crossings);
    if (s.clipped) ctx->printf(ctx, " clip %d", s.clipped);
    ctx->puts(ctx, "");
}

/**
 * Prints a connected audio waveform using Braille patterns.
 * Draws a single connected trace.
 */
void print_audio_braille_connected(skode_t *ctx, float *data, int n, int width_chars, int height_chars) {
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    int total_dots_y = height_chars * 4;
    int total_dots_x = width_chars * 2;
    int zero_y = (total_dots_y - 1) / 2;

    const int masks[2][4] = {
        {0x40, 0x04, 0x02, 0x01},
        {0x80, 0x20, 0x10, 0x08}
    };

    int *y_coords = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_min = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_max = (int *)malloc(total_dots_x * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }

    for (int x = 0; x < total_dots_x; x++) {
        float val = data[0];
        if (n > 1 && total_dots_x > 1) {
            float data_pos = (float)x * (n - 1) / (total_dots_x - 1);
            int idx = (int)data_pos;
            float fract = data_pos - idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx+1] * fract;
        }
        y_coords[x] = (int)((val / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));

        int start = (int)((long long)x * n / total_dots_x);
        int end = (int)((long long)(x + 1) * n / total_dots_x);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = (int)((bucket_min / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
        y_peak_max[x] = (int)((bucket_max / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
    }

    for (int r = height_chars - 1; r >= 0; r--) {
        ctx->printf(ctx, ":");
        for (int c = 0; c < width_chars; c++) {
            int pattern = 0;

            for (int dx = 0; dx < 2; dx++) {
                int x = c * 2 + dx;
                int y_curr = y_coords[x];
                int y_prev = (x > 0) ? y_coords[x - 1] : y_curr;
                int lo = (y_curr < y_prev) ? y_curr : y_prev;
                int hi = (y_curr > y_prev) ? y_curr : y_prev;

                for (int y = lo; y <= hi; y++) {
                    if (y / 4 == r) {
                        pattern |= masks[dx][y % 4];
                    }
                }
                if (zero_y / 4 == r && pattern == 0) {
                    pattern |= masks[dx][zero_y % 4];
                }
                if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_min[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_min[x] % 4];
                }
                if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_max[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_max[x] % 4];
                }
            }
            ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | (pattern >> 6), 0x80 | (pattern & 0x3F));
        }
        ctx->printf(ctx, ":\n");
    }
    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void print_audio_braille_labeled(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim) {
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    int total_dots_y = height_chars * 4;
    int total_dots_x = width_chars * 2;
    int zero_y = (total_dots_y - 1) / 2;

    const int masks[2][4] = {
        {0x40, 0x04, 0x02, 0x01},
        {0x80, 0x20, 0x10, 0x08}
    };

    // Pre-calculate Y positions
    int *y_coords = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_min = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_max = (int *)malloc(total_dots_x * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }
    for (int x = 0; x < total_dots_x; x++) {
        float val = data[0];
        if (n > 1 && total_dots_x > 1) {
            float data_pos = (float)x * (n - 1) / (total_dots_x - 1);
            int idx = (int)data_pos;
            float fract = data_pos - idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx+1] * fract;
        }

        y_coords[x] = (int)((val / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));

        int start = (int)((long long)x * n / total_dots_x);
        int end = (int)((long long)(x + 1) * n / total_dots_x);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = (int)((bucket_min / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
        y_peak_max[x] = (int)((bucket_max / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
    }

    // 1. Draw the Waveform rows
    for (int r = height_chars - 1; r >= 0; r--) {
        ctx->printf(ctx, ":");
        for (int c = 0; c < width_chars; c++) {
            int pattern = 0;
            for (int dx = 0; dx < 2; dx++) {
                int x = c * 2 + dx;
                int y_curr = y_coords[x];
                int y_prev = (x > 0) ? y_coords[x-1] : y_curr;
                int y_min = (y_curr < y_prev) ? y_curr : y_prev;
                int y_max = (y_curr > y_prev) ? y_curr : y_prev;

                for (int y = y_min; y <= y_max; y++) {
                    if (y / 4 == r) pattern |= masks[dx][y % 4];
                }
                if (zero_y / 4 == r && pattern == 0) {
                    pattern |= masks[dx][zero_y % 4];
                }
                if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_min[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_min[x] % 4];
                }
                if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_max[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_max[x] % 4];
                }
            }
            ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | (pattern >> 6), 0x80 | (pattern & 0x3F));
        }
        ctx->printf(ctx, ":\n");
    }

    // 2. Draw the Marker row (Offset and Trim)
    // Map indices to dot positions
    int dot_offset = 0;
    int dot_trim = total_dots_x - 1;
    if (n > 1) {
        dot_offset = (int)((float)offset * (total_dots_x - 1) / (n - 1));
        dot_trim   = (int)((float)trim   * (total_dots_x - 1) / (n - 1));
    }

    ctx->printf(ctx, "^");
    for (int c = 0; c < width_chars; c++) {
        int pattern = 0;
        for (int dx = 0; dx < 2; dx++) {
            int x = c * 2 + dx;
            // Use Dot 1 (0x01) and Dot 4 (0x08) as markers on the top of the char
            if (x >= dot_offset && x <= dot_trim) {
                pattern |= (dx == 0) ? 0x01 : 0x08;
            }
        }
        ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | (pattern >> 6), 0x80 | (pattern & 0x3F));
    }
    ctx->printf(ctx, "^\n");

    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

int wavetable_show(skode_t *ctx, int n) {
  if (n >= 0 && n < WAVE_TABLE_MAX && sw.data[n] && sw.size[n]) {
    int readonly = sw.readonly[n];
    int refcount = sw.refcount[n];
    int size = sw.size[n];
    wave_stats_t stats = wave_stats(sw.data[n], size);
    ctx->printf(ctx, "# w%-3d %d samples @", n, size);
    ctx->printf(ctx, " %gHz (+%gHz) MIDI %g",
      sw.rate[n],
      sw.offset_hz[n],
      sw.midi_note[n]);
    if (readonly) ctx->printf(ctx, " R/O");
    else ctx->printf(ctx, " R/W");
    ctx->printf(ctx, " ref#%d", refcount);
    ctx->printf(ctx, " [%s]", sw.name[n]);
    ctx->puts(ctx, "");
    ctx->printf(ctx,
      "# min %+0.3f max %+0.3f peak %0.3f rms %0.3f dc %+0.4f zc %d",
      stats.min, stats.max, stats.peak, stats.rms, stats.dc, stats.zero_crossings);
    if (stats.clipped) ctx->printf(ctx, " clip %d", stats.clipped);
    ctx->puts(ctx, "");
  }
  return 0;
}

void wave_table_dynamic_expand(int n) {
  float fbig = 0.0;
  float fsmall = 0.0;
  int len = sw.size[n];
  float *samples = sw.data[n];
  if (len <= 0 || samples == NULL) {
    return;
  }
  for (int i = 0; i < len; i++) {
    float g = samples[i];
    if (g > fbig) fbig = g;
    if (g < fsmall) fsmall = g;
  }

  // use the min/max to make a scale factor that keeps 0
  // in the same relative place

  float scale;
  if (fabsf(fsmall) > fabsf(fbig)) {
    scale = -1.0f / fsmall;
  } else {
    scale = 1.0f / fbig;
  }

  // Convert scaled float samples to 16-bit PCM

  for (int i = 0; i < len; i++) {
    float g = samples[i];
    g *= scale;
    if (g > 1.0f) g = 1.0f;
    if (g < -1.0f) g = -1.0f;
    samples[i] = g;
  }
}

#include <sys/time.h>
#include <unistd.h>

void skode_sample_alloc(int frames) {
  if (frames <= 0 || frames > INT_MAX - 4096) {
    sampling.frames = 0;
    return;
  }
  if (sampling.capacity < frames) {
    float *where = (float *)calloc((size_t)frames + 4096, sizeof(float));
    if (!where) {
      sampling.frames = 0;
      return;
    }
    free(sampling.where);
    sampling.where = where;
    sampling.capacity = frames;
  }
  sampling.frames = frames;
}

void skode_sample_go(int frames) {
  if (sampling.busy) {
    return;
  }
  skode_sample_alloc(frames);
  if (sampling.frames > 0 && sampling.where) sampling.go = 1;
}

#include "miniaudio.h"


















void normalize_buffer(float* pSamples, ma_uint32 frameCount, ma_uint32 channels) {
    float maxAmp = 0.0f;
    ma_uint32 sampleCount = frameCount * channels;

    // Find the absolute peak
    for (ma_uint32 i = 0; i < sampleCount; ++i) {
        float absValue = fabsf(pSamples[i]);
        if (absValue > maxAmp) maxAmp = absValue;
    }

    // Only scale if the peak exceeds our target (e.g., 0.95 for headroom)
    if (maxAmp > 0.0f) {
        float scale = 0.95f / maxAmp;
        for (ma_uint32 i = 0; i < sampleCount; ++i) {
            pSamples[i] *= scale;
        }
    }
}

void record_find_trim(int argc, float arg0, float arg1) {
  if (!sampling.where || sampling.len <= 0 ||
      sampling.len > sampling.capacity) {
    return;
  }
  int lead0 = -1;
  int trail0 = -1;
  float zero = 0.0;
  float threshold = 0.0;
  float tleft = 0.0;
  float tright = 0.0;
  if (argc) {
    threshold = fabs(arg0);
    tleft = threshold;
    tright = threshold;
    if (argc > 1) tright = arg1;
  }
  #define INSIDE(v,t,r) (fabs((v)-(t))<=(r))

  // 1. Find the first audible sample index
  int first_audible = -1;
  for (int i = 0; i < sampling.len; i++) {
    if (!INSIDE(sampling.where[i], zero, tleft)) {
      first_audible = i;
      break;
    }
  }

  // Look backward into the silence to find the closest zero-crossing point
  if (first_audible > 0) {
    lead0 = first_audible;
    while (lead0 > 0) {
      // Check for an exact zero or a sign change between adjacent samples
      if (sampling.where[lead0] == 0.0 || (sampling.where[lead0] * sampling.where[lead0 - 1] <= 0.0)) {
        break;
      }
      lead0--;
    }
  } else if (first_audible == 0) {
    lead0 = 0; // Starts immediately with audio
  }

  // 2. Find the last audible sample index
  int last_audible = -1;
  for (int i = sampling.len - 1; i >= 0; i--) {
    if (!INSIDE(sampling.where[i], zero, tright)) {
      last_audible = i;
      break;
    }
  }

  // Look forward into the trailing silence to find the closest zero-crossing point
  if (last_audible >= 0 && last_audible < sampling.len - 1) {
    int end_idx = last_audible;
    while (end_idx < sampling.len - 1) {
      // Check for an exact zero or a sign change between adjacent samples
      if (sampling.where[end_idx] == 0.0 || (sampling.where[end_idx] * sampling.where[end_idx + 1] <= 0.0)) {
        break;
      }
      end_idx++;
    }
    // Calculate total trailing samples to remove from the tail end
    trail0 = sampling.len - 1 - end_idx;
  }

  if (lead0 > 0) sampling.offset = lead0;
  if (trail0 > 0) sampling.trim = trail0;
}

void skode_envelope_velocity(int voice, float x, uint64_t now) {
  if (voice < 0 || voice >= synth_config.voice_max) return;
  uint64_t t = sv.link_trig_samp[voice];
  if (t > 0) {
    uint64_t qt = t > UINT64_MAX - now ? UINT64_MAX : t + now;
    event_t event = {0};
    event.voice = voice;
    event.opcode.code = SKODE_OP_ENVELOPE_VELOCITY;
    event.opcode.argc = 1;
    event.opcode.arg[0] = x;
    queue_event(qt, &event, 0);
  } else {
    envelope_velocity(voice, x);
  }
}

static int skode_compile_scheduled(skode_t *ctx, const char *text,
    event_program_t *program) {
  skode_compile_result_t result = skode_compile_program(text, program);
  if (result == SKODE_COMPILE_OK) return 1;
  ctx->printf(ctx, "# command is not schedulable (%d)\n", result);
  return 0;
}

int skode_opcode_supported(skode_opcode_t opcode) {
  switch (opcode) {
    case SKODE_OP_VOICE:
    case SKODE_OP_AMP:
    case SKODE_OP_FREQ:
    case SKODE_OP_MIDI_NOTE:
    case SKODE_OP_PAN:
    case SKODE_OP_VELOCITY:
    case SKODE_OP_ENVELOPE_VELOCITY:
    case SKODE_OP_WAVE_DIRECTION:
    case SKODE_OP_WAVE_LOOP:
    case SKODE_OP_LINK_MIDI:
    case SKODE_OP_LINK_VELOCITY:
    case SKODE_OP_TRIGGER_DELAY:
    case SKODE_OP_MUTE:
    case SKODE_OP_MIDI_DETUNE:
    case SKODE_OP_VOICE_RESET:
    case SKODE_OP_TRIGGER:
    case SKODE_OP_WAVE:
    case SKODE_OP_VOICE_COPY:
    case SKODE_OP_WAVE_DEFAULT:
    case SKODE_OP_VARIABLE_SET:
      return 1;
    case SKODE_OP_AMP_MOD: return 1;
    case SKODE_OP_PHASE_DISTORTION:
    case SKODE_OP_PHASE_MOD:
      return 1;
    case SKODE_OP_FILTER_ENVELOPE:
    case SKODE_OP_FILTER_ENVELOPE_DEPTH:
      return 1;
    case SKODE_OP_FREQ_MOD:
    case SKODE_OP_FREQ_MOD_MODE:
      return 1;
    case SKODE_OP_GLISSANDO: return 1;
    case SKODE_OP_SAMPLE_HOLD: return 1;
    case SKODE_OP_FILTER_MODE:
    case SKODE_OP_FILTER_FREQ:
    case SKODE_OP_FILTER_RESONANCE:
      return 1;
    case SKODE_OP_ENVELOPE_MODE:
    case SKODE_OP_ENVELOPE:
      return 1;
    case SKODE_OP_PAN_MOD: return 1;
    case SKODE_OP_QUANTIZE: return 1;
    case SKODE_OP_RECORD_TRACK: return 1;
    case SKODE_OP_SMOOTHER: return 1;
    case SKODE_OP_NONE:
    case SKODE_OP_DELAY:
    default:
      return 0;
  }
}

static int skode_opcode_int(const opcode_event_t *opcode, int n, int *value) {
  return opcode && n >= 0 && n < opcode->argc &&
    skode_double_to_int(opcode->arg[n], value);
}

static void skode_opcode_links(const opcode_event_t *opcode,
    float *link0, float *link1, float *link2, float *link3) {
  int links[4] = {-1, -1, -1, -1};
  for (int i = 0; i < opcode->argc && i < 4; i++) {
    int link;
    if (skode_opcode_int(opcode, i, &link) && skode_voice_valid(link))
      links[i] = link;
  }
  *link0 = links[0];
  *link1 = links[1];
  *link2 = links[2];
  *link3 = links[3];
}

int skode_execute_voice_opcode(const opcode_event_t *opcode, int voice) {
  if (!opcode || !skode_voice_valid(voice) ||
      opcode->argc > SEQ_OPCODE_ARG_MAX || opcode->var_mask != 0) return -1;
  uint8_t default_mask =
    opcode->code == SKODE_OP_MIDI_NOTE ||
    opcode->code == SKODE_OP_MIDI_DETUNE ? 1U : 0U;
  if (((uint8_t)opcode->mode & ~default_mask) != 0) return -1;
  for (int i = 0; i < opcode->argc; i++) {
    if (!isfinite(opcode->arg[i]) &&
        !(isnan(opcode->arg[i]) && (default_mask & (1U << i)))) {
      return -1;
    }
  }
  int x = 0;
  int x_valid = skode_opcode_int(opcode, 0, &x);
  switch ((skode_opcode_t)opcode->code) {
    case SKODE_OP_AMP:
      return opcode->argc == 1 ? amp_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_AMP_MOD:
      if (opcode->argc < 2) return amp_mod_set(voice, -1, 0, 0);
      return x_valid ? amp_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_WAVE_DIRECTION:
      return opcode->argc == 0 ? wave_dir(voice, -1) :
        (x_valid ? wave_dir(voice, x) : -1);
    case SKODE_OP_WAVE_LOOP:
      return opcode->argc == 0 ? wave_loop(voice, -1) :
        (x_valid ? wave_loop(voice, x) : -1);
    case SKODE_OP_PHASE_DISTORTION:
      if (opcode->argc == 0) return cz_set(voice, 0, .5f);
      if (!x_valid) return -1;
      return cz_set(voice, x,
        opcode->argc > 1 ? opcode->arg[1] : .5f);
    case SKODE_OP_PHASE_MOD:
      if (opcode->argc < 2) return cmod_set(voice, -1, 0);
      return x_valid ? cmod_set(voice, x, opcode->arg[1]) : -1;
    case SKODE_OP_FREQ:
      return opcode->argc == 1 ? freq_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_FILTER_ENVELOPE:
      if (opcode->argc != 4) return -1;
      envelope_init_e(&sv.filter_envelope[voice], opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]);
      sv.use_filter_envelope[voice] = !(opcode->arg[0] == 0 &&
        opcode->arg[1] == 0 && opcode->arg[2] == 1 &&
        opcode->arg[3] == 0);
      return 0;
    case SKODE_OP_FILTER_ENVELOPE_DEPTH:
      if (opcode->argc != 1) return -1;
      sv.filter_env_depth[voice] = opcode->arg[0];
      return 0;
    case SKODE_OP_FREQ_MOD:
      if (opcode->argc <= 1) return freq_mod_set(voice, -1, 0, 0);
      return x_valid ? freq_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_FREQ_MOD_MODE:
      if (!x_valid) return -1;
      sv.freq_mod_mode[voice] = x;
      return 0;
    case SKODE_OP_GLISSANDO:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.glissando_enable[voice] = 0;
        sv.glissando_time[voice] = 0;
      } else {
        sv.glissando_enable[voice] = 1;
        sv.glissando_time[voice] = opcode->arg[0];
      }
      return 0;
    case SKODE_OP_LINK_MIDI:
      if (opcode->argc < 1) return -1;
      skode_opcode_links(opcode, &sv.link_midi_0[voice],
        &sv.link_midi_1[voice], &sv.link_midi_2[voice],
        &sv.link_midi_3[voice]);
      return 0;
    case SKODE_OP_SAMPLE_HOLD:
      if (!x_valid) return -1;
      sv.sample_hold_max[voice] = x;
      return 0;
    case SKODE_OP_LINK_VELOCITY:
      if (opcode->argc < 1) return -1;
      skode_opcode_links(opcode, &sv.link_velo_0[voice],
        &sv.link_velo_1[voice], &sv.link_velo_2[voice],
        &sv.link_velo_3[voice]);
      return 0;
    case SKODE_OP_TRIGGER_DELAY:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.link_trig[voice] = -1;
        sv.link_trig_samp[voice] = 0;
      } else {
        long double samples =
          (long double)opcode->arg[0] * MAIN_SAMPLE_RATE;
        sv.link_trig[voice] = opcode->arg[0];
        sv.link_trig_samp[voice] = samples >= (long double)UINT64_MAX ?
          UINT64_MAX : (uint64_t)samples;
      }
      return 0;
    case SKODE_OP_FILTER_MODE:
      if (!x_valid) return -1;
      sv.filter_mode[voice] = x;
      mmf_set_params(voice, sv.filter_freq[voice], sv.filter_res[voice]);
      return 0;
    case SKODE_OP_FILTER_FREQ:
      return opcode->argc == 1 ? mmf_set_freq(voice, opcode->arg[0]) : -1;
    case SKODE_OP_ENVELOPE_MODE:
      if (!x_valid) return -1;
      sv.amp_envelope_mode[voice] = x;
      return 0;
    case SKODE_OP_ENVELOPE_VELOCITY:
      return opcode->argc == 1 ?
        envelope_velocity(voice, opcode->arg[0]) : -1;
    case SKODE_OP_VELOCITY:
      if (opcode->argc != 1) return -1;
      {
        uint64_t now = SAMPLE_COUNT_GET();
        skode_envelope_velocity(voice, opcode->arg[0], now);
        if (sv.link_velo_0[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_0[voice], opcode->arg[0], now);
        if (sv.link_velo_1[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_1[voice], opcode->arg[0], now);
        if (sv.link_velo_2[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_2[voice], opcode->arg[0], now);
        if (sv.link_velo_3[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_3[voice], opcode->arg[0], now);
        return 0;
      }
    case SKODE_OP_MUTE:
      return x_valid ? wave_mute(voice, x) : -1;
    case SKODE_OP_MIDI_NOTE:
      if (opcode->argc != 1 && opcode->argc != 2) return -1;
      return skode_midi_note(voice, opcode->arg[0],
        opcode->argc == 2 ? opcode->arg[1] : 0);
    case SKODE_OP_MIDI_DETUNE:
      if (opcode->argc < 1 || opcode->argc > 2) return -1;
      if (!isnan(opcode->arg[0]))
        sv.midi_transpose[voice] = opcode->arg[0];
      if (opcode->argc > 1) sv.midi_cents[voice] = opcode->arg[1];
      return 0;
    case SKODE_OP_PAN:
      return opcode->argc == 1 ? pan_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_PAN_MOD:
      if (opcode->argc < 2) return pan_mod_set(voice, -1, 0, 0);
      return x_valid ? pan_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_QUANTIZE:
      return x_valid ? wave_quant(voice, x) : -1;
    case SKODE_OP_FILTER_RESONANCE:
      return opcode->argc == 1 ? mmf_set_res(voice, opcode->arg[0]) : -1;
    case SKODE_OP_RECORD_TRACK:
      return x_valid ? synth_record_track_set(voice, x) : -1;
    case SKODE_OP_SMOOTHER:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.smoother_enable[voice] = 0;
      } else {
        sv.smoother_enable[voice] = 1;
        sv.smoother_smoothing[voice] = opcode->arg[0];
      }
      return 0;
    case SKODE_OP_VOICE_RESET:
      return x_valid ? wave_reset(x) : -1;
    case SKODE_OP_ENVELOPE:
      return opcode->argc == 4 ? envelope_set(voice, opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]) : -1;
    case SKODE_OP_TRIGGER:
      if (opcode->argc != 0) return -1;
      envelope_velocity(voice, 1);
      if (sv.link_velo_0[voice] >= 0) envelope_velocity(sv.link_velo_0[voice], 1);
      if (sv.link_velo_1[voice] >= 0) envelope_velocity(sv.link_velo_1[voice], 1);
      if (sv.link_velo_2[voice] >= 0) envelope_velocity(sv.link_velo_2[voice], 1);
      if (sv.link_velo_3[voice] >= 0) envelope_velocity(sv.link_velo_3[voice], 1);
      return 0;
    case SKODE_OP_WAVE:
      if (!x_valid || wave_set(voice, x) != 0) return -1;
      if (opcode->argc > 1) {
        int value;
        if (!skode_opcode_int(opcode, 1, &value)) return -1;
        sv.interpolate[voice] = value != 0;
      }
      if (opcode->argc > 2) {
        int value;
        if (!skode_opcode_int(opcode, 2, &value)) return -1;
        sv.one_shot[voice] = value != 0;
      }
      return 0;
    case SKODE_OP_VOICE_COPY:
      return x_valid && skode_voice_valid(x) ? voice_copy(voice, x) : -1;
    case SKODE_OP_WAVE_DEFAULT:
      return opcode->argc == 0 ? wave_default(voice) : -1;
    case SKODE_OP_VARIABLE_SET:
      if (opcode->argc != 2 || !x_valid ||
          x < 0 || x >= ANDS_VAR_MAX) return -1;
      global_var[x] = opcode->arg[1];
      return 0;
    case SKODE_OP_NONE:
    case SKODE_OP_DELAY:
    case SKODE_OP_VOICE:
    default:
      return -1;
  }
}

int skode_function(ands_t *s, int info) {
  uint32_t atom = ands_atom_num(s);
  int argc = ands_arg_len(s);
  skode_t *ctx = (skode_t*)ands_user(s);
  double *arg = ands_arg(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  if (ctx->trace) {
    ctx->printf(ctx, "# SKODE_FUNCTION ");
    ctx->printf(ctx, "%s", ands_atom_string(s));
    if (argc) {
      for (int i=0; i<argc; i++) ctx->printf(ctx, " %g", arg[i]);
    }
    ctx->puts(ctx, "");
  }
  switch (atom) {
    case ATOM4('wait'): // blocking msec wait
      if (x_valid && x >= 0) sk_sleep(x);
      break;
    case ATOM4('a---'): // amp loudness
      if (argc) amp_set(voice, arg[0]);
      break;
    case ATOM4('A---'): // AM voice depth
      if (argc < 2) {
        amp_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        amp_mod_set(voice, x, arg[1], a);
      }
      break;
    case ATOM4('b---'): // wave-direction bool
      if (argc == 0) { wave_dir(voice, -1); } else { wave_dir(voice, x); } break;
    case ATOM4('B---'): // wave-loop bool
      if (argc == 0) { wave_loop(voice, -1); } else { wave_loop(voice, x); } break;
    case ATOM4('c---'): // phase-distortion algo distortion
      if (argc == 0) {
        cz_set(voice, 0, .5);
      } else if (argc == 1) {
        cz_set(voice, x, .5);
      } else {
        cz_set(voice, x, arg[1]);
      }
      break;
    case ATOM4('C---'): // PD-mod voice depth
      if (argc < 2) {
        cmod_set(voice, -1, 0);
      } else if (x_valid) {
        cmod_set(voice, x, arg[1]);
      }
      break;
    case ATOM4('D---'): // data-size
      if (argc) {
        if (x > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, x);
      } else {
        ctx->printf(ctx, "# D[%d]\n", ands_data_cap(ctx->parse));
      }
      break;
    case ATOM4('?d--'): // show-skode-data (summary)
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        skode_double_dump(ctx, data, data_len);
      }
      break;
    case ATOM4('f---'): // freq hz
      if (argc) freq_set(voice, arg[0]);
      break;
    case ATOM4('ft--'): // filter-adsr A D S R
      if (argc == 4) {
        float a = arg[0];
        float d = arg[1];
        float s = arg[2];
        float r = arg[3];
        envelope_init_e(&sv.filter_envelope[voice], a, d, s, r);
        sv.use_filter_envelope[voice] = !(a==0 && d==0 && s==1 && r==0);
      }
      break;
    case ATOM4('fd--'): // filter-adsr depth
      if (argc) sv.filter_env_depth[voice] = arg[0];
      break;
    case ATOM4('F---'): // FM voice depth
      if (argc <= 1) {
        freq_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        freq_mod_set(voice, x, arg[1], a);
      }
      break;
    case ATOM4('FF--'): // FM mode
      if (argc) sv.freq_mod_mode[voice] = x;
      break;
    case ATOM4('g---'): // glissando speed
      if (argc) {
        if (arg[0] <= 0) {
          sv.glissando_enable[voice] = 0;
          sv.glissando_time[voice] = 0.0;
        } else {
          sv.glissando_enable[voice] = 1;
          sv.glissando_time[voice] = arg[0];
        }
      }
      break;
    case ATOM4('G---'): // link-midi voice [voice]
      if (argc) {
        int links[4] = {-1, -1, -1, -1};
        for (int i = 0; i < argc && i < 4; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_midi_0[voice] = links[0];
        sv.link_midi_1[voice] = links[1];
        sv.link_midi_2[voice] = links[2];
        sv.link_midi_3[voice] = links[3];
      }
      break;
    case ATOM4('h---'): // sample-hold phase-count
      if (argc) { sv.sample_hold_max[voice] = x; } break;
    case ATOM4('H---'): // link-velo voice [voice [voice [voice]]]
      if (argc) {
        int links[4] = {-1, -1, -1, -1};
        for (int i = 0; i < argc && i < 4; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_velo_0[voice] = links[0];
        sv.link_velo_1[voice] = links[1];
        sv.link_velo_2[voice] = links[2];
        sv.link_velo_3[voice] = links[3];
      }
      break;
    // TODO re-allocate the data/array buffer with the arg
    case ATOM4('/D--'): // resize-data count
      if (argc) {
        // free and re-allocate...
        if (x > 0) ands_data_resize(ctx->parse, x);
      }
      ctx->printf(ctx, "# /D data %p cap %d |%d|\n",
        ands_data(ctx->parse),
        ands_data_cap(ctx->parse),
        ands_data_len(ctx->parse));
      break;
    case ATOM4('I---'): // log-event bool
      if (argc) {} break; // TODO en/dis-able send timestamp wire to the event logger
    case ATOM4('L---'): // link-trigger-delay seconds
      if (argc) {
        double seconds = arg[0];
        if (!isfinite(seconds) || seconds <= 0.0) {
          sv.link_trig[voice] = -1.0f;
          sv.link_trig_samp[voice] = 0;
        } else {
          long double samples = (long double)seconds * (long double)MAIN_SAMPLE_RATE;
          sv.link_trig[voice] = (float)seconds;
          sv.link_trig_samp[voice] =
            samples >= (long double)UINT64_MAX ? UINT64_MAX : (uint64_t)samples;
        }
      }
      break;
    case ATOM4('J---'): // filter-mode selector
      if (argc) {
        sv.filter_mode[voice] = x;
        mmf_set_params(voice,
          sv.filter_freq[voice],
          sv.filter_res[voice]);
      }
      break;
    case ATOM4('K---'): // filter-cutoff freq
      if (argc) { mmf_set_freq(voice, arg[0]); }
      break;
    #define TMP_WRITER 0
    case ATOM4('/ks-'): // ksynth-load num (verbose)
      {
        char *file = ands_string(ctx->parse);
        int verbose = 0;
        if (argc) skode_double_to_int(arg[0], &verbose);
        if (strlen(file)) {
          ksynth_load_name(ctx, TMP_WRITER, file, verbose);
        }
      }
      break;
    case ATOM4('/k--'): // ksynth-load num (verbose)
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        ksynth_load(ctx, TMP_WRITER, x, verbose);
      }
      break;
    case ATOM4('ks--'): // run ksynth code in string buffer
    case ATOM4('k!--'): // run ksynth code in string buffer
      {
        int len = 0;
        char *cmd = ands_string(ctx->parse);
        if (cmd) len = strlen(cmd);
        if (ctx->trace) {
          ctx->printf(ctx, "cmd:[%s] len:%d\n", cmd, len);
        }
        if (len) skode_ks_submit(ctx, 0, cmd, len);
      }
      break;
    case ATOM4('kw--'): // wait for last ksynth request [timeout-ms]
      {
        int timeout_ms = -1;
        if (argc) timeout_ms = x;
        if (!skode_ks_wait(ctx, timeout_ms)) ctx->printf(ctx, "# kw timeout\n");
      }
      break;
    case ATOM4('kw>-'): // wait for last ksynth request, then copy result to data [timeout-ms]
      {
        int timeout_ms = -1;
        if (argc) timeout_ms = x;
        if (skode_ks_wait(ctx, timeout_ms)) {
          skode_ks_result_to_data(ctx, ctx->ks_wait_writer);
        } else {
          ctx->printf(ctx, "# kw> timeout\n");
        }
      }
      break;
    case ATOM4('k?--'): // k show last results
      {
        size_t len;
        int writer = 0;
        double *f = kse_result_copy(writer, &len, NULL);
        if (len && f) skode_double_dump(ctx, f, len);
        kse_result_free(f);
      }
      break;
    case ATOM4('k>d-'): // k results to d?
      {
        int writer = 0;
        skode_ks_result_to_data(ctx, writer);
      }
      break;
    case ATOM4('k---'): // adsr-mode bool
      if (argc) { sv.amp_envelope_mode[voice] = x; } break;
    case ATOM4('udp-'): // show-udp
      if (argc) {
        ctx->printf(ctx, "# udp [%d] %d/%d\n", ctx->which, ctx->ip, ctx->port);
      }
      break;
    case ATOM4('log-'): // log-enable bool
      if (argc) {
        if (x) { ctx->log_enable = 1; } else { ctx->log_enable = 0; }
      }
      break;
    case ATOM4('___l'): // delayed velocity amount (doesn't propogate)
      if (argc && isfinite(arg[0])) envelope_velocity(voice, arg[0]);
      break;
    case ATOM4('l---'): // velocity amount
      if (argc) {
        uint64_t now = SAMPLE_COUNT_GET();
        int a = sv.link_velo_0[voice];
        int b = sv.link_velo_1[voice];
        int c = sv.link_velo_2[voice];
        int d = sv.link_velo_3[voice];
        double vel = arg[0];
        skode_envelope_velocity(voice, vel, now);
        if (a >= 0) skode_envelope_velocity(a, vel, now);
        if (b >= 0) skode_envelope_velocity(b, vel, now);
        if (c >= 0) skode_envelope_velocity(c, vel, now);
        if (d >= 0) skode_envelope_velocity(d, vel, now);
      }
      break;
    case ATOM4('m---'): // mute-audio bool
      if (argc) { wave_mute(voice, x); }
      break;
    case ATOM4('M---'): // tempo bpm
      if (argc) { tempo_set(arg[0]); }
      break;
    case ATOM4('n---'): // midi-freq note-number (cents)
      if (argc) {
        float note = arg[0];
        float cents = 0.0;
        if (argc > 1) cents = arg[1];
        skode_midi_note(voice, note, cents);
      }
      break;
    case ATOM4('N---'): // detune-midi key cents
      if (argc) {
        if (isnan(arg[0])) {
          // do nothing
        } else {
          sv.midi_transpose[voice] = arg[0];
        }
        if (argc > 1) sv.midi_cents[voice] = arg[1];
      }
      break;
    case ATOM4('p---'): // pan value
      if (argc) pan_set(voice, arg[0]);
      break;
    case ATOM4('P---'): // pan-mod voice depth
      if (argc < 2) {
        pan_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        pan_mod_set(voice, x, arg[1], a);
      }
      break;
    case ATOM4('q---'):  // bit-crush bit-depth
      if (argc) { wave_quant(voice, x); }
      break;
    case ATOM4('Q---'):
      if (argc) { mmf_set_res(voice, arg[0]); }
      break;
    case ATOM4('r---'): // additional recording track, 0=none, 1..4=stem
      if (argc) synth_record_track_set(voice, x);
      break;
    case ATOM4('R!--'):  // remove-events tag
      if (argc) {
        int tag = x;
        seq_kill_by_tag(tag);
      }
      break;
    case ATOM4('R!!-'):
      seq_kill_all();
      break;
    case ATOM4('RR--'): // repeat-string-tempo count delay [tag]
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        uint64_t qt = SAMPLE_COUNT_GET();
        double t = (tempo_time_per_step * 4.0f);
        uint64_t dt;
        if (!skode_seconds_to_samples(t * arg[1], &dt)) break;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        for (int i=0; i<x; i++) {
          if (skode_queue_program(&program, ctx->voice, qt, tag) != 0) break;
          qt = skode_u64_add(qt, dt);
        }
      } break;
    case ATOM4('DO?-'): // conditional-string-if-gt-zero number [tag]
      if (argc && x>0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        int tag = 0;
        if (argc > 1) skode_double_to_int(arg[1], &tag);
        uint64_t qt = SAMPLE_COUNT_GET();
        skode_queue_program(&program, ctx->voice, qt, tag);
      } break;
    case ATOM4('R---'): // repeat-string count delay [tag]
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        uint64_t qt = SAMPLE_COUNT_GET();
        uint64_t dt;
        if (!skode_seconds_to_samples(arg[1], &dt)) break;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        for (int i=0; i<x; i++) {
          if (skode_queue_program(&program, ctx->voice, qt, tag) != 0) break;
          qt = skode_u64_add(qt, dt);
        }
      } break;
    case ATOM4('s---'): // volume-smooth bool
      if (argc) {
        if (arg[0] <= 0) {
          sv.smoother_enable[voice] = 0;
        } else {
          sv.smoother_enable[voice] = 1;
          sv.smoother_smoothing[voice] = arg[0];
        }
      }
      break;
    case ATOM4('S---'): // voice-reset voice
      if (argc) wave_reset(x);
      break;
    case ATOM4('t---'): // adsr-set attack decay sustain release
      if (argc > 3) envelope_set(voice, arg[0], arg[1], arg[2], arg[3]);
      break;
    case ATOM4('T---'): // trigger
      {
        envelope_velocity(voice, 1);
        if (sv.link_velo_0[voice] >= 0) envelope_velocity(sv.link_velo_0[voice], 1);
        if (sv.link_velo_1[voice] >= 0) envelope_velocity(sv.link_velo_1[voice], 1);
        if (sv.link_velo_2[voice] >= 0) envelope_velocity(sv.link_velo_2[voice], 1);
        if (sv.link_velo_3[voice] >= 0) envelope_velocity(sv.link_velo_3[voice], 1);
      }
      break;
    case ATOM4('v---'): // voice-select voice
      if (argc) voice_set(x, &ctx->voice);
      break;
    case ATOM4('V---'): // main-volume loudness
      if (argc) volume_set(arg[0]);
      break;
    case ATOM4('vt--'): // [name] voice-text-set
      skode_copy_string(sv.text[voice], TEXT_MAX, ands_string(ctx->parse));
      break;
    case ATOM4('wt--'): // [name] wave-text-set wave-number
      if (argc && x >= 0 && x < WAVE_TABLE_MAX) {
        skode_copy_string(sw.name[x], WAVE_NAME_MAX, ands_string(ctx->parse));
      }
      break;
    case ATOM4('w---'): // wave-select which-wave interpolate? one-shot?
      if (argc) {
        wave_set(voice, x);
        int n;
        if (argc > 1) {
          if (skode_double_to_int(arg[1], &n)) sv.interpolate[voice] = n != 0;
        }
        if (argc > 2) {
          if (skode_double_to_int(arg[2], &n)) sv.one_shot[voice] = n != 0;
        }
      }
      break;
    case ATOM4('=d--'): // assign a variable from an element of the d array =d var d-index
      if (argc > 1 && x_valid) {
        int y;
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (skode_double_to_int(arg[1], &y) &&
            x >= 0 && x < 128 && y >= 0 && y < data_len) {
          // x is the dest var y is the d index
          ands_set_local(ctx->parse, x, data[y]);
        }
      }
      break;
    case ATOM4('d@--'): // show an element from d array
      if (argc) {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (x>=0 && x < data_len) {
          ctx->printf(ctx, "# %g\n", data[x]);
        }
      }
      break;
    case ATOM4('d>r-'): // data-to-rec
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (!data || data_len <= 0) break;
        skode_sample_alloc(data_len);
        if (sampling.where && data_len <= sampling.capacity) {
          for (int i=0; i<data_len; i++) sampling.where[i] = (float)data[i];
          sampling.len = data_len;
          sampling.offset = 0;
          sampling.trim = 0;
        } else {
          ctx->printf(ctx, "NO!\n");
        }
      }
      break;
    case ATOM4('w>d-'): // wave-to-data
      if (x_valid && skode_wave_valid(x) && sw.data[x] && sw.size[x] > 0) {
        if (sw.size[x] > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, sw.size[x]);
        double *data = ands_data(ctx->parse);
        if (!data || sw.size[x] > ands_data_cap(ctx->parse)) break;
        for (int i=0; i<sw.size[x]; i++) data[i] = sw.data[x][i];
        ands_data_len_set(ctx->parse, sw.size[x]);
      }
      break;
    case ATOM4('w>r-'): // wave-to-rec
      if (x_valid && skode_wave_valid(x) && sw.data[x] && sw.size[x] > 0) {
        int valid = 1;
        if (sw.size[x] > sampling.capacity) {
          if (sampling.busy) {
            valid = 0;
          } else {
            skode_sample_alloc(sw.size[x]);
            valid = sampling.where != NULL && sampling.capacity >= sw.size[x];
          }
        }
        if (valid) {
          sampling.offset = 0;
          sampling.trim = 0;
          for (int i=0; i<sw.size[x]; i++) sampling.where[i] = sw.data[x][i];
          sampling.len = sw.size[x];
        }
      }
      break;
    case ATOM4('w!--'): // wave-lock
      {
        if (!sampling.where || sampling.offset < 0 || sampling.trim < 0 ||
            sampling.offset > sampling.len ||
            sampling.trim > sampling.len - sampling.offset) {
          ctx->printf(ctx, "# invalid recording bounds\n");
          break;
        }
        int j = 0;
        for (int i=sampling.offset; i<sampling.len-sampling.trim; i++) {
          sampling.where[j++] = sampling.where[i];
        }
        sampling.len = sampling.len - sampling.offset - sampling.trim;
        sampling.trim = 0;
        sampling.offset = 0;
      }
      break;
    case ATOM4('w@--'): // wave-nudge-reset
      sampling.offset = 0;
      sampling.trim = 0;
      break;
    case ATOM4('w>--'): // wave-nudge-start
      if (argc == 0) x = 1;
      if (argc == 0 || x_valid) {
        long long next = (long long)sampling.offset + x;
        if (next < 0) next = 0;
        if (next > sampling.len) next = sampling.len;
        sampling.offset = (int)next;
        if (sampling.trim > sampling.len - sampling.offset)
          sampling.trim = sampling.len - sampling.offset;
      }
      break;
    case ATOM4('w<--'): // wave-nudge-len
      if (argc == 0) x = 1;
      if (argc == 0 || x_valid) {
        long long next = (long long)sampling.trim + x;
        int max_trim = sampling.len - sampling.offset;
        if (max_trim < 0) max_trim = 0;
        if (next < 0) next = 0;
        if (next > max_trim) next = max_trim;
        sampling.trim = (int)next;
      }
      break;
    case ATOM4('w<>-'): // wave-auto-trim
      {
        float arg0 = -1;
        float arg1 = -1;
        if (argc > 0) arg0 = arg[0];
        if (argc > 1) arg1 = arg[1];
        record_find_trim(argc, arg0, arg1);
      }
      break;
    case ATOM4('W---'): // wave-show which-wave
      if (argc) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT;
        int m = 0;
        int show_record_buffer = (arg[0] < 0 || isnan(arg[0]));
        if (show_record_buffer) {
          if (argc > 1) {
            w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          }
          if (argc > 2) {
            h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
          }
        } else if (argc == 2) {
          if (isnan(arg[1])) m = WAVE_TABLE_MAX - 1;
          else if (!skode_double_to_int(arg[1], &m)) m = x;
          if (m < x) m = x;
          if (m >= WAVE_TABLE_MAX) m = WAVE_TABLE_MAX - 1;
        } else if (argc >= 3) {
          w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        }
        if (!show_record_buffer && skode_wave_valid(x)) {
          if (m == 0) {
            wavetable_show(ctx, x);
            print_audio_braille_connected(ctx, sw.data[x], sw.size[x], w, h);
          } else {
            for (int i=x; i<=m; i++) {
              wavetable_show(ctx, i);
            }
          }
        } else {
          if (sampling.where) {
            if ((sampling.offset > sampling.len) || (sampling.len - sampling.trim <= 0)) {
              ctx->printf(ctx,"NO!\n");
              ctx->printf(ctx, "offset:%d\n", sampling.offset);
              ctx->printf(ctx, "trim:%d\n", sampling.trim);
              ctx->printf(ctx, "len:%d\n", sampling.len);
              ctx->printf(ctx, "where:%p\n", sampling.where);
              ctx->printf(ctx, "busy:%d\n", sampling.busy);
              ctx->printf(ctx, "go:%d\n", sampling.go);
            } else {
              print_wave_stats(ctx, "recording", sampling.where, sampling.len, (float)MAIN_SAMPLE_RATE);
              print_audio_braille_labeled(ctx, sampling.where, sampling.len, w, h, sampling.offset, sampling.len - sampling.trim);
              int len = sampling.len - sampling.offset - sampling.trim;
              ctx->printf(ctx,"+offset %d -trim %d = |%d| %gms\n",
                sampling.offset, sampling.trim, len,
                SAMPLES_TO_MSEC(len));
            }
          }
        }
      } else if (argc == 0) {
        int c = 0;
        ctx->printf(ctx, "# MAX %d\n", WAVE_TABLE_MAX);
        for (int i=0; i<WAVE_TABLE_MAX; i++) {
          wavetable_show(ctx, i);
          c++;
        }
      }
      break;
    case ATOM4('xg--'): // goto-step #
    case ATOM4('>x--'): // goto-step #
      seq_step_goto(ctx->pattern, x);
      break;
    case ATOM4('xa--'): // append step
      {
        const char *source = ands_string(ctx->parse);
        event_program_t program;
        int source_only = source[0] == '\0' || strcmp(source, "-") == 0;
        skode_compile_result_t result = source_only ?
          SKODE_COMPILE_OK : skode_compile_program(source, &program);
        if (result == SKODE_COMPILE_OK) {
          seq_step_append(ctx->pattern, source, source_only ? NULL : &program);
        } else {
          ctx->printf(ctx, "# sequence command is not schedulable (%d)\n", result);
        }
      }
      break;
    case ATOM4('<x--'): // (pattern) step-string-to-skode step-number
      if (arg == 0) {
      } else {
        char *s = seq_step_get(ctx->pattern, x);
        ands_string_from_external(ctx->parse, s, strlen(s));
      }
      break;
    case ATOM4('x---'): // set-step-string step
      if (argc) {
        if (isnan(arg[0]) || !x_valid || x < 0) {
          ctx->step++;
          x = ctx->step;
        } else {
          ctx->step = x;
        }
        if (x >= 0 && x < SEQ_STEPS_MAX) {
          const char *source = ands_string(ctx->parse);
          event_program_t program;
          int source_only = source[0] == '\0' || strcmp(source, "-") == 0;
          skode_compile_result_t result = source_only ?
            SKODE_COMPILE_OK : skode_compile_program(source, &program);
          if (result == SKODE_COMPILE_OK) {
            seq_step_set(ctx->pattern, ctx->step, source,
              source_only ? NULL : &program);
          } else {
            ctx->printf(ctx, "# sequence command is not schedulable (%d)\n", result);
          }
        }
      }
      break;
    case ATOM4('y---'): // select-pattern which
      if (argc && x >= 0 && x < PATTERNS_MAX) {
        ctx->pattern = x;
        scope_pattern_pointer = x;
      }
      break;
    case ATOM4('yt--'): // {note} pattern-text
      if (ctx->pattern >= 0 && ctx->pattern < PATTERNS_MAX) {
        skode_copy_string(seq_text[ctx->pattern], TEXT_MAX, ands_string(ctx->parse));
      }
      break;
    case ATOM4('ym--'): // pattern-mute 0/1
      if (argc && ctx->pattern >= 0 && ctx->pattern < PATTERNS_MAX) seq_mute[ctx->pattern] = x;
      break;
    case ATOM4('Y---'): // clear-pattern which
      if (argc && x >= 0 && x < PATTERNS_MAX) {
        pattern_reset(x);
      }
      break;
    case ATOM4('z---'): // one-pattern-play-mode bool
      if (argc) {
        seq_state_set(ctx->pattern, x);
      } else pattern_show(ctx, ctx->pattern, 1);
      break;
    case ATOM4('z?--'): // one-pattern-play-mode bool
      pattern_show(ctx, ctx->pattern, 1);
      break;
    case ATOM4('Z---'): // all-pattern-play-mode bool
      if (argc) {
        seq_state_all(x);
      } else {
        ctx->printf(ctx, "M%g\n", tempo_bpm * 4.0f);
        for (int p = 0; p < PATTERNS_MAX; p++) pattern_show(ctx, p, 0);
      }
      break;
    case ATOM4('z?\?-'): // show all patterns
    case ATOM4('Z?--'): // show all patterns
      ctx->printf(ctx, "M%g\n", tempo_bpm * 4.0f);
      for (int p = 0; p < PATTERNS_MAX; p++) pattern_show(ctx, p, 1);
      break;
    case ATOM4('v?--'): // show-voice
    case ATOM4('?---'): // show-voice
      voice_show(ctx, voice, ' ', ctx->verbose); break;
    case ATOM4('\\---'): // verbose-show-voice
      voice_show(ctx, voice, ' ', 1); break;
    case ATOM4('v?\?-'): // show-active-voices
    case ATOM4('?\?--'): // show-active-voices
      voice_show_all(ctx, voice, ctx->verbose); break;
    case ATOM4('?s--'): // show-skode-string
      ctx->printf(ctx, "# [%s]\n", ands_string(ctx->parse));
      break;
    case ATOM4('?o--'): // show compiled opcode queue or pattern
      if (argc == 0) {
        opcode_queue_show(ctx);
      } else {
        if (!x_valid) {
          ctx->printf(ctx, "# invalid opcode pattern\n");
          break;
        }
        int pattern = x;
        int step = -1;
        if (pattern == -1) pattern = ctx->pattern;
        if (argc > 1 && !skode_double_to_int(arg[1], &step)) {
          ctx->printf(ctx, "# invalid opcode step\n");
          break;
        }
        opcode_pattern_show(ctx, pattern, step);
      }
      break;
    case ATOM4('l>g-'):
      if (argc) ands_local_to_global(ctx->parse, x);
      break;
    case ATOM4('g>l-'):
      if (argc) ands_global_to_local(ctx->parse, x);
      break;
    case ATOM4('/q--'): // quit
      ctx->quit = -1;
      return 0;
    case ATOM4('/rg-'): // start multitrack file recording
      {
        const char *filename = ands_string(ctx->parse);
        double max_seconds = argc ? arg[0] : 0.0;
        if (!filename || filename[0] == '\0') {
          ctx->printf(ctx, "# /rg requires [filename]\n");
        } else if (!isfinite(max_seconds) || max_seconds < 0.0) {
          ctx->printf(ctx, "# /rg duration must be >= 0\n");
        } else if (recorder_start(filename, max_seconds) == 0) {
          if (max_seconds > 0.0) {
            ctx->printf(ctx, "# recording [%s] max=%g seconds\n",
                        filename, max_seconds);
          } else {
            ctx->printf(ctx, "# recording [%s]\n", filename);
          }
        } else {
          ctx->printf(ctx, "# recording start failed [%s]\n", filename);
        }
      }
      break;
    case ATOM4('/rs-'): // stop multitrack file recording
      if (recorder_stop() == 0) {
        ctx->printf(ctx, "# recording stopped\n");
      } else {
        ctx->printf(ctx, "# recording stop failed\n");
      }
      break;
    case ATOM4('/r?-'): // multitrack file recording status
      {
        const char *state = "unknown";
        switch (recorder_state()) {
          case RECORDER_STOPPED: state = "stopped"; break;
          case RECORDER_RECORDING: state = "recording"; break;
          case RECORDER_STOPPING: state = "stopping"; break;
          case RECORDER_ERROR: state = "error"; break;
        }
        ctx->printf(ctx, "# recorder state=%s frames=%llu dropped=%llu\n",
                    state,
                    (unsigned long long)recorder_frames_written(),
                    (unsigned long long)recorder_dropped_frames());
      }
      break;
    case ATOM4('/r--'): // sample-to-wave slot one_shot offset
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 1;
        float offset = 1;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) break;
        if (argc > 1) skode_double_to_int(arg[1], &one_shot);
        if (argc > 2 && isfinite(arg[2])) offset = arg[2];
        rec_load(ctx, wave_slot, one_shot, offset);
      }
      break;
                        //              x/0  1     2        3
                        //              300  44100 one-shot offset
    case ATOM4('/d--'): // data-to-wave slot rate  one-shot offset
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = 44100.0;
        float offset = 0.0;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) break;
        if (argc > 1) rate = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &one_shot);
        if (argc > 3) offset = arg[3];
        data_load(ctx, wave_slot, one_shot, rate, offset);
      }
      break;
    case ATOM4('/f--'): // flag-mode num
      if (argc) { ctx->flag = x; }
      else { ctx->printf(ctx, "# /f%d\n", ctx->flag); }
      break;
    case ATOM4('/c--'): // chunk-mode bool
      if (argc) { ands_chunk_mode(ctx->parse, x); }
      else { ctx->printf(ctx, "# /c%d\n", ands_chunk_mode_get(ctx->parse)); }
      break;
    case ATOM4('/t--'): // trace-mode num
      if (argc == 0) x = (ctx->trace) ? 0 : 1;
      ctx->trace = x;
      ands_trace_set(s, x > 1);
      break;
    case ATOM4('/v--'): // verbose-mode num
      if (argc == 0) x = (ctx->verbose) ? 0 : 1;
      ctx->verbose = x;
      break;
    case ATOM4('<e--'): // external-string-to-skode external-index
      if (argc && skode_extra_valid(x)) {
        ands_string_from_external(ctx->parse, EXTRA_PTR(x), STRING_BUF_LEN);
      }
      break;
    case ATOM4('e>--'): // skode-string-to-external external-index
      if (argc && skode_extra_valid(x)) {
        char *s = ands_string(ctx->parse);
        //ands_string_to_extra(ctx->parse, x, s);
        //ands_string_to_external(ctx->parse, EXTRA_PTR(x), STRING_BUF_LEN);
        skode_copy_string(EXTRA_PTR(x), STRING_BUF_LEN, s);
      }
      break;
    case ATOM4('e!--'): // execute-string num
      {
        char *s = "";
        if (argc == 0) {
          s = ands_string(ctx->parse);
        } else if (skode_extra_valid(x)) {
          s = EXTRA_PTR(x);
        }
        if (s[0] != '\0') {
          event_program_t program;
          if (!skode_compile_scheduled(ctx, s, &program)) break;
          uint64_t now = SAMPLE_COUNT_GET();
          int tag = 0;
          skode_queue_program(&program, voice, now, tag);
        }
      }
      break;
    case ATOM4('e?--'): // show-execute-string [num]
      if (argc) {
        if (skode_extra_valid(x)) ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(x), x);
      } else {
        for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
          if (strlen(EXTRA_PTR(i)))
            ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
        }
      }
      break;
    case ATOM4('/s--'): // system-show num
      {
        if (argc == 0) {
          system_show(ctx);
        } else {
          switch (x) {
            default:
            case 0: system_show(ctx); break;
            case 2: audio_show(ctx); break;
            case 3: ctx->printf(ctx, "%s", synth_stats()); break;
            case 5: skode_show(ctx); break;
            case 7:
              for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
                if (strlen(EXTRA_PTR(i)))
                  ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
              }
              break;
          }
        }
      }
      break;
    case ATOM4('/l--'): // skode-load num
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        skode_load(ctx, voice, x, verbose);
      }
      break;
    case ATOM4('/ws-'): // wave-load-string wave channel
      ctx->printf(ctx, "# [%s] /ws\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        char *file_name = ands_string(ctx->parse);
        int wave_slot = EXT_SAMPLE_000;
        int ch = -1;
        if (argc >= 1) {
          if (!skode_double_to_int(arg[0], &wave_slot)) break;
          if (argc > 1) {
            if (!skode_double_to_int(arg[1], &ch)) ch = -1;
          }
        }
        ctx->printf(ctx, "# [%s] /ws %d %d\n", ands_string(ctx->parse), wave_slot, ch);
        wave_load_string(ctx, file_name, wave_slot, ch, 1);
      }
      break;
    case ATOM4('/w--'): // wave-load num wave channel
      {
        int file_num = 0;
        int wave_slot = EXT_SAMPLE_000;
        int ch = -1;
        if (argc >= 2) {
          if (!skode_double_to_int(arg[0], &file_num) ||
              !skode_double_to_int(arg[1], &wave_slot)) break;
          if (argc > 2 && !skode_double_to_int(arg[2], &ch)) ch = -1;
        } else if (argc == 1) {
          if (!skode_double_to_int(arg[0], &file_num)) break;
          wave_slot = EXT_SAMPLE_000;
        }
        if (argc) wave_load(ctx, file_num, wave_slot, ch, 1);
      }
      break;
    case ATOM4('>r--'): // record to file
      if (x_valid && sampling.where && sampling.len > 0) {
        char filename[64];
        snprintf(filename, sizeof(filename), "out-%d.wav", x);
        ma_encoder_config config;
        ma_encoder encoder;
        float *where;
        int len = sampling.len;
        where = (float *)malloc(len * sizeof(float));
        if (!where) break;
        for (int i=0; i<len; i++) where[i] = sampling.where[i];
        normalize_buffer(where, len, 1);
        config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, 44100);
        if (ma_encoder_init_file(filename, &config, &encoder) != MA_SUCCESS) {
        } else {
          ma_encoder_write_pcm_frames(&encoder, where, len, NULL);
          ma_encoder_uninit(&encoder);
        }
        free(where);
      }
      break;
    case ATOM4('^r--'): // record duration ... markdown/html doesn't like <
    case ATOM4('<r--'): // record duration
      if (argc && isfinite(arg[0]) && arg[0] > 0.0 &&
          arg[0] <= (double)(INT_MAX - 4096) / MAIN_SAMPLE_RATE) {
        skode_sample_go((int)(arg[0] * (double)MAIN_SAMPLE_RATE));
      } else {
        ctx->printf(ctx, "skode_sample -> %d %d %d\n", sampling.busy, sampling.go, sampling.frames);
      }
      break;
    case ATOM4('>---'): // copy-voice dest-voice
      if (x_valid && skode_voice_valid(x)) voice_copy(voice, x);
      break;
    case ATOM4('/---'): // default-wave voice
      wave_default(voice);
      break;
    case ATOM4('%---'): // pattern-modulus num
      if (argc) seq_modulo_set(ctx->pattern, x);
      break;
    case ATOM4('W@--'):  // get a wavetable parameter to a variable
      if (argc > 1 && x_valid && skode_wave_valid(x)) {
        int wave = x;
        int param;
        if (!skode_double_to_int(arg[1], &param)) break;
        double val = 0.0;
        switch (param) {
          case 0: // wavetable size
            val = sw.size[wave];
            break;
          case 1: // wavetable rate
            val = sw.rate[wave];
            break;
          case 2: // wavetable size / rate
            val = (float)sw.size[wave] / sw.rate[wave];
            break;
          default:
            argc = 0; // hack to do-nothing on unknown parameter
            break;
        }
        if (argc > 2) {
          int variable;
          if (skode_double_to_int(arg[2], &variable))
            ands_set_local(ctx->parse, variable, val);
        } else if (argc) {
          ctx->printf(ctx, "# W@ %d %d -> %g\n", wave, param, val);
        }
      }
      break;
    case ATOM4('v@--'):  // get a voice parameter to a variable
      if (argc) {
        double val = 0.0;
        switch (x) {
          case 0: // wavetable index
            val = sv.wave_table_index[voice];
            break;
          case 1: // amplitide
            val = sv.user_amp[voice];
            break;
          case 2: // freq
            val = sv.freq[voice];
            break;
          default:
            argc = 0; // hack to do-nothing on unknown parameter
            break;
        }
        if (argc > 1) {
          int y;
          if (skode_double_to_int(arg[1], &y))
            ands_set_local(ctx->parse, y, val);
        } else if (argc) {
          ctx->printf(ctx, "# v@ %d -> %g\n", x, val);
        }
      }
      break;
    case ATOM4('*=--'):  // variable-times-equal slot val0 val1
      if (argc > 2) ands_set_local(ctx->parse, x, arg[1] * arg[2]);
      break;
    case ATOM4('/=--'):  // variable-divide-equal slot val0 val1
      if (argc > 2 && arg[2] != 0.0) ands_set_local(ctx->parse, x, arg[1] / arg[2]);
      break;
    case ATOM4('a=--'):  // variable-plus-equal slot val0 val1
      if (argc > 2) ands_set_local(ctx->parse, x, arg[1] + arg[2]);
      break;
    case ATOM4('s=--'):  // variable-sub-equal slot val0 val1
      if (argc > 2) {
        ands_set_local(ctx->parse, x, arg[1] - arg[2]);
      }
      break;
    case ATOM4('=---'):  // variable-set slot value
      if (argc > 1) ands_set_local(ctx->parse, x, arg[1]);
      else if (argc == 1) {
        double f = ands_get_local(ctx->parse, x);
        ctx->printf(ctx, "# $%d %g\n", x, f);
      }
      else {
        for (int i=0; i<ANDS_VAR_MAX; i++) {
          double f = ands_get_local(ctx->parse, i);
          if (f != 0.0) ctx->printf(ctx, "# $%d %g\n", i, f);
        }
      }
      break;
    case ATOM4('/wex'): // wave-expand wave
      if (argc && x >= 200 && x <=999) wave_table_dynamic_expand(x);
      break;
    case ATOM4('/cat'): // print a text file
      if (strlen(ands_string(ctx->parse))) {
        FILE *in = fopen(ands_string(ctx->parse), "rt");
        if (in) {
          char line[1024];
          while (fgets(line, sizeof(line), in)) {
            char *p = line;
            while (*p) {
              if (*p == '\n' || *p == '\n') {
                *p = '\0';
                break;
              }
              if (!isprint(*p)) {
                *p = '\0';
                break;
              }
              p++;
            }
            ctx->printf(ctx, "%s\n", line);
          }
          fclose(in);
        }
      }
      break;
    case ATOM4('/cd-'): // change directory
      ctx->printf(ctx, "# [%s] /cd\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        chdir(ands_string(ctx->parse));
      }
      break;
    case ATOM4('/ls-'): // list directory (match-type)
      {
      /*
          types
          0 = .sk
          1 = .wav
          2 = .mp3
          3 = .ks
      */
      int p = -1;
      if (argc) p = x;
      struct dirent *entry;
      DIR *dp = opendir(".");
        if (dp) {
          while ((entry = readdir(dp))) {
            char *name = entry->d_name;
            int f = 0;
            switch (p) {
              default:
              case -1:
                f = 1;
                break;
              case 0:
                f = (strstr(name, ".sk") != NULL);
                break;
              case 1:
                f = (strstr(name, ".wav") != NULL);
                break;
              case 2:
                f = (strstr(name, ".mp3") != NULL);
                break;
              case 3:
                f = (strstr(name, ".ks") != NULL);
                break;
            }
            if (f) ctx->printf(ctx, "# [%s]\n", name);
          }
          closedir(dp);
        }
      }
      break;
    default:
      ctx->printf(ctx, "# unknown atom\n");
      if (ctx->trace) {
        ctx->printf(ctx, "# SKODE_UNKNOWN_FUNCTION %d [%x] :: %d", info, atom, argc);
        ctx->printf(ctx, " v%d", ctx->voice);
        ctx->puts(ctx, "");
      }
      break;
  }
  return 0;
}

int skode_defer(ands_t *s, int info) {
  (void)info;
  skode_t *ctx = (skode_t*)ands_user(s);
  char mode = ands_defer_mode(s);
  double delay = ands_defer_num(s);
  if (!isfinite(delay)) return 0;
  if (delay <= 0.0) ctx->defer_last = 0.0;
  if (ctx->defer_sample_time == 0) {
    ctx->defer_sample_time = SAMPLE_COUNT_GET();
  }
  uint64_t dst = ctx->defer_sample_time;
  if (mode == '+') delay *= (tempo_time_per_step * 4.0f);
  double t = ctx->defer_last + delay;
  uint64_t relative;
  if (!skode_seconds_to_samples(t, &relative)) return 0;
  uint64_t qt = skode_u64_add(dst, relative);
  if (ctx->trace) {
#ifdef _WIN32
    ctx->printf(ctx, "# SKODE_DEFER %c %g(%lld/%lld) '%s' (%g)\n",
#else
    ctx->printf(ctx, "# SKODE_DEFER %c %g(%ld/%ld) '%s' (%g)\n",
#endif
      mode,
      t, qt, dst,
      ands_defer_string(s),
      ctx->defer_last);
  }
  event_program_t program;
  if (!skode_compile_scheduled(ctx, ands_defer_string(s), &program)) return 0;
  skode_queue_program(&program, ctx->voice, qt, -1);
  // If this defer is created while seq() is already running a pattern step,
  // a due-now event such as +0 will not be drained until the next callback.
  // Revisit if mixed immediate/deferred pattern attacks need tighter alignment.
  ctx->defer_last = t;
  return 0;
}

int skode_chunk_end(ands_t *s, int info) {
  skode_t *ctx = (skode_t*)ands_user(s);
  if (ctx->trace) ctx->printf(ctx, "# CHUNK_END %d\n", info);
  ctx->defer_last = 0;
  ctx->defer_sample_time = 0;
  return 0;
}

int skode_unknown(skode_t *ctx, ands_t *s, int info) {
  (void)s;
  ctx->printf(ctx, "# SKODE_UNKNOWN %d\n", info);
  return 0;
}

int skode_callback(ands_t *s, int info) {
  skode_t *ctx = (skode_t*)ands_user(s);
  switch (info) {
    case FUNCTION: return skode_function(s, info);
    case DEFER: return skode_defer(s, info);
    case CHUNK_END: return skode_chunk_end(s, info);
    case GOT_STRING: { if (ctx->trace) ctx->printf(ctx, "# -> [%s]\n", ands_string(s)); } break;
    case GOT_ARRAY: { if (ctx->trace) ctx->printf(ctx, "# -> (..%d..)\n", ands_data_len(s)); } break;
    default: return skode_unknown(ctx, s, info);
  }
  return 0;
}

double global_var[ANDS_VAR_MAX];


int skode_consume(char *line, skode_t *ctx) {
  if (!line || !ctx) return -1;
  if (ctx->parse == NULL) {
    // TODO this should live in wire-init or similar
    ctx->parse = ands_new(skode_callback, (void *)ctx);
    if (!ctx->parse) return -1;
    ands_set_global(ctx->parse, global_var);
  }
  ctx->log_len = 0;
  ctx->log[0] = '\0';
  skode_ctx[skode_hash(ctx)] = ctx;

  int r = 0;

  ands_consume(ctx->parse, line);
  return ctx->quit;
  return r;
}

int audio_show(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  ctx->printf(ctx, "# synth backend is running\n");
  ctx->printf(ctx, "# synth total voice count %d\n", synth_config.voice_max);
  int active = 0;
  for (int i = 0; i < synth_config.voice_max; i++) if (sv.amp[i] != 0) active++;
  ctx->printf(ctx, "# synth active voice count %d\n", active);
#ifdef _WIN32
  ctx->printf(ctx, "# synth sample count %lld\n", SAMPLE_COUNT_GET());
#else
  ctx->printf(ctx, "# synth sample count %ld\n", SAMPLE_COUNT_GET());
#endif
  return 0;
}

void skode_init(skode_t *ctx) {
  static int first = 1;
  if (first) {
    for (int i = 0; i < SKODE_CTX_MAX; i++) {
      skode_ctx[i] = NULL;
    }
    EXTRA_INIT();
    first = 0;
  }
  ctx->voice = 0;
  ctx->pattern = 0;
  ctx->step = -1;
  ctx->trace = 0;
  ctx->verbose = 0;
  ctx->parse = NULL;
  ctx->quit = 0;
  ctx->puts = skode_puts;
  ctx->printf = skode_printf;
  ctx->log_enable = 0;
  ctx->log_max = SKODE_LOG_MAX;
  ctx->log_len = 0;
  ctx->log[0] = '\0';
  ctx->ks_wait_seq = 0;
  ctx->ks_wait_writer = 0;
}
