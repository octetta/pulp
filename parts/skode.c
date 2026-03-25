#include "skred.h"
#include "skode.h"
#include "seq.h"
#include "miniwav.h"

#include "synth-types.h"
#include "synth.h"
#include "synth-state.h"
#include "synth-config.h"

#include <stdarg.h>
#include <stdio.h>

int skode_puts(skode_t *ctx, const char *s) {
  if (!ctx || ctx->log_enable == 0) return 0;
  if (ctx->log_len + strlen(s) >= SKODE_LOG_MAX) return 0;
  strncat(ctx->log, s, ctx->log_max);
  strncat(ctx->log, "\n", ctx->log_max);
  ctx->log_len = strlen(ctx->log);
  return 0;
}

int skode_printf(skode_t *ctx, const char *fmt, ...) {
  if (!ctx || ctx->log_enable == 0) return 0;
  if (ctx->log_len + strlen(fmt) >= SKODE_LOG_MAX) return 0;
  //puts("PRINTF");
  char buf[SKODE_LOG_MAX + 1024];
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (len > 0) {
    size_t out_len = (len >= (int)sizeof(buf)) ? sizeof(buf)-1 : (size_t)len;
    if (out_len) {
      strncat(ctx->log, buf, ctx->log_max);
      ctx->log_len = strlen(ctx->log);
    }
  }
  return 0;
}

int null_puts(const char *s) { return 0; }
int null_printf(const char *fmt, ...) { return 0; }

#define MPSC_QUEUE_IMPLEMENTATION
#include "mpsc_queue.h"
static mpsc_queue mq;

#if 0 // performance event listener
#ifndef _WIN32
#include <pthread.h>
#endif

static pthread_t perf_thread_handle;
static int perf_running = 1;
#include "util.h"
static void *perf_main(void *arg) {
  char msg[65536];
  //util_set_thread_name("perf");
  while (perf_running) {
    mpsc_queue_receive(&mq, msg, sizeof(msg));
    //bool r = mpsc_queue_receive(&mq, msg, sizeof(msg));
    //printf("# perf_main (%d) <%s>\n", r, msg);
  }
  return NULL;
}

int perf_start(void) {
  mpsc_queue_init(&mq);
  perf_running = 1;
  pthread_create(&perf_thread_handle, NULL, perf_main, NULL);
  pthread_detach(perf_thread_handle);
  return 0;
}

void perf_stop(void) {
  perf_running = 0;
  mpsc_queue_send(&mq, "#"); // tickle perf_main to make sure it sees the flag change
}

#endif

void voice_push(voice_stack_t *s, float n) {
  s->ptr++;
  if (s->ptr >= VOICE_STACK_LEN) s->ptr = 0;
  s->s[s->ptr] = n;
}

float voice_pop(voice_stack_t *s) {
  float n = s->s[s->ptr];
  s->ptr--;
  if (s->ptr < 0) s->ptr = VOICE_STACK_LEN-1;
  return n;
}

#include <stdio.h>
#include <stdint.h>
#include <math.h>


void voice_show(skode_t *ctx, int v, char c, int verbose) {
  char s[1024];
  char e[8] = "";
  if (c != ' ') sprintf(e, " # *");
  voice_format(v, s, sizeof(s), verbose);
  if (strlen(s)) ctx->printf(ctx, "; %s%s\n", s, e);
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
#define EXTRA_PTR(n) _skode_extra[n % STRING_BUF_IDX_MAX]
#define EXTRA_INIT() {for (int i=0; i<STRING_BUF_IDX_MAX; i++) EXTRA_PTR(i)[0] = '\0';}

int skode_hash(skode_t *ctx) {
  uintptr_t addr = (uintptr_t)ctx;
  addr *= 2654435769u; // knuth's multiplicitive hash (based on golden thingy?)
  return addr % SKODE_CTX_MAX;
}

void skode_show(skode_t *ctx) {
  if (ctx != NULL) {
    ctx->printf(ctx, "# voice %d\n", ctx->voice);
    ctx->printf(ctx, "# pattern %d\n", ctx->pattern);
    ctx->printf(ctx, "# scratch %s\n", ands_string(ctx->parse));
    ctx->printf(ctx, "( ");
    int flag = 1;
    int show_dots = 0;
    double *data = ands_data(ctx->parse);
    int data_len = ands_data_len(ctx->parse);
#define DOT_NUM (3)
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
    ctx->printf(ctx, ") # %d elements\n", data_len);
  }
  for (int i = 0; i < SKODE_CTX_MAX; i++) {
    if (skode_ctx[i]) {
      ctx->printf(ctx, "# ctx[%d] ", skode_hash(skode_ctx[i]));
      ctx->printf(ctx, " v%d", skode_ctx[i]->voice);
      ctx->printf(ctx, " y%d", skode_ctx[i]->pattern);
      ctx->printf(ctx, " x%d", skode_ctx[i]->step);
      //ctx->printf(ctx, " .events=%dn", skode_ctx[i]->events);
      ctx->printf(ctx, " .which=%d", skode_ctx[i]->which);
      ctx->printf(ctx, " .ip=%x", skode_ctx[i]->ip);
      ctx->printf(ctx, " .port=%x", skode_ctx[i]->port);
      ctx->printf(ctx, "\n");
    }
  }
}


void system_show(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
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

int skode_load(skode_t *ctx, int voice, int n) {
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
      ctx->printf(ctx, "  %s\n", line);
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
  if (rate <= 0) {
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

int wave_load(skode_t *ctx, int file_num, int wave_index, int ch, int normalize) {
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
  sprintf(name, "%d.wav", file_num);
  FILE *in = fopen(name, "r");
  if (in) fclose(in);
  else {
    sprintf(name, "wav/%d.wav", file_num);
    in = fopen(name, "r");
    if (in) fclose(in);
    else {
      ctx->printf(ctx, "# cannot open %d.wav or wav/%d.wav\n", file_num, file_num);
      return -1;
    }
  }
  wav_t wav;
  int len;
  char out[4096];
  float *table = mw_get_str(name, &len, &wav, ch, out, sizeof(out));
  if (table == NULL) {
    ctx->printf(ctx, "# can not read %s\n", name);
    return -1;
  } else {
    strncpy(sw.name[wave_index], name, WAVE_NAME_MAX);
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

int wavetable_show(skode_t *ctx, int n) {
  if (n >= 0 && n < WAVE_TABLE_MAX && sw.data[n] && sw.size[n]) {
    float *table = sw.data[n];
    int readonly = sw.readonly[n];
    int refcount = sw.refcount[n];
    int size = sw.size[n];
    ctx->printf(ctx, "# w%d size:%d", n, size);
    ctx->printf(ctx, " rate:%g +hz:%g midi:%g",
      sw.rate[n],
      sw.offset_hz[n],
      sw.midi_note[n]);
    if (readonly) {
      ctx->printf(ctx, " r/o");
    } else {
      ctx->printf(ctx, " r/w ref:%d", refcount);
    }
    ctx->printf(ctx, " '%s'", sw.name[n]);
    ctx->puts(ctx, "");
  } else {
    ctx->printf(ctx, "# w%d nil\n", n);
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

int skode_function(ands_t *s, int info) {
  uint32_t atom = ands_atom_num(s);
  int argc = ands_arg_len(s);
  skode_t *ctx = (skode_t*)ands_user(s);
  double *arg = ands_arg(s);
  int voice = ctx->voice;
  int x = (int)arg[0];
  if (ctx->trace) {
    ctx->printf(ctx, "# SKODE_FUNCTION ");
    ctx->printf(ctx, "%s", ands_atom_string(s));
    if (argc) {
      for (int i=0; i<argc; i++) ctx->printf(ctx, " %g", arg[i]);
    }
    ctx->puts(ctx, "");
  }
  switch (atom) {
    case ATOM4('a---'): // amp loudness
      if (argc) amp_set(voice, arg[0]);
      break;
    case ATOM4('b---'): // wave-direction bool
      if (argc == 0) { wave_dir(voice, -1); } else { wave_dir(voice, x); } break;
    case ATOM4('B---'): // wave-loop bool
      if (argc == 0) { wave_loop(voice, -1); } else { wave_loop(voice, x); } break;
    case ATOM4('D---'): // data-size
      // need to use the data array in skode here, not ctx->data
      break;
    case ATOM4('f---'): // freq hz
      if (argc) freq_set(voice, arg[0]);
      break;
    case ATOM4('G---'): // link-midi voice [voice]
      if (argc) {
        sv.link_midi_a[voice] = x;
        if (argc > 1) sv.link_midi_b[voice] = (int)arg[1];
        if (argc > 2) sv.link_midi_c[voice] = (int)arg[2];
        if (argc > 3) sv.link_midi_d[voice] = (int)arg[3];
      }
      break;
    case ATOM4('H---'): // link-velo voice [voice [voice [voice]]]
      if (argc) {
        sv.link_velo_a[voice] = x;
        if (argc > 1) sv.link_velo_b[voice] = (int)arg[1];
        if (argc > 2) sv.link_velo_c[voice] = (int)arg[2];
        if (argc > 3) sv.link_velo_d[voice] = (int)arg[3];
      }
      break;
    // TODO re-allocate the data/array buffer with the arg
    case ATOM4('/D--'): // resize-data count
      if (argc) {
        // free and re-allocate...
        if (x > 0) ands_data_resize(ctx->parse, x);
      }
      ctx->printf(ctx, "# /D data %p cap %d len %d\n",
        ands_data(ctx->parse),
        ands_data_cap(ctx->parse),
        ands_data_len(ctx->parse));
      break;
    case ATOM4('I---'): // log-event bool
      if (argc) {} break; // TODO en/dis-able send timestamp wire to the event logger
    case ATOM4('L---'): // link-trigger voice
      if (argc) { sv.link_trig[voice] = x; } break;
    case ATOM4('log-'): // log-enable bool
      if (argc) {
        if (x) { ctx->log_enable = 1; } else { ctx->log_enable = 0; }
      }
      break;
    case ATOM4('m---'): // mute-audio bool
      if (argc) { wave_mute(voice, x); }
      break;
    case ATOM4('n---'): // midi-freq note-number
      if (argc) {
        float note = arg[0];
        if (isnan(note)) note = sv.last_midi_note[voice];
        freq_midi(voice, note);
        if (sv.link_midi_a[voice] >= 0) freq_midi(sv.link_midi_a[voice], note);
        if (sv.link_midi_b[voice] >= 0) freq_midi(sv.link_midi_b[voice], note);
        if (sv.link_midi_c[voice] >= 0) freq_midi(sv.link_midi_c[voice], note);
        if (sv.link_midi_d[voice] >= 0) freq_midi(sv.link_midi_d[voice], note);
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
    case ATOM4('S---'): // voice-reset voice
      if (argc) wave_reset(voice, x);
      break;
    case ATOM4('v---'): // voice-select voice
      if (argc) voice_set(x, &ctx->voice);
      break;
    case ATOM4('V---'): // main-volume loudness
      if (argc) volume_set(arg[0]);
      break;
    case ATOM4('w---'): // wave-select which-wave interpolate? one-shot?
      if (argc) {
        wave_set(voice, x);
        int n;
        if (argc > 1) {
          n = (int)arg[1];
          sv.interpolate[voice] = n;
        }
        if (argc > 2) {
          n = (int)arg[2];
          sv.one_shot[voice] = n;
        }
      }
      break;
    case ATOM4('W---'): // wave-show which-wave
      if (argc) {
        wavetable_show(ctx,x);
      } else if (argc == 0) {
        int c = 0;
        for (int i=0; i<WAVE_TABLE_MAX; i++) {
          if (sw.data[i] && sw.readonly[i] == 0) {
            wavetable_show(ctx, i);
            c++;
          }
        }
      }
      break;
    case ATOM4('?---'): // show-voice
      voice_show(ctx, voice, ' ', ctx->verbose); break;
    case ATOM4('\\---'): // verbose-show-voice
      voice_show(ctx, voice, ' ', 1); break;
    case ATOM4('??--'): // show-active-voices
      voice_show_all(ctx, voice, ctx->verbose); break;
    case ATOM4('?s--'): // show-skode-string
      {
        ctx->printf(ctx, "# {%}s\n", ands_string(ctx->parse));
      }
      break;
    case ATOM4('l>g-'):
      if (argc) ands_local_to_global(ctx->parse, x);
      break;
    case ATOM4('g>l-'):
      if (argc) ands_global_to_local(ctx->parse, x);
      break;
    case ATOM4('/m_-'): // benchmark voice
      synth_voice_bench(voice);
      break;
    case ATOM4('/q--'): // quit
      ctx->quit = -1;
      return 0;
    case ATOM4('/d--'): // data-to-wave slot rate rate offset
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = 44100.0;
        float offset = 0.0;
        if (argc) wave_slot = (int)arg[0];
        if (argc > 1) rate = arg[1];
        if (argc > 2) rate = arg[2];
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
      if (arg == 0) {
      } else {
        ands_string_from_external(ctx->parse, EXTRA_PTR(x), STRING_BUF_LEN);
      }
      break;
    case ATOM4('e>--'): // skode-string-to-external external-index
      if (arg == 0) {
      } else {
        char *s = ands_string(ctx->parse);
        //ands_string_to_extra(ctx->parse, x, s);
        //ands_string_to_external(ctx->parse, EXTRA_PTR(x), STRING_BUF_LEN);
        strncpy(EXTRA_PTR(x), s, STRING_BUF_LEN);
      }
      break;
    case ATOM4('e!--'): // execute-string num
      {
        char *s = "";
        if (arg == 0) {
          s = ands_string(ctx->parse);
        } else {
          s = _skode_extra[x % STRING_BUF_IDX_MAX];
        }
        if (s[0] != '\0') {
          uint64_t now = SAMPLE_COUNT_GET();
          int tag = 0;
          queue_item(now, s, voice, tag);
        }
      }
      break;
    case ATOM4('e?--'): // show-execute-string [num]
      if (arg) {
        ctx->printf(ctx, "# {%s} e>%d\n", EXTRA_PTR(x), x);
      } else {
        for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
          if (strlen(EXTRA_PTR(i)))
            ctx->printf(ctx, "# {%s} e>%d\n", EXTRA_PTR(i), i);
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
                  ctx->printf(ctx, "# {%s} e>%d\n", EXTRA_PTR(i), i);
              }
              break;
          }
        }
      }
      break;
    case ATOM4('/l--'): // skode-load num
      if (argc) { skode_load(ctx, voice, x); } break;
    case ATOM4('/w--'): // wave-load num wave channel
      {
        int file_num = 0;
        int wave_slot = EXT_SAMPLE_000;
        int ch = -1;
        if (argc >= 2) {
          file_num = (int)arg[0];
          wave_slot = (int)arg[1];
          if (argc > 2) ch = (int)arg[2];
        } else if (argc == 1) {
          file_num = (int)arg[0];
          wave_slot = EXT_SAMPLE_000;
        }
        if (argc) wave_load(ctx, file_num, wave_slot, ch, 1);
      }
      break;
    case ATOM4('>---'): // copy-voice dest-voice
      if (arg) voice_copy(voice, x);
      break;
    case ATOM4('/---'): // default-wave voice
      wave_default(voice);
      break;
    case ATOM4('=---'):  // variable-set slot value
      if (argc>1) ands_set_local(ctx->parse, x, arg[1]);
      else if (argc == 1) {
        double f = ands_get_local(ctx->parse, x);
        ctx->printf(ctx, "# $%d %g\n", x, f);
      }
      else {
        for (int i=0; i<10; i++) {
          double f = ands_get_local(ctx->parse, i);
          ctx->printf(ctx, "# $%d %g\n", i, f);
        }
      }
      break;
    case ATOM4('/wex'): // wave-expand wave
      if (argc && x >= 200 && x <=999) wave_table_dynamic_expand(x);
      break;
    default:
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
  ctx->printf(ctx, "# SKODE_UNKNOWN %d\n", info);
  return 0;
}

int skode_callback(ands_t *s, int info) {
  skode_t *ctx = (skode_t*)ands_user(s);
  switch (info) {
    case FUNCTION: return skode_function(s, info);
    case DEFER: return skode_defer(s, info);
    case CHUNK_END: return skode_chunk_end(s, info);
    case PUSH: { voice_push(&ctx->stack, ctx->voice); ctx->printf(ctx, "pushed v%d\n", ctx->voice); } break;
    case POP: { ctx->voice = voice_pop(&ctx->stack); } break;
    case GOT_STRING: { if (ctx->trace) ctx->printf(ctx, "# -> {%s}\n", ands_string(s)); } break;
    case GOT_ARRAY: { if (ctx->trace) ctx->printf(ctx, "# -> (..%d..)\n", ands_data_len(s)); } break;
    default: return skode_unknown(ctx, s, info);
  }
  return 0;
}

double global_var[10];


int skode_consume(char *line, skode_t *ctx) {
  if (ctx->parse == NULL) {
    // TODO this should live in wire-init or similar
    ctx->parse = ands_new(skode_callback, (void *)ctx);
    ands_set_global(ctx->parse, global_var);
  }
  ctx->log_len = 0;
  ctx->log[0] = '\0';
  skode_ctx[skode_hash(ctx)] = ctx;

  if (ctx->events) mpsc_queue_send(&mq, line);

  int r = 0;

  ands_consume(ctx->parse, line, skode_callback);
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
  ctx->events = 0;
  ctx->parse = NULL;
  ctx->quit = 0;
  ctx->puts = skode_puts;
  ctx->printf = skode_printf;
  ctx->log_enable = 0;
  ctx->log_max = SKODE_LOG_MAX;
  ctx->log_len = 0;
  ctx->log[0] = '\0';
}
