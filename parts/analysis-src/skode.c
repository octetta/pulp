#include "skred.h"
#include "api.h"
#include "skode.h"
#include "seq.h"
#include "miniwav.h"

#include "synth-types.h"
#include "synth.h"
#include "synth-state.h"
#include "synth-config.h"
#include "control-events.h"
#include "polyphony.h"
#include "skode-dict.h"
#include "midi.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <dirent.h>
#include "exp-vfs/skred_vfs.h"

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__WIN32) || defined(__WINDOWS__)
#define SKODE_WINDOWS_BUILD 1
#else
#define SKODE_WINDOWS_BUILD 0
#endif

char *skred_performance_status(void);

typedef enum {
  SKODE_ASSET_ANY = 0,
  SKODE_ASSET_SKODE,
  SKODE_ASSET_WAVE,
  SKODE_ASSET_KSYNTH
} skode_asset_kind_t;

static int skode_path_has_dir(const char *path) {
  return path && (strchr(path, '/') || strchr(path, '\\'));
}

static int skode_asset_try_read(const char *path, int real_only,
    void **data, size_t *size, char *resolved, size_t resolved_size) {
  if (!path || path[0] == '\0') return 0;
  if (real_only) {
    if (!skred_vfs_read_real_file(path, data, size)) return 0;
  } else {
    if (!skred_vfs_read_file(path, data, size)) return 0;
  }
  if (resolved && resolved_size > 0) {
    snprintf(resolved, resolved_size, "%s", path);
  }
  return 1;
}

static int skode_asset_read(const char *path, skode_asset_kind_t kind,
    void **data, size_t *size, char *resolved, size_t resolved_size) {
  char candidate[1024];
  int has_dir;
  const char *fallback_dir = NULL;

  if (!data || !size) return 0;
  *data = NULL;
  *size = 0;
  if (resolved && resolved_size > 0) resolved[0] = '\0';
  if (!path || path[0] == '\0') return 0;

  /*
   * Search order:
   *   mounted VFS, real cwd, type-specific dir in mounted VFS, then
   *   type-specific real subdir.
   * A file: prefix bypasses the mounted zip and reads the real filesystem.
   */
  if (strncmp(path, "file:", 5) == 0) {
    return skode_asset_try_read(path + 5, 1, data, size,
      resolved, resolved_size);
  }

  has_dir = skode_path_has_dir(path);
  if (skode_asset_try_read(path, 0, data, size, resolved, resolved_size))
    return 1;
  if (skode_asset_try_read(path, 1, data, size, resolved, resolved_size))
    return 1;

  if (has_dir) return 0;

  switch (kind) {
    case SKODE_ASSET_SKODE: fallback_dir = "sk"; break;
    case SKODE_ASSET_WAVE: fallback_dir = "wav"; break;
    case SKODE_ASSET_KSYNTH: fallback_dir = "ks"; break;
    default: break;
  }
  if (!fallback_dir) return 0;

  snprintf(candidate, sizeof(candidate), "%s/%s", fallback_dir, path);
  if (skode_asset_try_read(candidate, 0, data, size,
      resolved, resolved_size)) {
    return 1;
  }
  char vfs_candidate[1024];
  snprintf(vfs_candidate, sizeof(vfs_candidate), "/%s", candidate);
  if (skode_asset_try_read(vfs_candidate, 0, data, size,
      resolved, resolved_size)) {
    if (resolved && resolved_size > 0)
      snprintf(resolved, resolved_size, "%s", candidate);
    return 1;
  }
  if (skode_asset_try_read(candidate, 1, data, size, resolved, resolved_size))
    return 1;
  return 0;
}

#include "vendor/ksynth/ksynth.h"
#include "recorder.h"
#include "scope-ipc.h"

static void skode_log_reset(skode_t *ctx) {
  if (!ctx) return;
  ctx->log[0] = '\0';
  ctx->log_len = 0;
  ctx->log_head = 0;
  ctx->log_count = 0;
  ctx->log_dropped = 0;
  ctx->log_pending[0] = '\0';
  ctx->log_pending_len = 0;
}

static void skode_log_append_snapshot(skode_t *ctx, const char *s) {
  size_t capacity = sizeof(ctx->log);
  size_t used;
  int written;

  if (!ctx || !s || capacity == 0) return;
  used = strnlen(ctx->log, capacity);
  if (used >= capacity - 1) {
    ctx->log[capacity - 1] = '\0';
    ctx->log_len = (int)(capacity - 1);
    return;
  }

  written = snprintf(ctx->log + used, capacity - used, "%s", s);
  if (written > 0) ctx->log_len = (int)strnlen(ctx->log, capacity);
}

static void skode_log_snapshot(skode_t *ctx) {
  int oldest;

  if (!ctx) return;
  ctx->log[0] = '\0';
  ctx->log_len = 0;

  if (ctx->log_dropped > 0) {
    char dropped[64];
    snprintf(dropped, sizeof(dropped), "# log dropped %d line%s\n",
             ctx->log_dropped, ctx->log_dropped == 1 ? "" : "s");
    skode_log_append_snapshot(ctx, dropped);
  }

  oldest = ctx->log_head - ctx->log_count;
  if (oldest < 0) oldest += SKODE_LOG_LINES;
  for (int i = 0; i < ctx->log_count; i++) {
    int idx = (oldest + i) % SKODE_LOG_LINES;
    skode_log_append_snapshot(ctx, ctx->log_ring[idx]);
    skode_log_append_snapshot(ctx, "\n");
  }

  if (ctx->log_pending_len > 0) {
    skode_log_append_snapshot(ctx, ctx->log_pending);
  }
}

static void skode_log_push_line(skode_t *ctx, const char *line) {
  if (!ctx || !line) return;
  snprintf(ctx->log_ring[ctx->log_head], SKODE_LOG_LINE_MAX, "%s", line);
  ctx->log_head = (ctx->log_head + 1) % SKODE_LOG_LINES;
  if (ctx->log_count < SKODE_LOG_LINES) {
    ctx->log_count++;
  } else {
    ctx->log_dropped++;
  }
}

static void skode_log_flush_pending(skode_t *ctx) {
  if (!ctx) return;
  ctx->log_pending[ctx->log_pending_len] = '\0';
  skode_log_push_line(ctx, ctx->log_pending);
  ctx->log_pending[0] = '\0';
  ctx->log_pending_len = 0;
}

static void skode_log_write(skode_t *ctx, const char *s) {
  if (!ctx || !s || ctx->log_enable == 0) return;
  if (ctx->log_len == 0 && ctx->log[0] == '\0' &&
      (ctx->log_count || ctx->log_pending_len || ctx->log_dropped)) {
    skode_log_reset(ctx);
  }

  for (const char *p = s; *p; p++) {
    if (*p == '\n') {
      skode_log_flush_pending(ctx);
      continue;
    }

    if (ctx->log_pending_len >= SKODE_LOG_LINE_MAX - 1) {
      skode_log_flush_pending(ctx);
    }

    ctx->log_pending[ctx->log_pending_len++] = *p;
    ctx->log_pending[ctx->log_pending_len] = '\0';
  }

  skode_log_snapshot(ctx);
}

int skode_puts(skode_t *ctx, const char *s) {
  if (!ctx || !s || ctx->log_enable == 0) return 0;
  skode_log_write(ctx, s);
  skode_log_flush_pending(ctx);
  skode_log_snapshot(ctx);
  return 0;
}

int skode_printf(skode_t *ctx, const char *fmt, ...) {
  char buf[1024];
  va_list ap;
  va_list count_ap;
  int needed;

  if (!ctx || !fmt || ctx->log_enable == 0) return 0;
  va_start(ap, fmt);
  va_copy(count_ap, ap);
  needed = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (needed < 0) {
    va_end(count_ap);
    return 0;
  }
  if ((size_t)needed < sizeof(buf)) {
    skode_log_write(ctx, buf);
  } else {
    char *big = (char *)malloc((size_t)needed + 1);
    if (big) {
      vsnprintf(big, (size_t)needed + 1, fmt, count_ap);
      skode_log_write(ctx, big);
      free(big);
    }
  }
  va_end(count_ap);
  return 0;
}

void skode_log_message(skode_t *ctx, const char *message) {
  if (!ctx) return;
  skode_log_reset(ctx);
  if (message && message[0]) {
    size_t length = strlen(message);
    ctx->printf(ctx, "%s%s", message,
      length > 0 && message[length - 1] == '\n' ? "" : "\n");
  }
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
#include "portable_atomic.h"

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

static void record_tracks_show(skode_t *ctx) {
  for (int track = 1; track <= RECORD_TRACK_MAX; track++) {
    const char *name = synth_track_name_get(track);
    ctx->printf(ctx, "[%s] rt%d rv%d,%g #",
      name && name[0] ? name : "",
      track, track, synth_track_volume_db_get(track));
    for (int voice = 0; voice < synth_config.voice_max; voice++) {
      if (synth_record_track_get(voice) == track)
        ctx->printf(ctx, " v%d", voice);
    }
    ctx->puts(ctx, "");
  }
}

#define SKODE_CTX_MAX (100)
static skode_t *skode_ctx[SKODE_CTX_MAX];

#define STRING_BUF_LEN (256)
#define STRING_BUF_IDX_MAX SKODE_EXTRA_MAX // idea one macro per midi key?
static char _skode_extra[STRING_BUF_IDX_MAX][STRING_BUF_LEN];
static char _skode_extra_invalid[STRING_BUF_LEN];
static simple_mutex_t skode_extra_mutex;
static simple_mutex_t skode_ks_eval_mutex;
static atomic_int_t skode_global_state;
static int skode_extra_valid(int n) { return n >= 0 && n < STRING_BUF_IDX_MAX; }
static char *skode_extra_ptr(int n) {
  if (skode_extra_valid(n)) return _skode_extra[n];
  _skode_extra_invalid[0] = '\0';
  return _skode_extra_invalid;
}

static void skode_global_init(void) {
  int state = atomic_load_int(&skode_global_state);
  if (state == 2) return;

  int expected = 0;
  if (atomic_compare_exchange_int(&skode_global_state, &expected, 1)) {
    simple_mutex_init(&skode_extra_mutex);
    simple_mutex_init(&skode_ks_eval_mutex);
    for (int i = 0; i < SKODE_CTX_MAX; i++) skode_ctx[i] = NULL;
    _skode_extra_invalid[0] = '\0';
    for (int i = 0; i < STRING_BUF_IDX_MAX; i++) _skode_extra[i][0] = '\0';
    atomic_store_int(&skode_global_state, 2);
    return;
  }

  while (atomic_load_int(&skode_global_state) != 2) {
  }
}

int skode_extra_copy(int index, char *dst, size_t dst_size) {
  if (!skode_extra_valid(index) || !dst || dst_size == 0) return -1;
  skode_global_init();
  simple_mutex_lock(&skode_extra_mutex);
  snprintf(dst, dst_size, "%s", _skode_extra[index]);
  simple_mutex_unlock(&skode_extra_mutex);
  return 0;
}

static int skode_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

static int skode_wave_valid(int wave) {
  return wave >= 0 && wave < synth_config.wave_table_max;
}

int skode_double_to_int(double value, int *out) {
  if (!out || !isfinite(value) || value < INT_MIN || value > INT_MAX) return 0;
  *out = (int)value;
  return 1;
}

typedef struct {
  const char *key;
  const char *file;
  int line;
  const char *text;
} skode_doc_entry_t;

static const skode_doc_entry_t skode_doc_entries[] = {
#define KIT_DOC_BEGIN(symbol, key, file, line) { key, file, line, ""
#define KIT_DOC_LINE(symbol, text) text
#define KIT_DOC_END(symbol) },
#if defined(__has_include)
#if __has_include("skode-doc.inc")
#include "skode-doc.inc"
#endif
#endif
#undef KIT_DOC_BEGIN
#undef KIT_DOC_LINE
#undef KIT_DOC_END
  { NULL, NULL, 0, NULL }
};

#define SKODE_HELP_FIELD_MAX 96
#define SKODE_HELP_CATEGORY_MAX 64

static const char *skode_help_ltrim(const char *s) {
  while (s && *s && isspace((unsigned char)*s)) s++;
  return s ? s : "";
}

static void skode_help_copy_trimmed(char *dst, size_t dst_size,
                                    const char *src, size_t len) {
  size_t start = 0;
  if (!dst || dst_size == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  while (start < len && isspace((unsigned char)src[start])) start++;
  while (len > start && isspace((unsigned char)src[len - 1])) len--;
  len -= start;
  if (len >= dst_size) len = dst_size - 1;
  memcpy(dst, src + start, len);
  dst[len] = '\0';
}

static int skode_help_field(const skode_doc_entry_t *doc, const char *field,
                            char *out, size_t out_size) {
  const char *p;
  size_t field_len;
  if (!doc || !doc->text || !field || !out || out_size == 0) return 0;
  out[0] = '\0';
  field_len = strlen(field);
  for (p = doc->text; *p;) {
    const char *line = skode_help_ltrim(p);
    const char *end = strchr(line, '\n');
    size_t len = end ? (size_t)(end - line) : strlen(line);
    if (len > field_len && strncmp(line, field, field_len) == 0 &&
        line[field_len] == ':') {
      skode_help_copy_trimmed(out, out_size, line + field_len + 1,
                              len - field_len - 1);
      return out[0] != '\0';
    }
    p = end ? end + 1 : line + len;
  }
  return 0;
}

static int skode_help_is_command_doc(const skode_doc_entry_t *doc) {
  return doc && doc->key && strncmp(doc->key, "command.", 8) == 0;
}

static int skode_help_category_index(char categories[][SKODE_HELP_FIELD_MAX],
                                     int count, const char *category) {
  for (int i = 0; i < count; i++) {
    if (strcmp(categories[i], category) == 0) return i;
  }
  return -1;
}

static int skode_help_categories(char categories[][SKODE_HELP_FIELD_MAX],
                                 int max_categories) {
  int count = 0;
  for (int i = 0; skode_doc_entries[i].key; i++) {
    char category[SKODE_HELP_FIELD_MAX];
    if (!skode_help_is_command_doc(&skode_doc_entries[i])) continue;
    if (!skode_help_field(&skode_doc_entries[i], "category", category,
                          sizeof(category))) continue;
    if (skode_help_category_index(categories, count, category) >= 0) continue;
    if (count >= max_categories) break;
    snprintf(categories[count], SKODE_HELP_FIELD_MAX, "%s", category);
    count++;
  }
  return count;
}

static const skode_doc_entry_t *skode_help_doc_for_category_command(
    const char *category, int command_number) {
  int n = 0;
  if (!category || command_number <= 0) return NULL;
  for (int i = 0; skode_doc_entries[i].key; i++) {
    char doc_category[SKODE_HELP_FIELD_MAX];
    if (!skode_help_is_command_doc(&skode_doc_entries[i])) continue;
    if (!skode_help_field(&skode_doc_entries[i], "category", doc_category,
                          sizeof(doc_category))) continue;
    if (strcmp(doc_category, category) != 0) continue;
    n++;
    if (n == command_number) return &skode_doc_entries[i];
  }
  return NULL;
}

static void skode_help_show_doc(skode_t *ctx, const skode_doc_entry_t *doc) {
  char name[SKODE_HELP_FIELD_MAX] = "";
  char category[SKODE_HELP_FIELD_MAX] = "";
  char summary[SKODE_HELP_FIELD_MAX] = "";
  if (!ctx) return;
  if (!doc) {
    ctx->puts(ctx, "# help command not found");
    return;
  }
  skode_help_field(doc, "name", name, sizeof(name));
  skode_help_field(doc, "category", category, sizeof(category));
  skode_help_field(doc, "summary", summary, sizeof(summary));
  ctx->printf(ctx, "# help %s", name[0] ? name : doc->key);
  if (category[0]) ctx->printf(ctx, " [%s]", category);
  ctx->puts(ctx, "");
  if (summary[0]) ctx->printf(ctx, "#   %s\n", summary);
  if (doc->file) ctx->printf(ctx, "#   %s:%d\n", doc->file, doc->line);
}

static void skode_help_show_categories(skode_t *ctx) {
  char categories[SKODE_HELP_CATEGORY_MAX][SKODE_HELP_FIELD_MAX];
  int count = skode_help_categories(categories, SKODE_HELP_CATEGORY_MAX);
  ctx->puts(ctx, "# help categories");
  for (int i = 0; i < count; i++) {
    ctx->printf(ctx, "# %d %s\n", i + 1, categories[i]);
  }
  if (count == 0) ctx->puts(ctx, "# no embedded command docs");
}

static void skode_help_show_category(skode_t *ctx, int category_number) {
  char categories[SKODE_HELP_CATEGORY_MAX][SKODE_HELP_FIELD_MAX];
  int count = skode_help_categories(categories, SKODE_HELP_CATEGORY_MAX);
  const char *category;
  int command_number = 0;
  if (category_number <= 0 || category_number > count) {
    ctx->printf(ctx, "# help category %d not found\n", category_number);
    return;
  }
  category = categories[category_number - 1];
  ctx->printf(ctx, "# help %s\n", category);
  for (int i = 0; skode_doc_entries[i].key; i++) {
    char doc_category[SKODE_HELP_FIELD_MAX];
    char name[SKODE_HELP_FIELD_MAX] = "";
    char summary[SKODE_HELP_FIELD_MAX] = "";
    if (!skode_help_is_command_doc(&skode_doc_entries[i])) continue;
    if (!skode_help_field(&skode_doc_entries[i], "category", doc_category,
                          sizeof(doc_category))) continue;
    if (strcmp(doc_category, category) != 0) continue;
    command_number++;
    skode_help_field(&skode_doc_entries[i], "name", name, sizeof(name));
    skode_help_field(&skode_doc_entries[i], "summary", summary, sizeof(summary));
    ctx->printf(ctx, "# %d %s", command_number, name[0] ? name : skode_doc_entries[i].key);
    if (summary[0]) ctx->printf(ctx, " - %s", summary);
    ctx->puts(ctx, "");
  }
}

static void skode_help_lookup_string(skode_t *ctx, const char *query) {
  const skode_doc_entry_t *category_match = NULL;
  if (!query || !query[0]) {
    skode_help_show_categories(ctx);
    return;
  }
  for (int i = 0; skode_doc_entries[i].key; i++) {
    char name[SKODE_HELP_FIELD_MAX] = "";
    char category[SKODE_HELP_FIELD_MAX] = "";
    if (!skode_help_is_command_doc(&skode_doc_entries[i])) continue;
    skode_help_field(&skode_doc_entries[i], "name", name, sizeof(name));
    skode_help_field(&skode_doc_entries[i], "category", category, sizeof(category));
    if (strcmp(skode_doc_entries[i].key, query) == 0 ||
        (name[0] && strcmp(name, query) == 0)) {
      skode_help_show_doc(ctx, &skode_doc_entries[i]);
      return;
    }
    if (!category_match && category[0] && strcmp(category, query) == 0) {
      category_match = &skode_doc_entries[i];
    }
  }
  if (category_match) {
    char categories[SKODE_HELP_CATEGORY_MAX][SKODE_HELP_FIELD_MAX];
    char category[SKODE_HELP_FIELD_MAX] = "";
    int count = skode_help_categories(categories, SKODE_HELP_CATEGORY_MAX);
    skode_help_field(category_match, "category", category, sizeof(category));
    for (int i = 0; i < count; i++) {
      if (strcmp(categories[i], category) == 0) {
        skode_help_show_category(ctx, i + 1);
        return;
      }
    }
  }
  ctx->printf(ctx, "# help [%s] not found\n", query);
}

static void skode_help(skode_t *ctx, double *arg, int argc) {
  int category_number = 0;
  int command_number = 0;
  char categories[SKODE_HELP_CATEGORY_MAX][SKODE_HELP_FIELD_MAX];
  int category_count;
  if (ands_string_fresh(ctx->parse) && strlen(ands_string(ctx->parse)) > 0) {
    skode_help_lookup_string(ctx, ands_string(ctx->parse));
    return;
  }
  if (argc == 0 || !skode_double_to_int(arg[0], &category_number)) {
    skode_help_show_categories(ctx);
    return;
  }
  if (argc == 1 || !skode_double_to_int(arg[1], &command_number)) {
    skode_help_show_category(ctx, category_number);
    return;
  }
  category_count = skode_help_categories(categories, SKODE_HELP_CATEGORY_MAX);
  if (category_number <= 0 || category_number > category_count) {
    ctx->printf(ctx, "# help category %d not found\n", category_number);
    return;
  }
  skode_help_show_doc(ctx, skode_help_doc_for_category_command(
    categories[category_number - 1], command_number));
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

static void skode_format_string_args(char *dst, size_t dst_size,
                                     const char *fmt, double *arg, int argc) {
  size_t used = 0;

  if (!dst || dst_size == 0) return;
  dst[0] = '\0';
  if (!fmt) return;

  for (const char *p = fmt; *p && used + 1 < dst_size; p++) {
    if (*p == '@' && isdigit((unsigned char)p[1])) {
      int idx = p[1] - '0';
      if (idx < argc) {
        char num[32];
        int n = snprintf(num, sizeof(num), "%.8g", arg[idx]);
        if (n > 0) {
          size_t copy = (size_t)n;
          if (copy > dst_size - used - 1) copy = dst_size - used - 1;
          memcpy(dst + used, num, copy);
          used += copy;
          dst[used] = '\0';
        }
      } else if (used + 2 < dst_size) {
        dst[used++] = *p;
        dst[used++] = p[1];
        dst[used] = '\0';
      }
      p++;
    } else {
      dst[used++] = *p;
      dst[used] = '\0';
    }
  }
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
      ctx->puts(ctx, "");
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

static void skode_macros_show(skode_t *ctx, int pasteable) {
  char name[ANDS_MACRO_NAME_LEN];
  char body[ANDS_MACRO_BODY_LEN];
  int arg_count = 0;
  int count = ands_macro_count(ctx->parse);
  if (count == 0) {
    if (!pasteable) ctx->printf(ctx, "# macros empty\n");
  } else {
    for (int i = 0; i < count; i++) {
      if (ands_macro_get(ctx->parse, i, name, sizeof(name),
                         body, sizeof(body), &arg_count)) {
        int status = ands_macro_status(ctx->parse, i);
        const char *status_name =
          status == ANDS_MACRO_REALTIME ? "realtime" :
          status == ANDS_MACRO_IMMEDIATE ? "immediate" :
          status == ANDS_MACRO_INVALID ? "invalid" :
          status == ANDS_MACRO_TOO_LARGE ? "too-large" : "unchecked";
        if (pasteable) {
          ctx->printf(ctx, "[%s] :%s ;\n", name, body);
        } else {
          ctx->printf(ctx, "# [%s] :%s ; # @%d %s\n",
            name, body, arg_count, status_name);
        }
      }
    }
  }
}

static void wave_labels_show(skode_t *ctx) {
  for (int wave = 0; wave < synth_config.wave_table_max; wave++) {
    if (!sw.data[wave] || sw.size[wave] <= 0 || sw.readonly[wave])
      continue;
    if (sw.name[wave][0] != '\0')
      ctx->printf(ctx, "[%s] wt%d\n", sw.name[wave], wave);
    ctx->printf(ctx, "WL%d,%d,%d\n",
      wave, sw.loop_start[wave], sw.loop_end[wave]);
  }
}

void pattern_show(skode_t *ctx, int pattern_pointer, int verbose);

void global_status_show(skode_t *ctx, int full) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  ctx->printf(ctx, "# skred_version %s\n", skred_version());
  ctx->printf(ctx, "V%g\n", volume_get());
  ctx->printf(ctx, "M%g\n", tempo_bpm_get());
  ctx->printf(ctx, "# sample_rate %d voices %d waves %d\n",
    synth_sample_rate_get(), synth_config.voice_max, synth_config.wave_table_max);
  ctx->printf(ctx, "%s", delay_format());
  if (!full) return;

  ctx->printf(ctx, "# skred_text_state 1\n");
  ctx->printf(ctx, "# wavetable sample data is not embedded in this text snapshot\n");
  skode_macros_show(ctx, 1);
  wave_labels_show(ctx);
  record_tracks_show(ctx);
  voice_show_all(ctx, ctx->voice, 0);
  for (int pattern = 0; pattern < PATTERNS_MAX; pattern++)
    pattern_show(ctx, pattern, 1);
}


static const char *control_event_type_name(uint32_t type) {
  switch (type) {
    case SKRED_CONTROL_EVENT_VOICE_TRIGGER: return "VOICE_TRIGGER";
    case SKRED_CONTROL_EVENT_VOICE_RELEASE: return "VOICE_RELEASE";
    case SKRED_CONTROL_EVENT_VOICE_FINISHED: return "VOICE_FINISHED";
    case SKRED_CONTROL_EVENT_USER: return "USER";
    case SKRED_CONTROL_EVENT_PATTERN_START: return "PATTERN_START";
    case SKRED_CONTROL_EVENT_PATTERN_END: return "PATTERN_END";
    case SKRED_CONTROL_EVENT_MIDI: return "MIDI";
    default: return "UNKNOWN";
  }
}

static void control_event_show(skode_t *ctx, int consume) {
  skred_control_event_t events[128];
  int count = consume ?
    skred_control_event_poll(events, 128) :
    skred_control_event_snapshot(events, 128);
  if (count <= 0) {
    ctx->puts(ctx, "# control events empty");
    return;
  }
  ctx->printf(ctx, "# control events:%d dropped:%" PRIu64 "%s\n",
    count, skred_control_event_dropped(),
    consume ? "" : " snapshot");
  for (int i = 0; i < count; i++) {
    skred_control_event_t *event = &events[i];
    ctx->printf(ctx,
      "# control %02d seq:%" PRIu64 " type:%s sample:%" PRIu64
      " voice:%d pattern:%d step:%d tag:%d opcode:%s\n",
      i,
      event->sequence,
      control_event_type_name(event->type),
      event->sample,
      event->voice,
      event->pattern,
      event->step,
      event->tag,
      skode_opcode_name((uint8_t)event->opcode));
    if (event->type == SKRED_CONTROL_EVENT_USER) {
      ctx->printf(ctx, "#   id:%d", event->id);
      for (uint32_t a = 0; a < event->value_count && a < 3; a++)
        ctx->printf(ctx, " value%u:%g", (unsigned)a, event->value[a]);
      ctx->puts(ctx, "");
    }
  }
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

static void opcode_queue_show(skode_t *ctx) {
  int total = skred_scheduled_event_count();
  skred_scheduled_event_t events[128];
  int count = skred_scheduled_event_snapshot(events, 128);
  if (count < 0) {
    ctx->puts(ctx, "# opcode queue snapshot failed");
    return;
  }
  ctx->printf(ctx, "# opcode queue size:%d\n", total);
  int shown = count < 128 ? count : 128;
  for (int n = 0; n < shown; n++) {
    skred_scheduled_event_t *event = &events[n];
    uint64_t now = SAMPLE_COUNT_GET();
    double ms = event->timestamp >= now ?
      (double)(event->timestamp - now) * 1000.0 / MAIN_SAMPLE_RATE :
      -(double)(now - event->timestamp) * 1000.0 / MAIN_SAMPLE_RATE;
    ctx->printf(ctx, "# queue %02d id:%" PRIu64 " tag:%d at:%" PRIu64
      " %+.3fms voice:", event->index, event->id, event->tag,
      event->timestamp, ms);
    if (event->voice_var)
      ctx->printf(ctx, "$%u", (unsigned)event->voice_var - 1);
    else
      ctx->printf(ctx, "%d", event->voice);
    opcode_event_t opcode = {
      .code = event->opcode,
      .argc = event->opcode_argc,
      .mode = event->opcode_mode,
      .var_mask = event->opcode_var_mask,
    };
    for (int i = 0; i < SEQ_OPCODE_ARG_MAX; i++)
      opcode.arg[i] = event->opcode_arg[i];
    ctx->printf(ctx, " %s", skode_opcode_name(opcode.code));
    for (int i = 0; i < opcode.argc; i++)
      opcode_arg_show(ctx, &opcode, i);
    ctx->puts(ctx, "");
  }
  if (count > shown)
    ctx->printf(ctx, "# queue snapshot truncated: %d shown of %d\n",
      shown, count);
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
  seq_edit_lock();
  if (step >= 0) {
    if (step >= SEQ_STEPS_MAX) {
      ctx->printf(ctx, "# invalid opcode step:%d\n", step);
      seq_edit_unlock();
      return;
    }
    opcode_pattern_step_show(ctx, pattern, step);
    seq_edit_unlock();
    return;
  }
  ctx->printf(ctx, "# opcode pattern:%d length:%d\n",
    pattern, seq_pattern_length[pattern]);
  for (int s = 0; s < seq_pattern_length[pattern]; s++)
    opcode_pattern_step_show(ctx, pattern, s);
  seq_edit_unlock();
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

static ks_ctx *skode_ks_ctx(skode_t *ctx) {
  if (!ctx) return NULL;
  if (!ctx->ks) {
    ctx->ks = ks_create(16 * 1024 * 1024, 10000000);
    if (!ctx->ks) ctx->printf(ctx, "# ksynth context allocation failed\n");
  }
  return ctx->ks;
}

static void skode_ks_result_clear(skode_t *ctx) {
  if (!ctx || !ctx->ks_result) return;
  if (ctx->ks) k_free(ctx->ks, (K)ctx->ks_result);
  ctx->ks_result = NULL;
}

int skode_ks_eval(skode_t *ctx, char *cmd, int len) {
  if (!ctx || !cmd || len < 0) return 0;
  ks_ctx *ks = skode_ks_ctx(ctx);
  if (!ks) return 0;

  if (len >= 2 && strncmp(cmd, "\\X", 2) == 0) {
    skode_ks_result_clear(ctx);
    ks_clear_vars(ks);
    return 1;
  }

  simple_mutex_lock(&skode_ks_eval_mutex);
  K next = ks_eval(ks, cmd, (size_t)len);
  simple_mutex_unlock(&skode_ks_eval_mutex);
  if (ks->last_status != KS_OK) {
    ctx->printf(ctx, "# ksynth status %d error %s\n",
                ks->last_status, ks_strerror(ks->last_status));
  }
  skode_ks_result_clear(ctx);
  ctx->ks_result = next;
  return next != NULL;
}

int skode_ks_result_to_data(skode_t *ctx) {
  if (!ctx || !ctx->ks_result || k_is_func((K)ctx->ks_result)) return 0;
  K result = (K)ctx->ks_result;
  size_t len = (size_t)result->n;
  if (len) {
    int dlen = ands_data_cap(ctx->parse);
    if ((int)len > dlen) {
      ctx->printf(ctx, "# resize %d -> %d\n", dlen, (int)len);
      ands_data_resize(ctx->parse, (int)len);
    }
    double *g = ands_data(ctx->parse);
    for (int i=0; i<(int)len; i++) g[i] = result->f[i];
    ands_data_len_set(ctx->parse, (int)len);
  }
  return len > 0;
}

int skode_ks_bind_values(skode_t *ctx, int variable,
                         const double *values, size_t len) {
  if (variable < 0 || variable >= 26) {
    ctx->printf(ctx, "# invalid ksynth variable %d (expected 0..25)\n",
                variable);
    return 0;
  }
  if (!values || len == 0) {
    ctx->printf(ctx, "# no data to bind\n");
    return 0;
  }
  if (len > 1000000) {
    ctx->printf(ctx, "# ksynth vector too large: %zu\n", len);
    return 0;
  }
  ks_ctx *ks = skode_ks_ctx(ctx);
  if (!ks) return 0;
  ks_status status = ks_bind_vector(ks, (char)('A' + variable), values, len);
  if (status != KS_OK) {
    ctx->printf(ctx, "# bind %c status %d error %s\n",
                (char)('A' + variable), status, ks_strerror(status));
    return 0;
  }
  return 1;
}

void ksynth_loader(skode_t *ctx, const char *text, size_t text_len,
    const char *label, int verbose) {
  size_t pos = 0;
  (void)label;
  while (pos < text_len) {
    char line[1024];
    size_t start = pos;
    size_t len;
    while (pos < text_len && text[pos] != '\n' && text[pos] != '\r') pos++;
    len = pos - start;
    while (pos < text_len && (text[pos] == '\n' || text[pos] == '\r')) pos++;
    if (len >= sizeof(line)) len = sizeof(line) - 1;
    memcpy(line, text + start, len);
    line[len] = '\0';
    if (verbose) ctx->printf(ctx, "  %s\n", line);
    if (len > 0) skode_ks_eval(ctx, line, (int)len);
  }
}

int ksynth_load_name(skode_t *ctx, char *file, int verbose) {
  void *data = NULL;
  size_t size = 0;
  char resolved[1024];
  int r = 0;
  if (!skode_asset_read(file, SKODE_ASSET_KSYNTH, &data, &size,
      resolved, sizeof(resolved))) {
    ctx->printf(ctx, "# cannot load %s\n", file ? file : "(null)");
    return -1;
  }
  ksynth_loader(ctx, (const char *)data, size, resolved, verbose);
  skred_vfs_free_file(data);
  return r;
}

int ksynth_load(skode_t *ctx, int n, int verbose) {
  char file[1024];
  char resolved[1024];
  void *data = NULL;
  size_t size = 0;
  sprintf(file, "%d.ks", n);
  if (!skode_asset_read(file, SKODE_ASSET_KSYNTH, &data, &size,
      resolved, sizeof(resolved))) {
    ctx->printf(ctx, "# cannot load %d.ks or ks/%d.ks\n", n, n);
    return -1;
  }
  int r = 0;
  ksynth_loader(ctx, (const char *)data, size, resolved, verbose);
  skred_vfs_free_file(data);
  return r;
}

static int skode_load_buffer(skode_t *ctx, const char *text, size_t text_len,
    const char *label,
    int verbose) {
  int r = 0;
  skode_t *loader = (skode_t *)calloc(1, sizeof(*loader));
  if (!loader) {
    ctx->printf(ctx, "# cannot allocate patch loader\n");
    return -1;
  }
  skode_init(loader);
  if (text) {
    char line[1024];
    int line_no = 0;
    size_t pos = 0;
    while (pos < text_len) {
      size_t start = pos;
      size_t len;
      line_no++;
      while (pos < text_len && text[pos] != '\n' && text[pos] != '\r') pos++;
      len = pos - start;
      while (pos < text_len && (text[pos] == '\n' || text[pos] == '\r')) pos++;
      if (len >= sizeof(line)) len = sizeof(line) - 1;
      memcpy(line, text + start, len);
      line[len] = '\0';
      if (verbose) ctx->printf(ctx, "# %s # (%d)\n", line, line_no);
      r = skode_consume(line, loader);
      if (loader->log_len > 0) ctx->printf(ctx, "%s", loader->log);
      if (r != 0) {
        ctx->printf(ctx, "# error in patch %s:%d status=%d\n",
          label ? label : "(unknown)", line_no, r);
        break;
      }
    }
  } else {
    ctx->printf(ctx, "# cannot load %s\n", label ? label : "(null)");
    r = -1;
  }
  skode_free(loader);
  free(loader);
  return r;
}

int skode_load_name(skode_t *ctx, const char *name, int verbose) {
  skode_t caller_storage = SKODE_EMPTY();
  if (ctx == NULL) {
    ctx = &caller_storage;
    skode_init(ctx);
  }
  if (!name || name[0] == '\0') {
    ctx->printf(ctx, "# cannot load empty filename\n");
    return -1;
  }
  char file[1024];
  char resolved[1024];
  void *data = NULL;
  size_t size = 0;
  snprintf(file, sizeof(file), "%s", name);
  skode_asset_read(file, SKODE_ASSET_SKODE, &data, &size,
    resolved, sizeof(resolved));
  int r = skode_load_buffer(ctx, (const char *)data, size,
    resolved[0] ? resolved : file, verbose);
  skred_vfs_free_file(data);
  return r;
}

int skode_load(skode_t *ctx, int voice, int n, int verbose) {
  (void)voice;
  skode_t caller_storage = SKODE_EMPTY();
  if (ctx == NULL) {
    ctx = &caller_storage;
    skode_init(ctx);
  }
  char file[1024];
  char resolved[1024];
  void *data = NULL;
  size_t size = 0;
  sprintf(file, "%d.sk", n);
  skode_asset_read(file, SKODE_ASSET_SKODE, &data, &size,
    resolved, sizeof(resolved));
  int r = skode_load_buffer(ctx, (const char *)data, size,
    resolved[0] ? resolved : file, verbose);
  skred_vfs_free_file(data);
  return r;
}

extern synth_sample_t sampling;

static void wave_install_memory(int wave_slot, float *table, int len,
                                float rate, int one_shot, const char *name,
                                float midi_note, float offset_hz) {
  skode_copy_string(sw.name[wave_slot], WAVE_NAME_MAX, name ? name : "data");
  sw.is_heap[wave_slot] = 1;
  sw.data[wave_slot] = table;
  sw.size[wave_slot] = len;
  sw.rate[wave_slot] = rate;
  sw.one_shot[wave_slot] = one_shot != 0;
  sw.loop_enabled[wave_slot] = 0;
  sw.loop_start[wave_slot] = 0;
  sw.loop_end[wave_slot] = len;
  sw.direction[wave_slot] = 0.0f;
  sw.midi_note[wave_slot] = midi_note;
  sw.offset_hz[wave_slot] = offset_hz;
}

int rec_load(skode_t *ctx, int wave_slot, int one_shot, int channel) {
  ctx->printf(ctx, "# rec_load(ctx, %d, %d, %d)\n",
    wave_slot, one_shot, channel);
  if (!skode_wave_valid(wave_slot)) {
    ctx->printf(ctx, "# invalid slot %d\n", wave_slot);
    return -1;
  }
  if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
    ctx->printf(ctx, "# recording buffer is not complete\n");
    return -1;
  }
  if (!sampling.where || sampling.offset < 0 || sampling.trim < 0 ||
      sampling.offset > sampling.len ||
      sampling.trim > sampling.len - sampling.offset) {
    ctx->printf(ctx, "# invalid recording bounds\n");
    return -1;
  }
  int channels = sampling.channels == 2 ? 2 : 1;
  int data_len = sampling.len - sampling.offset - sampling.trim;
  if (data_len <= 0) {
    ctx->printf(ctx, "# no data len\n");
    return 100;
  }
  if (channel < -1 || channel >= channels) {
    ctx->printf(ctx, "# recording channel must be -1..%d\n", channels - 1);
    return -1;
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
  for (int i=0; i<data_len; i++) {
    size_t frame = (size_t)(sampling.offset + i) * (size_t)channels;
    if (channels == 1) {
      table[i] = sampling.where[frame];
    } else if (channel >= 0) {
      table[i] = sampling.where[frame + (size_t)channel];
    } else {
      table[i] = 0.5f * (sampling.where[frame] + sampling.where[frame + 1]);
    }
  }
  normalize_preserve_zero(table, data_len);
  int len = data_len;
    char wave_name[WAVE_NAME_MAX];
    snprintf(wave_name, sizeof(wave_name), "data[%d]", data_len);
    float offset_hz = one_shot
      ? (float)len / (float)MAIN_SAMPLE_RATE * 440.0f : 0.0f;
    wave_install_memory(wave_slot, table, len, (float)MAIN_SAMPLE_RATE,
      one_shot, wave_name, one_shot ? 69.0f : 0.0f, offset_hz);
    char *name = "data";
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:1)\n",
                data_len, name, wave_slot);
  return 0;
}

int data_load(skode_t *ctx, int wave_slot, int one_shot, float rate, float offset) {
  if (ctx == NULL) return 100; // fix todo
  ctx->printf(ctx, "# data_load(ctx, %d, %d, %g, %g)\n", wave_slot, one_shot, rate, offset);
  if (!skode_wave_valid(wave_slot)) {
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
    char wave_name[WAVE_NAME_MAX];
    snprintf(wave_name, sizeof(wave_name), "data[%d]", data_len);
    float offset_hz = offset > 0 ? (float)len / rate * 440.0f : 0.0f;
    wave_install_memory(wave_slot, table, len, rate, one_shot, wave_name,
      offset > 0 ? 69.0f : 0.0f, offset_hz);
    char *name = "data";
    int channels = 1;
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%g)\n", len, name, wave_slot, channels, rate);
  return 0;
}

static void wave_load_apply_smpl_loop(skode_t *ctx, const char *name,
                                      const void *data, size_t data_size,
                                      int wave_index, int len) {
  mw_smpl_loop_t loop;
  if (!mw_get_smpl_loop_mem(data, data_size, len, &loop)) return;
  sw.loop_enabled[wave_index] = 1;
  sw.loop_start[wave_index] = loop.start;
  sw.loop_end[wave_index] = loop.end;
  sw.direction[wave_index] = loop.type == 1 ? 2.0f :
    (loop.type == 2 ? 1.0f : 0.0f);
  ctx->printf(ctx, "# smpl loop %d..%d type:%d play:%d\n",
    loop.start, loop.end, loop.type, loop.play_count);
}

int wave_load_string(skode_t *ctx, char *name, int wave_index, int ch, int normalize) {
  (void)normalize;
  if (ctx == NULL) return 100; // fix todo
  if (!skode_wave_valid(wave_index)) {
    ctx->printf(ctx, "# invalid slot %d\n", wave_index);
    return -1;
  }
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
  void *data = NULL;
  size_t data_size = 0;
  char resolved[1024];
  if (!skode_asset_read(name, SKODE_ASSET_WAVE, &data, &data_size,
      resolved, sizeof(resolved))) {
    ctx->printf(ctx, "# cannot open %s\n", name);
    return -1;
  }
  wav_t wav;
  int len;
  char out[4096];
  float *table = mw_get_mem(data, data_size, resolved, &len, &wav, ch, out, sizeof(out));
  if (table == NULL) {
    ctx->printf(ctx, "# can not read %s\n", resolved);
    skred_vfs_free_file(data);
    return -1;
  } else {
    wave_install_memory(wave_index, table, len, (float)wav.SamplesRate, 1,
      resolved, 69.0f, (float)len / (float)wav.SamplesRate * 440.0f);
    wave_load_apply_smpl_loop(ctx, resolved, data, data_size, wave_index, len);
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%d)\n",
      len, resolved, wave_index, wav.Channels, wav.SamplesRate);
    normalize_preserve_zero(table, len);
  }
  skred_vfs_free_file(data);
  return 0;
}

int wave_try_open_number(int file_num, char *name, int len) {
  void *data = NULL;
  size_t size = 0;
  char resolved[1024];
  snprintf(name, len, "%d.wav", file_num);
  if (skode_asset_read(name, SKODE_ASSET_WAVE, &data, &size, resolved, sizeof(resolved))) {
    snprintf(name, len, "%s", resolved);
    skred_vfs_free_file(data);
    return 0;
  }
  snprintf(name, len, "%d.mp3", file_num);
  if (skode_asset_read(name, SKODE_ASSET_WAVE, &data, &size, resolved, sizeof(resolved))) {
    snprintf(name, len, "%s", resolved);
    skred_vfs_free_file(data);
    return 0;
  }
  snprintf(name, len, "%d.flac", file_num);
  if (skode_asset_read(name, SKODE_ASSET_WAVE, &data, &size, resolved, sizeof(resolved))) {
    snprintf(name, len, "%s", resolved);
    skred_vfs_free_file(data);
    return 0;
  }
  return -1;
}

int wave_load(skode_t *ctx, int file_num, int wave_index, int ch, int normalize) {
  (void)normalize;
  if (ctx == NULL) return 100; // fix todo
  if (!skode_wave_valid(wave_index)) {
    ctx->printf(ctx, "# invalid slot %d\n", wave_index);
    return -1;
  }
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
  void *data = NULL;
  size_t data_size = 0;
  char resolved[1024];
  if (!skode_asset_read(name, SKODE_ASSET_WAVE, &data, &data_size,
      resolved, sizeof(resolved))) {
    ctx->printf(ctx, "# cannot open %s\n", name);
    return -1;
  }
  wav_t wav;
  int len;
  char out[4096];
  float *table = mw_get_mem(data, data_size, resolved, &len, &wav, ch, out, sizeof(out));
  if (table == NULL) {
    ctx->printf(ctx, "# can not read %s\n", resolved);
    skred_vfs_free_file(data);
    return -1;
  } else {
    wave_install_memory(wave_index, table, len, (float)wav.SamplesRate, 1,
      resolved, 69.0f, (float)len / (float)wav.SamplesRate * 440.0f);
    wave_load_apply_smpl_loop(ctx, resolved, data, data_size, wave_index, len);
    ctx->printf(ctx, "# read %d frames from %s to %d (ch:%d sr:%d)\n",
      len, resolved, wave_index, wav.Channels, wav.SamplesRate);
    normalize_preserve_zero(table, len);
  }
  skred_vfs_free_file(data);
  return 0;
}


void pattern_show(skode_t *ctx, int pattern_pointer, int verbose) {
  if (pattern_pointer < 0 || pattern_pointer >= PATTERNS_MAX) return;
  seq_edit_lock();
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
      if (seq_control_events[pattern_pointer]) ctx->printf(ctx, " yc1");
      if (seq_text[pattern_pointer][0] != '\0') ctx->printf(ctx, " [%s] yt", seq_text[pattern_pointer]);
      ctx->puts(ctx, "");
      first = 0;
      if (verbose == 0) break;
    }
    ctx->printf(ctx, "[%s] x%d", line, s);
    ctx->puts(ctx, "");
  }
  seq_edit_unlock();
}


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
#define RECORD_TRIM_DEFAULT_THRESHOLD 0.001f
#define RECORD_TRIM_CONSECUTIVE_SAMPLES 4

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

static float wave_samples_to_ms(int samples, float rate) {
    if (samples <= 0 || rate <= 0.0f) return 0.0f;
    return (float)samples / rate * 1000.0f;
}

static int wave_boundary_col(int boundary, int n, int width) {
    if (width <= 1 || n <= 0) return 0;
    if (boundary < 0) boundary = 0;
    if (boundary > n) boundary = n;
    return (int)(((long long)boundary * (long long)(width - 1) + (n / 2)) / n);
}

static void print_braille_cell(skode_t *ctx, unsigned int pattern) {
    ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | ((pattern >> 6) & 0x03),
                0x80 | (pattern & 0x3F));
}

static void print_wave_marker_row(skode_t *ctx, int n, int width, int start, int end,
                                  int braille) {
    if (!ctx || n <= 0 || width <= 0) return;
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > n) start = n;
    if (end > n) end = n;
    if (end < start) {
        int tmp = end;
        end = start;
        start = tmp;
    }

    int col_start = wave_boundary_col(start, n, width);
    int col_end = wave_boundary_col(end, n, width);
    if (col_end < col_start) {
        int tmp = col_end;
        col_end = col_start;
        col_start = tmp;
    }

    if (braille) print_braille_cell(ctx, 0xFF); /* physical start */
    else ctx->printf(ctx, "|");
    for (int x = 0; x < width; x++) {
        if (braille) {
            unsigned int pattern = 0;
            if (x >= col_start && x <= col_end) pattern |= 0x12; /* dots 2,5 */
            if (x == col_start) pattern |= 0x47; /* left column */
            if (x == col_end) pattern |= 0xB8;   /* right column */
            print_braille_cell(ctx, pattern);
        } else {
            char ch = ' ';
            if (x >= col_start && x <= col_end) ch = '-';
            if (x == col_start) ch = '[';
            if (x == col_end) ch = ']';
            if (col_start == col_end && x == col_start) ch = '|';
            ctx->printf(ctx, "%c", ch);
        }
    }
    if (braille) print_braille_cell(ctx, 0xFF); /* physical end */
    else ctx->printf(ctx, "|");
    ctx->puts(ctx, "");
}

static int skode_env_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int skode_wave_display_use_braille(void) {
    const char *mode = getenv("SKRED_WAVE_DISPLAY");
    if (mode && mode[0]) {
        if (skode_env_eq(mode, "braille") ||
            skode_env_eq(mode, "unicode") ||
            skode_env_eq(mode, "utf8") ||
            skode_env_eq(mode, "1")) {
            return 1;
        }
        if (skode_env_eq(mode, "ascii") ||
            skode_env_eq(mode, "plain") ||
            skode_env_eq(mode, "0")) {
            return 0;
        }
    }
#if SKODE_WINDOWS_BUILD
    return 0;
#else
    return 1;
#endif
}

static const char *skode_wave_display_name(void) {
    return skode_wave_display_use_braille() ? "braille" : "ascii";
}

static int wave_y_from_value(float value, float max_abs, int rows) {
    int y = (int)((value / max_abs + 1.0f) * 0.5f * (float)(rows - 1));
    if (y < 0) y = 0;
    if (y >= rows) y = rows - 1;
    return y;
}

static int wave_ascii_points(float *data, int n, int width, int rows, int *y_coords, int *y_peak_min, int *y_peak_max) {
    if (!data || n <= 0 || width <= 0 || rows <= 0) return 0;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    for (int x = 0; x < width; x++) {
        float val = data[0];
        if (n > 1 && width > 1) {
            float data_pos = (float)x * (float)(n - 1) / (float)(width - 1);
            int idx = (int)data_pos;
            float fract = data_pos - (float)idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx + 1] * fract;
        }
        y_coords[x] = wave_y_from_value(val, max_abs, rows);

        int start = (int)((long long)x * n / width);
        int end = (int)((long long)(x + 1) * n / width);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = wave_y_from_value(bucket_min, max_abs, rows);
        y_peak_max[x] = wave_y_from_value(bucket_max, max_abs, rows);
    }
    return 1;
}

static void print_audio_ascii_wave(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim, int labeled) {
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    int width = width_chars;
    int rows = height_chars * 2;
    if (rows < 1) rows = 1;
    int zero_y = (rows - 1) / 2;

    int *y_coords = (int *)malloc(width * sizeof(int));
    int *y_peak_min = (int *)malloc(width * sizeof(int));
    int *y_peak_max = (int *)malloc(width * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }
    if (!wave_ascii_points(data, n, width, rows, y_coords, y_peak_min, y_peak_max)) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }

    for (int y = rows - 1; y >= 0; y--) {
        ctx->printf(ctx, ":");
        for (int x = 0; x < width; x++) {
            int y_curr = y_coords[x];
            int y_prev = (x > 0) ? y_coords[x - 1] : y_curr;
            int lo = (y_curr < y_prev) ? y_curr : y_prev;
            int hi = (y_curr > y_prev) ? y_curr : y_prev;
            char ch = ' ';

            if (y >= lo && y <= hi) ch = '*';
            if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y == y_peak_min[x] && ch == ' ') ch = ':';
            if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y == y_peak_max[x] && ch == ' ') ch = ':';
            if (y == zero_y && ch == ' ') ch = '-';

            ctx->printf(ctx, "%c", ch);
        }
        ctx->printf(ctx, ":\n");
    }

    if (labeled) {
        print_wave_marker_row(ctx, n, width, offset, trim, 0);
    }

    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

/**
 * Prints a connected audio waveform using Braille patterns.
 * Draws a single connected trace.
 */
void print_audio_braille_connected(skode_t *ctx, float *data, int n, int width_chars, int height_chars) {
    if (!skode_wave_display_use_braille()) {
        print_audio_ascii_wave(ctx, data, n, width_chars, height_chars, 0, n - 1, 0);
        return;
    }
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
    if (!skode_wave_display_use_braille()) {
        print_audio_ascii_wave(ctx, data, n, width_chars, height_chars, offset, trim, 1);
        return;
    }
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

    print_wave_marker_row(ctx, n, width_chars, offset, trim, 1);

    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

#define WTWFS(ms) if ((ms) >= 1000.0f) { \
  ctx->printf(ctx, " %gsec", (ms)/1000.0f); \
  } else { ctx->printf(ctx, " %gmsec", (ms)); }

int wavetable_show(skode_t *ctx, int n) {
  if (skode_wave_valid(n) && sw.data[n] && sw.size[n]) {
    int readonly = sw.readonly[n];
    int refcount = sw.refcount[n];
    int size = sw.size[n];
    int loop_start = sw.loop_start[n];
    int loop_end = sw.loop_end[n];
    if (loop_start < 0) loop_start = 0;
    if (loop_end < loop_start) loop_end = loop_start;
    if (loop_end > size) loop_end = size;
    int loop_len = loop_end - loop_start;
    float rate = sw.rate[n] > 0.0f ? sw.rate[n] : (float)MAIN_SAMPLE_RATE;
    wave_stats_t stats = wave_stats(sw.data[n], size);
    ctx->printf(ctx, "# W%d", n);
    WTWFS(wave_samples_to_ms(size, rate));
    if (readonly) ctx->printf(ctx, " R/O"); else ctx->printf(ctx, " R/W");
    ctx->printf(ctx, " ref#%d", refcount);
    ctx->printf(ctx, " [%s]", sw.name[n]);
    ctx->puts(ctx, "");
    ctx->printf(ctx, "# playback rate %gHz offset %+gHz MIDI %g mode %s\n",
      sw.rate[n], sw.offset_hz[n], sw.midi_note[n],
      sw.one_shot[n] ? "one-shot" : "cycle");
    // ctx->printf(ctx, "# loop %d..%d |%d| %gms\n", loop_start, loop_end, loop_len, wave_samples_to_ms(loop_len, rate));
    ctx->printf(ctx,
      "# min %+0.3f max %+0.3f peak %0.3f rms %0.3f dc %+0.4f zc %d",
      stats.min, stats.max, stats.peak, stats.rms, stats.dc, stats.zero_crossings);
    if (stats.clipped) ctx->printf(ctx, " clip %d", stats.clipped);
    ctx->puts(ctx, "");
  }
  return 0;
}

static void wavetable_waveform_show(skode_t *ctx, int wave, int width, int height,
                                    int loop_start, int loop_end,
                                    const char *label) {
  if (!skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) return;
  wavetable_show(ctx, wave);
  if (label && label[0]) ctx->printf(ctx, "# %s\n", label);
  print_audio_braille_labeled(ctx, sw.data[wave], sw.size[wave], width, height,
    loop_start, loop_end);
  ctx->printf(ctx, "# wave [0..%d)", sw.size[wave]);
  double rate = sw.rate[wave] > 0.0f ? sw.rate[wave] : (float)MAIN_SAMPLE_RATE;
  double ms = wave_samples_to_ms(sw.size[wave], rate);
  WTWFS(ms);
  ctx->printf(ctx, "\n# loop [%d..%d)", loop_start, loop_end);
  double lms = wave_samples_to_ms(loop_end - loop_start, rate);
  WTWFS(lms);
  ctx->puts(ctx, "");
}


/* Tiny iterative radix-2 Cooley-Tukey FFT, in place. n must be a power of two. */
typedef struct { float re, im; } spectro_cplx_t;

static void spectro_fft(spectro_cplx_t *buf, int n) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { spectro_cplx_t tmp = buf[i]; buf[i] = buf[j]; buf[j] = tmp; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -6.28318530717958647692f / (float)len;
        spectro_cplx_t wlen = { cosf(ang), sinf(ang) };
        for (int i = 0; i < n; i += len) {
            spectro_cplx_t w = { 1.0f, 0.0f };
            for (int k = 0; k < len / 2; k++) {
                spectro_cplx_t u = buf[i + k];
                spectro_cplx_t t = buf[i + k + len / 2];
                spectro_cplx_t v = { t.re * w.re - t.im * w.im, t.re * w.im + t.im * w.re };
                buf[i + k].re         = u.re + v.re;
                buf[i + k].im         = u.im + v.im;
                buf[i + k + len/2].re = u.re - v.re;
                buf[i + k + len/2].im = u.im - v.im;
                float nwre = w.re * wlen.re - w.im * wlen.im;
                float nwim = w.re * wlen.im + w.im * wlen.re;
                w.re = nwre; w.im = nwim;
            }
        }
    }
}

static int spectro_next_pow2(int x) {
    int p = 1;
    while (p < x) p <<= 1;
    return p;
}

/* SKRED_SPECTROGRAM_COLOR: "256" (default) -- the 6x6x6 cube is plenty of
   steps for a heat ramp at glyph size and costs half the bytes of truecolor.
   "truecolor"/"24bit" opts into full RGB if you want it and can afford the
   line length. "none"/"mono"/"0" disables color. */
static int skode_spectrogram_color_mode(void) {
    const char *mode = getenv("SKRED_SPECTROGRAM_COLOR");
    if (mode && mode[0]) {
        if (skode_env_eq(mode, "none") || skode_env_eq(mode, "mono") || skode_env_eq(mode, "0")) return 0;
        if (skode_env_eq(mode, "256")) return 1;
        if (skode_env_eq(mode, "truecolor") || skode_env_eq(mode, "24bit") || skode_env_eq(mode, "1")) return 2;
    }
    if (getenv("NO_COLOR")) return 0;
#if SKODE_WINDOWS_BUILD
    return 0;
#else
    return 1;
#endif
}

/* Inferno-ish heat ramp: t in [0,1] -> RGB */
static void spectro_heat_rgb(float t, int *r, int *g, int *b) {
    static const float stops[5][3] = {
        {0.0f,   0.0f,   4.0f},
        {87.0f,  16.0f,  110.0f},
        {188.0f, 55.0f,  84.0f},
        {249.0f, 142.0f, 8.0f},
        {252.0f, 255.0f, 164.0f}
    };
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float pos = t * 4.0f;
    int i0 = (int)pos;
    if (i0 > 3) i0 = 3;
    int i1 = i0 + 1;
    float frac = pos - (float)i0;
    *r = (int)(stops[i0][0] + (stops[i1][0] - stops[i0][0]) * frac);
    *g = (int)(stops[i0][1] + (stops[i1][1] - stops[i0][1]) * frac);
    *b = (int)(stops[i0][2] + (stops[i1][2] - stops[i0][2]) * frac);
}

static void spectro_reset_color(skode_t *ctx, int mode) {
    if (mode) ctx->printf(ctx, "\x1b[0m");
}

/* 4x4 Bayer matrix, used to dither magnitude into on/off dots so the
   braille cells carry fine texture on top of the per-cell color. */
static const float spectro_bayer4[4][4] = {
    { 0.0f/16,  8.0f/16,  2.0f/16, 10.0f/16},
    {12.0f/16,  4.0f/16, 14.0f/16,  6.0f/16},
    { 3.0f/16, 11.0f/16,  1.0f/16,  9.0f/16},
    {15.0f/16,  7.0f/16, 13.0f/16,  5.0f/16}
};

#define SPECTRO_NOISE_FLOOR_DB -60.0f

/* Conservative worst-case row length in bytes if EVERY cell happened to
   change color (never actually true after quantization, but cheap and
   safe to assume for sizing purposes). Set SPECTRO_LOG_LINE_BUDGET to a
   little under your actual SKODE_LOG_LINE_MAX (see skode.h), or override
   per-run with SKRED_SPECTROGRAM_LINE_BUDGET, if you want more headroom. */
#ifndef SPECTRO_LOG_LINE_BUDGET
#define SPECTRO_LOG_LINE_BUDGET 400
#endif

static int skode_spectrogram_line_budget(void) {
    const char *v = getenv("SKRED_SPECTROGRAM_LINE_BUDGET");
    if (v && v[0]) {
        int n = atoi(v);
        if (n > 0) return n;
    }
    return SPECTRO_LOG_LINE_BUDGET;
}

static int spectro_row_worst_case_bytes(int mode, int width, int use_braille) {
    int glyph_bytes = use_braille ? width * 3 + 2 : width; /* +2 for ':' borders */
    int per_cell_escape = (mode == 2) ? 20 : (mode == 1 ? 12 : 0); /* "\x1b[38;2;255;255;255m" / "\x1b[38;5;231m" */
    int color_bytes = mode ? width * per_cell_escape + 4 /* trailing reset */ : 0;
    return glyph_bytes + color_bytes;
}

/* Steps color_mode down (2->1->0) until the worst-case row estimate fits
   the budget. Prints one short note on the way down so a truncated/garbled
   display isn't a silent mystery. */
static int spectro_fit_color_mode(skode_t *ctx, int color_mode, int width, int use_braille) {
    int budget = skode_spectrogram_line_budget();
    int original = color_mode;
    while (color_mode > 0 && spectro_row_worst_case_bytes(color_mode, width, use_braille) > budget) {
        color_mode--;
    }
    if (color_mode != original) {
        ctx->printf(ctx, "# spectrogram: width %d too wide for %s at line budget %d, using %s\n",
                    width,
                    original == 2 ? "truecolor" : "256-color",
                    budget,
                    color_mode == 1 ? "256-color" : "mono");
    }
    return color_mode;
}

/* Same prototype as wavetable_waveform_show(). loop_start/loop_end are
   drawn as a marker bar under the plot, same as the waveform view. */
static void wavetable_spectrogram_show(skode_t *ctx, int wave, int width, int height,
                                        int loop_start, int loop_end,
                                        const char *label) {
    if (!skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) return;

    float *data = sw.data[wave];
    int n = sw.size[wave];
    int use_braille = skode_wave_display_use_braille();
    int color_mode = skode_spectrogram_color_mode();
    color_mode = spectro_fit_color_mode(ctx, color_mode, width, use_braille);

    wavetable_show(ctx, wave);
    if (label && label[0]) ctx->printf(ctx, "# %s\n", label);

    if (width <= 0 || height <= 0) return;

    int total_dots_x = use_braille ? width * 2 : width;
    int total_dots_y = use_braille ? height * 4 : height;

    int fft_size = spectro_next_pow2(total_dots_y * 2);
    if (fft_size < 64) fft_size = 64;
    if (fft_size > 4096) fft_size = 4096;
    int half = fft_size / 2;

    float *win = (float *)malloc((size_t)fft_size * sizeof(float));
    spectro_cplx_t *buf = (spectro_cplx_t *)malloc((size_t)fft_size * sizeof(spectro_cplx_t));
    float *mag = (float *)malloc((size_t)total_dots_x * total_dots_y * sizeof(float));
    if (!win || !buf || !mag) {
        free(win); free(buf); free(mag);
        return;
    }

    for (int i = 0; i < fft_size; i++) {
        win[i] = 0.5f - 0.5f * cosf(6.28318530717958647692f * i / (fft_size - 1));
    }

    int hop = total_dots_x > 0 ? n / total_dots_x : n;
    if (hop < 1) hop = 1;

    float max_mag = 1e-9f;
    for (int x = 0; x < total_dots_x; x++) {
        int center = x * hop + hop / 2;
        int start = center - fft_size / 2;
        for (int i = 0; i < fft_size; i++) {
            int idx = start + i;
            float s = (idx >= 0 && idx < n) ? data[idx] : 0.0f;
            buf[i].re = s * win[i];
            buf[i].im = 0.0f;
        }
        spectro_fft(buf, fft_size);
        for (int y = 0; y < total_dots_y; y++) {
            int bin = (int)(((float)y + 0.5f) * half / total_dots_y);
            if (bin >= half) bin = half - 1;
            float re = buf[bin].re, im = buf[bin].im;
            float m = sqrtf(re * re + im * im);
            mag[y * total_dots_x + x] = m;
            if (m > max_mag) max_mag = m;
        }
    }

    /* normalize to 0..1 using dB scale against the loudest bin found */
    for (int i = 0; i < total_dots_x * total_dots_y; i++) {
        float db = 20.0f * log10f(mag[i] / max_mag + 1e-9f);
        float t = (db - SPECTRO_NOISE_FLOOR_DB) / (0.0f - SPECTRO_NOISE_FLOOR_DB);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        mag[i] = t;
    }

    static const char ascii_ramp[] = " .:-=+*#%@";
    int ascii_levels = (int)sizeof(ascii_ramp) - 2;

    for (int r = height - 1; r >= 0; r--) {
        /* Track the last color actually emitted so we only send an escape
           code on a real change, and reset once at end-of-row instead of
           after every cell. Emitting set+reset per cell can blow a colored
           row up to ~10x the byte length of the plain glyphs, which is
           enough to overrun a fixed-size line buffer in some logging/
           console layers (they'll force-split or truncate long lines,
           often mid-escape-sequence, which is what corrupted output looks
           like). Coalescing keeps escape codes down to one per color
           transition instead of one per cell. */
        int last_r = -1, last_g = -1, last_b = -1, last_idx = -1;
        int row_has_color = 0;

        if (use_braille) ctx->printf(ctx, ":");
        for (int c = 0; c < width; c++) {
            if (use_braille) {
                unsigned int pattern = 0;
                float cell_sum = 0.0f;
                int cell_n = 0;
                for (int dx = 0; dx < 2; dx++) {
                    int x = c * 2 + dx;
                    for (int dy = 0; dy < 4; dy++) {
                        int y = r * 4 + dy;
                        float level = mag[y * total_dots_x + x];
                        cell_sum += level;
                        cell_n++;
                        float threshold = spectro_bayer4[y % 4][x % 4];
                        if (level > threshold * 0.6f) {
                            static const unsigned char masks[2][4] = {
                                {0x40, 0x04, 0x02, 0x01},
                                {0x80, 0x20, 0x10, 0x08}
                            };
                            pattern |= masks[dx][dy];
                        }
                    }
                }
                float cell_level = cell_n ? cell_sum / cell_n : 0.0f;
                if (color_mode && pattern) {
                    /* Quantize to 20 steps (5% granularity) so visually-flat
                       regions actually repeat the same RGB triple instead of
                       drifting by a rounding error every cell -- that's what
                       lets the "only emit on change" logic above coalesce
                       runs into one escape code instead of dozens. */
                    float cell_level_q = roundf(cell_level * 20.0f) / 20.0f;
                    int r8, g8, b8;
                    spectro_heat_rgb(cell_level_q, &r8, &g8, &b8);
                    if (color_mode == 2) {
                        if (r8 != last_r || g8 != last_g || b8 != last_b) {
                            ctx->printf(ctx, "\x1b[38;2;%d;%d;%dm", r8, g8, b8);
                            last_r = r8; last_g = g8; last_b = b8;
                        }
                    } else {
                        int ri = r8 * 5 / 255, gi = g8 * 5 / 255, bi = b8 * 5 / 255;
                        int idx = 16 + 36 * ri + 6 * gi + bi;
                        if (idx != last_idx) {
                            ctx->printf(ctx, "\x1b[38;5;%dm", idx);
                            last_idx = idx;
                        }
                    }
                    row_has_color = 1;
                }
                print_braille_cell(ctx, pattern);
            } else {
                float level = mag[r * total_dots_x + c];
                int idx = (int)(level * ascii_levels + 0.5f);
                if (idx < 0) idx = 0;
                if (idx > ascii_levels) idx = ascii_levels;
                if (color_mode && idx > 0) {
                    float level_q = roundf(level * 20.0f) / 20.0f;
                    int r8, g8, b8;
                    spectro_heat_rgb(level_q, &r8, &g8, &b8);
                    if (color_mode == 2) {
                        if (r8 != last_r || g8 != last_g || b8 != last_b) {
                            ctx->printf(ctx, "\x1b[38;2;%d;%d;%dm", r8, g8, b8);
                            last_r = r8; last_g = g8; last_b = b8;
                        }
                    } else {
                        int ri = r8 * 5 / 255, gi = g8 * 5 / 255, bi = b8 * 5 / 255;
                        int cidx = 16 + 36 * ri + 6 * gi + bi;
                        if (cidx != last_idx) {
                            ctx->printf(ctx, "\x1b[38;5;%dm", cidx);
                            last_idx = cidx;
                        }
                    }
                    row_has_color = 1;
                }
                ctx->printf(ctx, "%c", ascii_ramp[idx]);
            }
        }
        if (row_has_color) spectro_reset_color(ctx, color_mode);
        if (use_braille) ctx->printf(ctx, ":");
        ctx->printf(ctx, "\n");
    }

    print_wave_marker_row(ctx, n, width, loop_start, loop_end, use_braille);

    free(win);
    free(buf);
    free(mag);

    ctx->printf(ctx, "# spectrogram [0..%d) fft=%d", n, fft_size);
    double rate = sw.rate[wave] > 0.0f ? sw.rate[wave] : (float)MAIN_SAMPLE_RATE;
    double ms = wave_samples_to_ms(n, rate);
    WTWFS(ms);
    ctx->printf(ctx, " floor %gdB\n", SPECTRO_NOISE_FLOOR_DB);
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

static int skode_sample_alloc(int frames) {
  if (frames <= 0 || frames > INT_MAX / AUDIO_CHANNELS) {
    return 0;
  }
  if (sampling.capacity < frames) {
    size_t samples = (size_t)frames * AUDIO_CHANNELS;
    float *where = (float *)calloc(samples, sizeof(float));
    if (!where) {
      return 0;
    }
    free(sampling.where);
    sampling.where = where;
    sampling.capacity = frames;
  }
  return sampling.where != NULL;
}

int skode_sample_go(int frames, int source, int voice) {
  int state = atomic_load_int(&sampling.state);
  if (state == SAMPLE_STATE_ARMED || state == SAMPLE_STATE_RECORDING) {
    return 0;
  }
  if (!skode_sample_alloc(frames)) return 0;
  sampling.source = source;
  sampling.source_voice = voice;
  sampling.channels = source == SAMPLE_SOURCE_MASTER ? 2 : 1;
  atomic_store_int(&sampling.frames, frames);
  atomic_store_int(&sampling.state, SAMPLE_STATE_ARMED);
  return 1;
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

static int skode_write_wav(skode_t *ctx, const char *filename,
                           const float *samples, int frames,
                           ma_uint32 channels, ma_uint32 sample_rate,
                           int normalize) {
    ma_encoder_config config;
    ma_encoder encoder;
    const float *where = samples;
    float *copy = NULL;
    ma_uint64 frames_written = 0;
    ma_result result;

    if (!filename || !filename[0]) {
        ctx->printf(ctx, "# WAV output requires [filename]\n");
        return 0;
    }
    if (!samples || frames <= 0) {
        ctx->printf(ctx, "# no samples to write to %s\n", filename);
        return 0;
    }
    if (channels < 1 || channels > AUDIO_CHANNELS) {
        ctx->printf(ctx, "# invalid WAV channel count\n");
        return 0;
    }
    if (sample_rate == 0) sample_rate = MAIN_SAMPLE_RATE;
    if (normalize) {
        size_t sample_count = (size_t)frames * channels;
        copy = (float *)malloc(sample_count * sizeof(float));
        if (!copy) {
            ctx->printf(ctx, "# allocation failed writing %s\n", filename);
            return 0;
        }
        memcpy(copy, samples, sample_count * sizeof(float));
        normalize_buffer(copy, (ma_uint32)frames, channels);
        where = copy;
    }

    config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32,
                                    channels, sample_rate);
    result = ma_encoder_init_file(filename, &config, &encoder);
    if (result != MA_SUCCESS) {
        ctx->printf(ctx, "# cannot open %s\n", filename);
        free(copy);
        return 0;
    }
    result = ma_encoder_write_pcm_frames(&encoder, where, (ma_uint64)frames,
                                         &frames_written);
    ma_encoder_uninit(&encoder);
    free(copy);
    if (result != MA_SUCCESS || frames_written != (ma_uint64)frames) {
        ctx->printf(ctx, "# cannot write %s\n", filename);
        return 0;
    }
    ctx->printf(ctx, "# wrote %d frames (%u channel%s) to %s at %u Hz\n",
                frames, channels, channels == 1 ? "" : "s",
                filename, sample_rate);
    return 1;
}

static float record_frame_mono(int frame) {
  int channels = sampling.channels == 2 ? 2 : 1;
  size_t index = (size_t)frame * (size_t)channels;
  if (channels == 1) return sampling.where[index];
  return 0.5f * (sampling.where[index] + sampling.where[index + 1]);
}

static float record_frame_level(int frame) {
  int channels = sampling.channels == 2 ? 2 : 1;
  size_t index = (size_t)frame * (size_t)channels;
  float level = fabsf(sampling.where[index]);
  if (channels == 2) {
    float right = fabsf(sampling.where[index + 1]);
    if (right > level) level = right;
  }
  return level;
}

static int record_frames_cross_zero(int a, int b) {
  int channels = sampling.channels == 2 ? 2 : 1;
  size_t ai = (size_t)a * (size_t)channels;
  size_t bi = (size_t)b * (size_t)channels;
  for (int channel = 0; channel < channels; channel++) {
    float av = sampling.where[ai + (size_t)channel];
    float bv = sampling.where[bi + (size_t)channel];
    if (av == 0.0f || av * bv <= 0.0f) return 1;
  }
  return 0;
}

static int record_trim_run_above(int start, int count, float threshold) {
  if (start < 0 || count <= 0 || start + count > sampling.len) return 0;
  for (int i = 0; i < count; i++) {
    if (record_frame_level(start + i) <= threshold) return 0;
  }
  return 1;
}

void record_find_trim(int argc, float arg0, float arg1, int margin) {
  if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE ||
      !sampling.where || sampling.len <= 0 ||
      sampling.len > sampling.capacity) {
    return;
  }
  int lead0 = -1;
  int trail0 = -1;
  float tleft = RECORD_TRIM_DEFAULT_THRESHOLD;
  float tright = RECORD_TRIM_DEFAULT_THRESHOLD;
  int run = RECORD_TRIM_CONSECUTIVE_SAMPLES;
  if (run > sampling.len) run = sampling.len;
  if (argc > 0 && isfinite(arg0)) {
    tleft = fabsf(arg0);
    tright = tleft;
  }
  if (argc > 1 && isfinite(arg1)) tright = fabsf(arg1);
  if (margin < 0) margin = 0;

  // 1. Find the first audible sample index
  int first_audible = -1;
  for (int i = 0; i <= sampling.len - run; i++) {
    if (record_trim_run_above(i, run, tleft)) {
      first_audible = i;
      break;
    }
  }

  if (first_audible > 0) {
    lead0 = first_audible - margin;
    if (lead0 < 0) lead0 = 0;
    // Look backward into silence/margin to find the closest zero crossing.
    while (lead0 > 0) {
      if (record_frames_cross_zero(lead0, lead0 - 1)) {
        break;
      }
      lead0--;
    }
  } else if (first_audible == 0) {
    lead0 = 0; // Starts immediately with audio
  }

  // 2. Find the last audible sample index
  int last_audible = -1;
  for (int i = sampling.len - run; i >= 0; i--) {
    if (record_trim_run_above(i, run, tright)) {
      last_audible = i + run - 1;
      break;
    }
  }

  if (last_audible >= 0 && last_audible < sampling.len - 1) {
    int end_idx = last_audible + margin;
    if (end_idx >= sampling.len) end_idx = sampling.len - 1;
    // Look forward into silence/margin to find the closest zero crossing.
    while (end_idx < sampling.len - 1) {
      if (record_frames_cross_zero(end_idx, end_idx + 1)) {
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
  skode_compile_result_t result =
    skode_compile_program_ex(text, program, ctx ? ctx->vocab : NULL);
  if (result == SKODE_COMPILE_OK) return 1;
  ctx->printf(ctx, "# command is not schedulable (%d)\n", result);
  return 0;
}

static void skode_queue_repeated(const event_program_t *program, int voice,
    int count, double seconds, int tag) {
  uint64_t dt;
  if (!program || count <= 0 || count > QUEUE_SIZE ||
      !skode_seconds_to_samples(seconds, &dt)) {
    return;
  }
  uint64_t qt = SAMPLE_COUNT_GET();
  for (int i = 0; i < count; i++) {
    if (skode_queue_program(program, voice, qt, tag) != 0) break;
    qt = skode_u64_add(qt, dt);
  }
}

static void skode_repeat_macro(skode_t *ctx, const double *arg, int argc,
    int tempo_relative) {
  int macro_index;
  int count;
  if (!ctx || !arg || argc < 3 ||
      !skode_double_to_int(arg[0], &macro_index) ||
      !skode_extra_valid(macro_index) ||
      !skode_double_to_int(arg[1], &count) ||
      count <= 0 || count > QUEUE_SIZE ||
      !isfinite(arg[2]) || arg[2] < 0.0) {
    return;
  }

  int tag = 0;
  if (argc > 3 && !skode_double_to_int(arg[3], &tag)) return;

  char macro[STRING_BUF_LEN];
  if (skode_extra_copy(macro_index, macro, sizeof(macro)) != 0 ||
      macro[0] == '\0') {
    return;
  }
  event_program_t program;
  if (!skode_compile_scheduled(ctx, macro, &program)) return;

  double seconds = arg[2];
  if (tempo_relative) seconds *= tempo_step_seconds_get() * 4.0f;
  skode_queue_repeated(&program, ctx->voice, count, seconds, tag);
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
    case SKODE_OP_WAVE_LOOP_COUNT:
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
    case SKODE_OP_CONTROL_EVENT:
    case SKODE_OP_DELAY_PARAMS:
      return 1;
    case SKODE_OP_AMP_MOD: return 1;
    case SKODE_OP_PHASE_DISTORTION:
    case SKODE_OP_PHASE_MOD:
    case SKODE_OP_PHASE_ENVELOPE:
    case SKODE_OP_PHASE_ENVELOPE_DEPTH:
      return 1;
    case SKODE_OP_FILTER_ENVELOPE:
    case SKODE_OP_FILTER_ENVELOPE_DEPTH:
      return 1;
    case SKODE_OP_FREQ_MOD:
    case SKODE_OP_FREQ_MOD_MODE:
    case SKODE_OP_FREQ_FEEDBACK:
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
    case SKODE_OP_RING_MOD: return 1;
    case SKODE_OP_WAVE_RANGE_SET:
    case SKODE_OP_WAVE_LOOP_SET:
    case SKODE_OP_POLY_NOTE:
    case SKODE_OP_POLY_RELEASE:
    case SKODE_OP_POLY_BEND:
      return 1;
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

static int skode_foreign_function(skode_t *ctx, int index,
    const double *arg, int argc) {
  if (!ctx || !ctx->parse) return -1;
  skred_foreign_call_t call = {
    .index = index,
    .argc = argc,
    .arg = arg,
    .string = ands_string(ctx->parse),
    .data = ands_data(ctx->parse),
    .data_len = ands_data_len(ctx->parse),
    .voice = ctx->voice,
    .pattern = ctx->pattern,
    .step = ctx->step,
  };
  return skred_foreign_function_call(&call);
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
    opcode->code == SKODE_OP_MIDI_DETUNE ? 1U :
    opcode->code == SKODE_OP_DELAY_PARAMS ? 0x7eU : 0U;
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
    case SKODE_OP_POLY_NOTE:
      {
        int key;
        if (opcode->argc < 4 || opcode->argc > 5 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_note(x, key, opcode->arg[2], opcode->arg[3],
          opcode->argc > 4 ? opcode->arg[4] : 0);
      }
    case SKODE_OP_POLY_RELEASE:
      {
        int key;
        if (opcode->argc < 2 || opcode->argc > 3 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_release(x, key,
          opcode->argc > 2 ? opcode->arg[2] : 0);
      }
    case SKODE_OP_POLY_BEND:
      {
        int key;
        if (opcode->argc < 3 || opcode->argc > 4 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_bend(x, key, opcode->arg[2],
          opcode->argc > 3 ? opcode->arg[3] : 0);
      }
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
    case SKODE_OP_WAVE_LOOP_COUNT:
      return x_valid ? wave_loop_count(voice, x) : -1;
    case SKODE_OP_PHASE_DISTORTION:
      if (opcode->argc == 0) return cz_set(voice, 0, 0.0f);
      if (!x_valid) return -1;
      return cz_set(voice, x,
        opcode->argc > 1 ? opcode->arg[1] : 0.0f);
    case SKODE_OP_PHASE_MOD:
      if (opcode->argc < 2) return cmod_set(voice, -1, 0);
      return x_valid ? cmod_set(voice, x, opcode->arg[1]) : -1;
    case SKODE_OP_PHASE_ENVELOPE:
      if (opcode->argc != 4) return -1;
      envelope_configure_e(&sv.cz_envelope[voice], opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]);
      sv.use_cz_envelope[voice] = !(opcode->arg[0] == 0 &&
        opcode->arg[1] == 0 && opcode->arg[2] == 1 &&
        opcode->arg[3] == 0);
      return 0;
    case SKODE_OP_PHASE_ENVELOPE_DEPTH:
      if (opcode->argc != 1) return -1;
      sv.cz_env_depth[voice] = opcode->arg[0];
      return 0;
    case SKODE_OP_FREQ:
      return opcode->argc == 1 ? freq_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_FILTER_ENVELOPE:
      if (opcode->argc != 4) return -1;
      envelope_configure_e(&sv.filter_envelope[voice], opcode->arg[0],
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
      return freq_mod_mode_set(voice, x);
    case SKODE_OP_FREQ_FEEDBACK:
      if (opcode->argc != 1) return -1;
      return freq_feedback_set(voice, opcode->arg[0]);
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
    #if 1
      return opcode->argc == 1 ?
        skode_linked_velocity(voice, opcode->arg[0], SAMPLE_COUNT_GET()) : -1;
    #else
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
    #endif
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
      } else sv.one_shot[voice] = sw.one_shot[x];
      osc_reclassify(voice);
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
    case SKODE_OP_CONTROL_EVENT:
      return skode_emit_control_event_opcode(opcode, voice, -1, -1, -1);
    case SKODE_OP_DELAY_PARAMS:
      {
        int bus = 1;
        int coarse, fine, feedback, mod_freq, mod_depth, level;
        if (opcode->argc < 1 || opcode->argc > 7 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        delay_params_get(bus, &coarse, &fine, &feedback, &mod_freq,
          &mod_depth, &level);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &coarse)) return -1;
        if (opcode->argc > 2 && isfinite(opcode->arg[2]) &&
            !skode_opcode_int(opcode, 2, &fine)) return -1;
        if (opcode->argc > 3 && isfinite(opcode->arg[3]) &&
            !skode_opcode_int(opcode, 3, &feedback)) return -1;
        if (opcode->argc > 4 && isfinite(opcode->arg[4]) &&
            !skode_opcode_int(opcode, 4, &mod_freq)) return -1;
        if (opcode->argc > 5 && isfinite(opcode->arg[5]) &&
            !skode_opcode_int(opcode, 5, &mod_depth)) return -1;
        if (opcode->argc > 6 && isfinite(opcode->arg[6]) &&
            !skode_opcode_int(opcode, 6, &level)) return -1;
        return delay_params_set(bus, coarse, fine, feedback, mod_freq,
          mod_depth, level);
      }
    case SKODE_OP_RING_MOD:
      if (opcode->argc < 1 || opcode->argc > 2) return -1;
      sv.ring_osc[voice] =
        x_valid && skode_voice_valid(x) ? x : -1;
      sv.ring_amount[voice] =
        opcode->argc > 1 ? opcode->arg[1] : 0;
      return 0;
    case SKODE_OP_WAVE_RANGE_SET:
      if (opcode->argc == 2) {
        voice_wave_range_set(voice, opcode->arg[0], opcode->arg[1]);
        return 0;
      }
      if (opcode->argc == 0) {
        voice_wave_range_reset(voice);
        return 0;
      }
      return -1;
    case SKODE_OP_WAVE_LOOP_SET:
      if (opcode->argc == 2) {
        voice_loop_points_set(voice, opcode->arg[0], opcode->arg[1]);
        return 0;
      }
      if (opcode->argc == 0) {
        voice_loop_points_reset(voice);
        return 0;
      }
      return -1;
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
  int dict_result;
  if (skode_execute_word(ctx, s, atom, arg, argc, &dict_result))
    return dict_result;
  switch (atom) {
    case ATOM4('/als'):
      if (argc == 0) {
        (void)skred_audio_command("/als");
        ctx->printf(ctx, "%s\n", skred_audio_message());
      }
      break;
    case ATOM4('/a?-'):
      if (argc == 0) ctx->printf(ctx, "# %s\n", skred_audio_status());
      break;
    case ATOM4('/ai-'):
    case ATOM4('/ao-'):
      if (argc == 1 && x_valid) {
        int is_capture = atom == ATOM4('/ai-');
        int result = 0;
        if (x >= 0) result = skred_audio_refresh();
        if (result == 0) result = skred_audio_select(is_capture, x);
        if (result == 0) ctx->printf(ctx, "# %s\n", skred_audio_status());
        else ctx->printf(ctx, "# audio selection failed: /a%c %d\n",
          is_capture ? 'i' : 'o', x);
      } else {
        ctx->printf(ctx, "# usage: /a%c selection (-1 default%s)\n",
          atom == ATOM4('/ai-') ? 'i' : 'o',
          atom == ATOM4('/ai-') ? ", -2 off" : "");
      }
      break;
    case ATOM4('/mL-'):
      if (argc == 0) {
        int result = skred_midi_init("pulp");
        if (result == 0) {
          ctx->printf(ctx, "# MIDI inputs\n");
          int count = skred_midi_input_count();
          for (int i = 0; i < count; i++) {
            char name[128] = {0};
            if (skred_midi_input_name(i, name, sizeof(name)) == 0)
              ctx->printf(ctx, "#   %d %s\n", i, name);
          }
          ctx->printf(ctx, "# MIDI outputs\n");
          count = skred_midi_output_count();
          for (int i = 0; i < count; i++) {
            char name[128] = {0};
            if (skred_midi_output_name(i, name, sizeof(name)) == 0)
              ctx->printf(ctx, "#   %d %s\n", i, name);
          }
        } else ctx->printf(ctx, "# MIDI init failed (%d)\n", result);
      }
      break;
    case ATOM4('/m?-'):
      ctx->printf(ctx, "# %s\n", skred_midi_status());
      break;
    case ATOM4('/mi-'):
    case ATOM4('/mo-'):
      if (argc == 1 && x_valid) {
        int result = skred_midi_init("pulp");
        if (result == 0) result = atom == ATOM4('/mi-') ?
          skred_midi_input_open(x) : skred_midi_output_open(x);
        if (result != 0) ctx->printf(ctx, "# MIDI open failed (%d)\n", result);
      }
      break;
    case ATOM4('/md-'):
      if (argc == 0) x = skred_midi_debug_get() ? 0 : 1;
      skred_midi_debug_set(x);
      ctx->printf(ctx, "# midi debug %s\n", skred_midi_debug_get() ? "on" : "off");
      break;
    case ATOM4('/miV'):
    case ATOM4('/moV'):
      if (argc == 0) {
        const char *name = ands_string(ctx->parse);
        int result = skred_midi_init(name[0] ? name : "pulp");
        if (result == 0) result = atom == ATOM4('/miV') ?
          skred_midi_input_open_virtual(name) :
          skred_midi_output_open_virtual(name);
        if (result != 0)
          ctx->printf(ctx, "# MIDI virtual open failed (%d)\n", result);
      }
      break;
    case ATOM4('/mic'):
      if (argc == 0 && skred_midi_input_close() != 0)
        ctx->printf(ctx, "# MIDI input close failed\n");
      break;
    case ATOM4('/moc'):
      if (argc == 0 && skred_midi_output_close() != 0)
        ctx->printf(ctx, "# MIDI output close failed\n");
      break;
    case ATOM4('/mv-'):
    case ATOM4('/mp-'):
      {
        int channel = -1, target;
        float bend = 2.0f;
        if (argc >= 2 && argc <= 3 &&
            (isnan(arg[0]) || skode_double_to_int(arg[0], &channel)) &&
            skode_double_to_int(arg[1], &target) &&
            (argc < 3 || (isfinite(arg[2]) && arg[2] >= 0.0))) {
          if (argc == 3) bend = (float)arg[2];
          int kind = atom == ATOM4('/mv-') ? SKRED_MIDI_ROUTE_VOICE :
            SKRED_MIDI_ROUTE_POOL;
          if (skred_midi_route_set(channel, kind, target, bend) == 0 &&
              skred_control_dispatch_start() == 0)
            ctx->printf(ctx, "# MIDI route installed\n");
          else ctx->printf(ctx, "# MIDI route failed\n");
        } else ctx->printf(ctx, "# usage: /m%c channel target [bend]\n",
          atom == ATOM4('/mv-') ? 'v' : 'p');
      }
      break;
    case ATOM4('/mvd'):
    case ATOM4('/mpd'):
      {
        int channel = -1, target;
        if (argc == 2 &&
            (isnan(arg[0]) || skode_double_to_int(arg[0], &channel)) &&
            skode_double_to_int(arg[1], &target)) {
          int kind = atom == ATOM4('/mvd') ? SKRED_MIDI_ROUTE_VOICE :
            SKRED_MIDI_ROUTE_POOL;
          ctx->printf(ctx, "# MIDI routes removed: %d\n",
            skred_midi_route_remove(channel, kind, target));
        }
      }
      break;
    case ATOM4('/mR-'):
      ctx->printf(ctx, "%s", skred_midi_route_status());
      break;
    case ATOM4('/mC-'):
      skred_midi_route_clear();
      ctx->printf(ctx, "# MIDI routes cleared\n");
      break;
    case ATOM4('/mb-'):
      {
        int type, channel = -1, data1 = -1;
        if (argc == 3 && skode_double_to_int(arg[0], &type) &&
            (isnan(arg[1]) || skode_double_to_int(arg[1], &channel)) &&
            (isnan(arg[2]) || skode_double_to_int(arg[2], &data1)) &&
            ands_string_len(ctx->parse) > 0 &&
            skred_midi_binding_set(type, channel, data1,
              ands_string(ctx->parse)) == 0 &&
            skred_control_dispatch_start() == 0)
          ctx->printf(ctx, "# MIDI Skode binding installed\n");
        else ctx->printf(ctx,
          "# usage: [skode-command] /mb type channel data1\n");
      }
      break;
    case ATOM4('/mbd'):
      {
        int type, channel = -1, data1 = -1;
        if (argc == 3 && skode_double_to_int(arg[0], &type) &&
            (isnan(arg[1]) || skode_double_to_int(arg[1], &channel)) &&
            (isnan(arg[2]) || skode_double_to_int(arg[2], &data1)))
          ctx->printf(ctx, "# MIDI Skode bindings removed: %d\n",
            skred_midi_binding_remove(type, channel, data1));
      }
      break;
    case ATOM4('/mb?'):
      ctx->printf(ctx, "%s", skred_midi_binding_status());
      break;
    case ATOM4('/mbC'):
      skred_midi_binding_clear();
      ctx->printf(ctx, "# MIDI Skode bindings cleared\n");
      break;
    case ATOM4('/pg-'):
      {
        int group, source, width, root = 0;
        if (argc < 3 || argc > 4 ||
            !skode_double_to_int(arg[0], &group) ||
            !skode_double_to_int(arg[1], &source) ||
            !skode_double_to_int(arg[2], &width) ||
            (argc > 3 && !skode_double_to_int(arg[3], &root)) ||
            skred_poly_group_set(group, source, width, root) != 0) {
          ctx->printf(ctx, "# usage: /pg group,source,width[,root-offset]\n");
        }
      }
      break;
    case ATOM4('/pg!'):
      if (!x_valid || argc != 1 || skred_poly_group_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pg! group\n");
      break;
    case ATOM4('/pp-'):
      {
        int pool, group, base, count, policy = SKRED_POLY_STEAL_RELEASE_OLDEST;
        if (argc < 4 || argc > 5 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &group) ||
            !skode_double_to_int(arg[2], &base) ||
            !skode_double_to_int(arg[3], &count) ||
            (argc > 4 && !skode_double_to_int(arg[4], &policy)) ||
            skred_poly_pool_set(pool, group, base, count, policy) != 0) {
          ctx->printf(ctx,
            "# usage: /pp pool,group,base,count[,steal-policy]\n");
        }
      }
      break;
    case ATOM4('/pp!'):
      if (!x_valid || argc != 1 || skred_poly_pool_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pp! pool\n");
      break;
    case ATOM4('/pm-'):
      {
        int pool, mode, priority = SKRED_POLY_PRIORITY_LAST;
        int articulation = SKRED_POLY_ARTICULATION_RETRIGGER;
        if (argc < 2 || argc > 4 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &mode) ||
            (argc > 2 && !skode_double_to_int(arg[2], &priority)) ||
            (argc > 3 && !skode_double_to_int(arg[3], &articulation)) ||
            skred_poly_pool_mode(pool, mode, priority, articulation) != 0) {
          ctx->printf(ctx,
            "# usage: /pm pool,mode[,priority[,articulation]]\n");
        }
      }
      break;
    case ATOM4('?pg-'):
      ctx->printf(ctx, "%s", skred_poly_group_status(x_valid ? x : -1));
      break;
    case ATOM4('?pp-'):
      ctx->printf(ctx, "%s", skred_poly_pool_status(x_valid ? x : -1));
      break;
    case ATOM4('/vg-'):
      {
        int graph_voice, format = 0, depth = 0;
        if (argc < 1 || argc > 3 ||
            !skode_double_to_int(arg[0], &graph_voice) ||
            (argc > 1 && !skode_double_to_int(arg[1], &format)) ||
            (argc > 2 && !skode_double_to_int(arg[2], &depth))) {
          ctx->printf(ctx, "# usage: /vg voice[,format[,depth]]\n");
        } else {
          ctx->printf(ctx, "%s", skred_voice_graph(graph_voice, format, depth));
        }
      }
      break;
    case ATOM4('pn--'):
      {
        int pool, key;
        int result = -1;
        if (argc >= 4 && argc <= 5 &&
            skode_double_to_int(arg[0], &pool) &&
            skode_double_to_int(arg[1], &key))
          result = skred_poly_note(pool, key, arg[2], arg[3],
            argc > 4 ? arg[4] : 0);
        if (result < 0 || argc < 4 || argc > 5 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key))
          ctx->printf(ctx, "# usage: pn pool,key,note,velocity[,cents]\n");
        else if (result > 0)
          ctx->printf(ctx, "# poly pool %d is full (no-steal policy)\n", pool);
      }
      break;
    case ATOM4('pr--'):
      {
        int pool, key;
        if (argc < 2 || argc > 3 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key) ||
            skred_poly_release(pool, key, argc > 2 ? arg[2] : 0) != 0)
          ctx->printf(ctx, "# usage: pr pool,key[,release-velocity]\n");
      }
      break;
    case ATOM4('pb--'):
      {
        int pool, key;
        if (argc < 3 || argc > 4 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key) ||
            skred_poly_bend(pool, key, arg[2], argc > 3 ? arg[3] : 0) != 0)
          ctx->printf(ctx, "# usage: pb pool,key,semitones[,cents]\n");
      }
      break;
    case ATOM4('wait'): // blocking msec wait
      if (x_valid && x >= 0) sk_sleep(x);
      break;
    case ATOM4('clr-'): // clear parser argument stack
      ands_arg_clear(s);
      return 1;
    case ATOM4('drop'): // drop first parser argument
      ands_arg_drop(s);
      return 1;
    case ATOM4('dup-'): // duplicate first parser argument
      ands_arg_dup(s);
      return 1;
    case ATOM4('over'): // duplicate second parser argument to front
      ands_arg_over(s);
      return 1;
    case ATOM4('rot-'): // rotate first three parser arguments left
      ands_arg_rot(s);
      return 1;
    case ATOM4('swap'): // swap first two parser arguments
      ands_arg_swap(s);
      return 1;
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
    case ATOM4('BC--'): // bounded one-shot loop count
      if (argc && x_valid) wave_loop_count(voice, x);
      break;
    case ATOM4('c---'): // phase-distortion algo distortion
      if (argc == 0) {
        cz_set(voice, 0, 0);
      } else if (argc == 1) {
        cz_set(voice, x, 0);
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
    case ATOM4('ct--'): // phase-distortion ADSR A D S R
      if (argc == 4) {
        float a = arg[0];
        float d = arg[1];
        float s = arg[2];
        float r = arg[3];
        envelope_configure_e(&sv.cz_envelope[voice], a, d, s, r);
        sv.use_cz_envelope[voice] = !(a == 0 && d == 0 && s == 1 && r == 0);
      }
      break;
    case ATOM4('cd--'): // phase-distortion envelope depth
      if (argc) sv.cz_env_depth[voice] = arg[0];
      break;
    case ATOM4('D---'): // data-size
      if (argc) {
        if (x > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, x);
      } else {
        ctx->printf(ctx, "# D[%d]\n", ands_data_cap(ctx->parse));
      }
      break;
    case ATOM4('MO--'): // MIDI output bytes
      {
        uint8_t bytes[3];
        if (argc < 1 || argc > 3) {
          ctx->printf(ctx, "# usage: MO status[,data1[,data2]]\n");
          break;
        }
        int valid = 1;
        for (int i = 0; i < argc; i++) {
          int byte;
          if (!skode_double_to_int(arg[i], &byte) || byte < 0 || byte > 255 ||
              arg[i] != (double)byte) {
            valid = 0;
            break;
          }
          bytes[i] = (uint8_t)byte;
        }
        int result = valid ? skred_midi_send_raw(bytes, argc) : -2;
        if (result != 0)
          ctx->printf(ctx, "# MIDI output failed (%d)\n", result);
      }
      break;
    case ATOM4('ce--'): // control-plane user event id [value0 [value1 [value2]]]
      if (argc > 0 && argc <= 4) {
        opcode_event_t opcode = {
          .code = SKODE_OP_CONTROL_EVENT,
          .argc = (uint8_t)argc,
        };
        for (int i = 0; i < argc; i++) opcode.arg[i] = (float)arg[i];
        skode_emit_control_event_opcode(&opcode, voice, -1, -1, -1);
      }
      break;
    case ATOM4('?d--'): // show-skode-data (summary)
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        skode_double_dump(ctx, data, data_len);
      }
      break;
    case ATOM4('ft--'): // filter-adsr A D S R
      if (argc == 4) {
        float a = arg[0];
        float d = arg[1];
        float s = arg[2];
        float r = arg[3];
        envelope_configure_e(&sv.filter_envelope[voice], a, d, s, r);
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
      if (argc) freq_mod_mode_set(voice, x);
      break;
    case ATOM4('FB--'): // FF2 operator feedback amount
      if (argc) freq_feedback_set(voice, arg[0]);
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
    case ATOM4('/ks-'): // ksynth-load num (verbose)
      {
        char *file = ands_string(ctx->parse);
        int verbose = 0;
        if (argc) skode_double_to_int(arg[0], &verbose);
        if (strlen(file)) {
          ksynth_load_name(ctx, file, verbose);
        }
      }
      break;
    case ATOM4('/k--'): // ksynth-load num (verbose)
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        ksynth_load(ctx, x, verbose);
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
        if (len) skode_ks_eval(ctx, cmd, len);
      }
      break;
    case ATOM4('kw--'): // wait for last ksynth request [timeout-ms]
      {
        (void)x;
      }
      break;
    case ATOM4('kw>-'): // compatibility: copy latest ksynth result to data
      {
        (void)x;
        skode_ks_result_to_data(ctx);
      }
      break;
    case ATOM4('k?--'): // k show last results
      {
        K result = (K)ctx->ks_result;
        if (result && !k_is_func(result))
          skode_double_dump(ctx, result->f, (size_t)result->n);
      }
      break;
    case ATOM4('k>d-'): // k results to d?
      {
        skode_ks_result_to_data(ctx);
      }
      break;
    case ATOM4('k>w-'):
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = (float)MAIN_SAMPLE_RATE;
        float offset = 0.0f;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) break;
        if (argc > 1) rate = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &one_shot);
        if (argc > 3) offset = arg[3];
        if (skode_ks_result_to_data(ctx))
          data_load(ctx, wave_slot, one_shot, rate, offset);
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
    #if 1
      if (argc) skode_linked_velocity(voice, arg[0], SAMPLE_COUNT_GET());
    #else
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
    #endif
      break;
    case ATOM4('M---'): // tempo bpm
      if (argc && tempo_set(arg[0]) != 0)
        ctx->printf(ctx, "# tempo must be between %g and %g BPM\n",
          (double)SEQ_TEMPO_MIN_BPM, (double)SEQ_TEMPO_MAX_BPM);
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
    case ATOM4('ds--'): // track-delay send amount; active only for routed, centered, unmodulated voices
      if (argc) delay_send_set(voice, arg[0]);
      break;
    case ATOM4('DL--'): // track-delay params track coarse fine feedback mod-freq mod-depth level
      {
        int bus = 1;
        int coarse, fine, feedback, mod_freq, mod_depth, level;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        delay_params_get(bus, &coarse, &fine, &feedback, &mod_freq, &mod_depth, &level);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &coarse);
        if (argc > 2 && isfinite(arg[2])) skode_double_to_int(arg[2], &fine);
        if (argc > 3 && isfinite(arg[3])) skode_double_to_int(arg[3], &feedback);
        if (argc > 4 && isfinite(arg[4])) skode_double_to_int(arg[4], &mod_freq);
        if (argc > 5 && isfinite(arg[5])) skode_double_to_int(arg[5], &mod_depth);
        if (argc > 6 && isfinite(arg[6])) skode_double_to_int(arg[6], &level);
        delay_params_set(bus, coarse, fine, feedback, mod_freq, mod_depth, level);
      }
      break;
    case ATOM4('DL?-'): // show track delay params
      if (argc) {
        int bus = 1;
        skode_double_to_int(arg[0], &bus);
        ctx->printf(ctx, "%s", delay_bus_format(bus));
      } else {
        ctx->printf(ctx, "%s", delay_format());
      }
      break;
    case ATOM4('GS--'): // show global synth status
      global_status_show(ctx, argc > 0 && arg[0] > 0.0);
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
    case ATOM4('r---'): // route voice to track, 0=master only, 1..4=track/delay bus
      if (argc) synth_record_track_set(voice, x);
      break;
    case ATOM4('rt--'): // track-name track
      if (argc && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_name_set(x, ands_string(ctx->parse));
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
      }
      break;
    case ATOM4('rv--'): // track-volume track dB
      if (argc > 1 && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_volume_set(x, arg[1]);
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
      }
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
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        double seconds = tempo_step_seconds_get() * 4.0f * arg[1];
        skode_queue_repeated(&program, ctx->voice, x, seconds, tag);
      } break;
    case ATOM4('eRR-'): // repeat-external-macro-tempo macro count beats [tag]
      skode_repeat_macro(ctx, arg, argc, 1);
      break;
    case ATOM4('eR--'): // repeat-external-macro macro count seconds [tag]
      skode_repeat_macro(ctx, arg, argc, 0);
      break;
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
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        skode_queue_repeated(&program, ctx->voice, x, arg[1], tag);
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
    case ATOM4('vc--'): // voice control-plane event publication bool
      if (argc) voice_control_events_set(voice, x != 0);
      break;
    case ATOM4('V---'): // main-volume loudness
      if (argc) {
        volume_set(arg[0]);
        scope_ipc_track_metadata_set(0, synth_track_name_get(0),
          synth_track_volume_db_get(0));
      }
      break;
    case ATOM4('vt--'): // [name] voice-text-set
      skode_copy_string(sv.text[voice], TEXT_MAX, ands_string(ctx->parse));
      break;
    case ATOM4('wt--'): // [name] wave-text-set wave-number
      if (argc && skode_wave_valid(x)) {
        skode_copy_string(sw.name[x], WAVE_NAME_MAX, ands_string(ctx->parse));
      }
      break;
    case ATOM4('WL--'): // wave-loop-points wave start end
      if (argc > 2 && x_valid && skode_wave_valid(x)) {
        int start, end;
        if (skode_double_to_int(arg[1], &start) &&
            skode_double_to_int(arg[2], &end)) {
          wave_loop_points_set(x, start, end);
        }
      }
      break;
    case ATOM4('VS--'): // voice-set-points start end; no args resets from wave
      if (argc >= 2) {
        int start, end;
        if (skode_double_to_int(arg[0], &start) &&
            skode_double_to_int(arg[1], &end)) {
          if (voice_wave_range_set(voice, start, end) != 0) {
            ctx->printf(ctx,
              "# VS rejected for v%d: %d..%d must be within 0..%d\n",
              voice, start, end, sv.table_size[voice]);
          }
        }
      } else if (argc == 0) {
        voice_wave_range_reset(voice);
      }
      break;
    case ATOM4('VL--'): // voice-loop-points start end; no args resets from wave
      if (argc >= 2) {
        int start, end;
        if (skode_double_to_int(arg[0], &start) &&
            skode_double_to_int(arg[1], &end)) {
          if (voice_loop_points_set(voice, start, end) != 0) {
            ctx->printf(ctx,
              "# VL rejected for v%d: %d..%d must be within VS %d..%d\n",
              voice, start, end, sv.wave_range_start[voice],
              sv.wave_range_end[voice]);
          }
        }
      } else if (argc == 0) {
        voice_loop_points_reset(voice);
      }
      break;
    case ATOM4('VW--'): // voice-wave-show [voice] [width height]
      {
        int target_voice = voice;
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT;
        if (argc == 1) {
          int parsed_voice;
          if (skode_double_to_int(arg[0], &parsed_voice)) target_voice = parsed_voice;
        } else if (argc == 2) {
          w = wave_display_dim(arg[0], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[1], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        } else if (argc >= 3) {
          int parsed_voice;
          if (skode_double_to_int(arg[0], &parsed_voice)) target_voice = parsed_voice;
          w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        }
        if (target_voice >= 0 && target_voice < synth_config.voice_max) {
          int wave = sv.wave_table_index[target_voice];
          if (skode_wave_valid(wave)) {
            // ctx->printf(ctx, "# wave [%d..%d)\n", sv.wave_range_start[target_voice], sv.wave_range_end[target_voice]);
            char label[96];
            snprintf(label, sizeof(label), "v%d w%d", target_voice, wave);
            wavetable_waveform_show(ctx, wave, w, h,
              sv.loop_start[target_voice], sv.loop_end[target_voice], label);
          }
        }
      }
      break;
    case ATOM4('w---'): // wave-select which-wave interpolate? mode-override?
      if (argc && wave_set(voice, x) == 0) {
        int n;
        if (argc > 1) {
          if (skode_double_to_int(arg[1], &n)) sv.interpolate[voice] = n != 0;
        }
        if (argc > 2) {
          if (skode_double_to_int(arg[2], &n)) sv.one_shot[voice] = n != 0;
        } else sv.one_shot[voice] = sw.one_shot[x];
        osc_reclassify(voice);
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
    case ATOM4('d*--'): // show an element from d array
      if (argc) {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (x>=0 && x < data_len) {
          double val = data[x];
          ctx->printf(ctx, "# %g\n", val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      break;
    case ATOM4('d>r-'): // data-to-rec
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (!data || data_len <= 0) break;
        int sample_state = atomic_load_int(&sampling.state);
        if (sample_state == SAMPLE_STATE_ARMED ||
            sample_state == SAMPLE_STATE_RECORDING) {
          ctx->printf(ctx, "# recording buffer busy\n");
          break;
        }
        if (data_len > sampling.capacity) skode_sample_alloc(data_len);
        if (!sampling.where || data_len > sampling.capacity) {
          ctx->printf(ctx, "# recording buffer allocation failed\n");
          break;
        }
        for (int i=0; i<data_len; i++) sampling.where[i] = (float)data[i];
        sampling.len = data_len;
        sampling.channels = 1;
        sampling.offset = 0;
        sampling.trim = 0;
        atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
      }
      break;
    case ATOM4('d>MO'): // data-to-MIDI-output
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (!data || data_len <= 0 || data_len > 65536) {
          ctx->printf(ctx, "# d>MO requires 1..65536 data bytes\n");
          break;
        }
        uint8_t *bytes = (uint8_t*)malloc((size_t)data_len);
        if (!bytes) {
          ctx->printf(ctx, "# d>MO allocation failed\n");
          break;
        }
        int valid = 1;
        for (int i = 0; i < data_len; i++) {
          int byte;
          if (!skode_double_to_int(data[i], &byte) || byte < 0 || byte > 255 ||
              data[i] != (double)byte) {
            valid = 0;
            break;
          }
          bytes[i] = (uint8_t)byte;
        }
        int result = valid ? skred_midi_send_raw(bytes, data_len) : -2;
        free(bytes);
        if (result != 0)
          ctx->printf(ctx, "# MIDI output failed (%d)\n", result);
      }
      break;
    case ATOM4('d>k-'): // data-to-ksynth-variable
      if (argc) {
        int variable;
        if (skode_double_to_int(arg[0], &variable)) {
          skode_ks_bind_values(ctx, variable, ands_data(ctx->parse),
                               (size_t)ands_data_len(ctx->parse));
        }
      }
      break;
    case ATOM4('w>k-'): // wavetable-to-ksynth-variable
      if (argc > 1) {
        int wave;
        int variable;
        if (!skode_double_to_int(arg[0], &wave) ||
            !skode_double_to_int(arg[1], &variable) ||
            !skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) {
          ctx->printf(ctx, "# invalid wavetable for w>k\n");
          break;
        }
        size_t len = (size_t)sw.size[wave];
        if (len > 1000000 || len > SIZE_MAX / sizeof(double)) {
          ctx->printf(ctx, "# ksynth vector too large: %zu\n", len);
          break;
        }
        double *values = malloc(len * sizeof(double));
        if (!values) {
          ctx->printf(ctx, "# allocation failed\n");
          break;
        }
        for (size_t i = 0; i < len; i++) values[i] = sw.data[wave][i];
        skode_ks_bind_values(ctx, variable, values, len);
        free(values);
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
        int sample_state = atomic_load_int(&sampling.state);
        if (sample_state == SAMPLE_STATE_ARMED ||
            sample_state == SAMPLE_STATE_RECORDING) {
          valid = 0;
          ctx->printf(ctx, "# recording buffer busy\n");
        } else if (sw.size[x] > sampling.capacity) {
          skode_sample_alloc(sw.size[x]);
          valid = sampling.where != NULL && sampling.capacity >= sw.size[x];
        }
        if (valid) {
          sampling.offset = 0;
          sampling.trim = 0;
          for (int i=0; i<sw.size[x]; i++) sampling.where[i] = sw.data[x][i];
          sampling.len = sw.size[x];
          sampling.channels = 1;
          atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
        }
      }
      break;
    case ATOM4('w>w-'): // wave-to-wav-file
      if (!ands_string_fresh(ctx->parse) ||
          !ands_string(ctx->parse)[0]) {
        ctx->printf(ctx, "# w>w requires [filename]\n");
      } else if (!x_valid || !skode_wave_valid(x) ||
                 !sw.data[x] || sw.size[x] <= 0) {
        ctx->printf(ctx, "# invalid wavetable for w>w\n");
      } else {
        double stored_rate = sw.rate[x];
        ma_uint32 sample_rate = MAIN_SAMPLE_RATE;
        if (isfinite(stored_rate) && stored_rate >= 1.0 &&
            stored_rate <= (double)UINT32_MAX - 0.5) {
          sample_rate = (ma_uint32)(stored_rate + 0.5);
        }
        skode_write_wav(ctx, ands_string(ctx->parse), sw.data[x],
                        sw.size[x], 1, sample_rate, 0);
      }
      break;
    case ATOM4('w!--'): // wave-lock
      {
        if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE ||
            !sampling.where || sampling.offset < 0 || sampling.trim < 0 ||
            sampling.offset > sampling.len ||
            sampling.trim > sampling.len - sampling.offset) {
          ctx->printf(ctx, "# invalid recording bounds\n");
          break;
        }
        int channels = sampling.channels == 2 ? 2 : 1;
        int new_len = sampling.len - sampling.offset - sampling.trim;
        memmove(sampling.where,
                sampling.where + (size_t)sampling.offset * channels,
                (size_t)new_len * channels * sizeof(float));
        sampling.len = new_len;
        sampling.trim = 0;
        sampling.offset = 0;
      }
      break;
    case ATOM4('w*--'): // wave-nudge-reset
      if (atomic_load_int(&sampling.state) == SAMPLE_STATE_COMPLETE) {
        sampling.offset = 0;
        sampling.trim = 0;
      } else {
        ctx->printf(ctx, "# recording buffer is not complete\n");
      }
      break;
    case ATOM4('w>--'): // wave-nudge-start
      if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
        ctx->printf(ctx, "# recording buffer is not complete\n");
        break;
      }
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
      if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
        ctx->printf(ctx, "# recording buffer is not complete\n");
        break;
      }
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
        int margin = 0;
        if (argc > 0) arg0 = arg[0];
        if (argc > 1) arg1 = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &margin);
        record_find_trim(argc, arg0, arg1, margin);
      }
      break;
    case ATOM4('WS--'): // wave-spectro which-wave
      if (argc && arg[0] >= 0) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT / 2;
        wavetable_spectrogram_show(ctx, x, w, h, sw.loop_start[x], sw.loop_end[x], NULL);
      }
      break;
    case ATOM4('W---'): // wave-show which-wave
      if (argc) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT;
        int m = 0;
        int wave_max = synth_config.wave_table_max - 1;
        int show_record_buffer = (arg[0] < 0 || isnan(arg[0]));
        if (show_record_buffer) {
          if (argc > 1) {
            w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          }
          if (argc > 2) {
            h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
          }
        } else if (argc == 2) {
          if (isnan(arg[1])) m = wave_max;
          else if (!skode_double_to_int(arg[1], &m)) m = x;
          if (m < x) m = x;
          if (m > wave_max) m = wave_max;
        } else if (argc >= 3) {
          w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        }
        if (!show_record_buffer && skode_wave_valid(x)) {
        if (m == 0) {
            wavetable_waveform_show(ctx, x, w, h, sw.loop_start[x],
              sw.loop_end[x], NULL);
          } else {
            for (int i=x; i<=m; i++) {
              wavetable_show(ctx, i);
            }
          }
        } else {
          if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
            ctx->printf(ctx, "# recording buffer is not complete\n");
          } else if (sampling.where) {
            if ((sampling.offset > sampling.len) || (sampling.len - sampling.trim <= 0)) {
              ctx->printf(ctx,"NO!\n");
              ctx->printf(ctx, "offset:%d\n", sampling.offset);
              ctx->printf(ctx, "trim:%d\n", sampling.trim);
              ctx->printf(ctx, "len:%d\n", sampling.len);
              ctx->printf(ctx, "where:%p\n", sampling.where);
              ctx->printf(ctx, "state:%d\n",
                          atomic_load_int(&sampling.state));
            } else {
              float *display = sampling.where;
              float *mono = NULL;
              if (sampling.channels == 2) {
                mono = malloc((size_t)sampling.len * sizeof(float));
                if (mono) {
                  for (int i = 0; i < sampling.len; i++)
                    mono[i] = record_frame_mono(i);
                  display = mono;
                }
              }
              print_wave_stats(ctx, "recording", display, sampling.len,
                               (float)MAIN_SAMPLE_RATE);
              print_audio_braille_labeled(ctx, display, sampling.len, w, h,
                sampling.offset, sampling.len - sampling.trim);
              free(mono);
              int len = sampling.len - sampling.offset - sampling.trim;
              ctx->printf(ctx, "# recording channels %d\n",
                          sampling.channels == 2 ? 2 : 1);
              ctx->printf(ctx,"# found start %d end %d |%d| %gms\n",
                sampling.offset, sampling.len - sampling.trim, len,
                SAMPLES_TO_MSEC(len));
              ctx->printf(ctx,"+offset %d -trim %d = |%d| %gms\n",
                sampling.offset, sampling.trim, len,
                SAMPLES_TO_MSEC(len));
            }
          }
        }
      } else if (argc == 0) {
        int c = 0;
        ctx->printf(ctx, "# MAX %d\n", synth_config.wave_table_max);
        for (int i=0; i<synth_config.wave_table_max; i++) {
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
        seq_edit_lock();
        char *s = seq_step_get(ctx->pattern, x);
        ands_string_from_external(ctx->parse, s, strlen(s));
        seq_edit_unlock();
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
        seq_edit_lock();
        skode_copy_string(seq_text[ctx->pattern], TEXT_MAX, ands_string(ctx->parse));
        seq_edit_unlock();
      }
      break;
    case ATOM4('ym--'): // pattern-mute 0/1
      if (argc) seq_mute_set(ctx->pattern, x);
      break;
    case ATOM4('yc--'): // pattern control-plane event publication bool
      if (argc) seq_control_events_set(ctx->pattern, x);
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
        ctx->printf(ctx, "M%g\n", tempo_bpm_get());
        for (int p = 0; p < PATTERNS_MAX; p++) pattern_show(ctx, p, 0);
      }
      break;
    case ATOM4('z?\?-'): // show all patterns
    case ATOM4('Z?--'): // show all patterns
      ctx->printf(ctx, "M%g\n", tempo_bpm_get());
      for (int p = 0; p < PATTERNS_MAX; p++) pattern_show(ctx, p, 1);
      break;
    case ATOM4('XM--'): // ring modulation osc amount
      if (argc) {
        sv.ring_osc[voice] = x_valid && skode_voice_valid(x) ? x : -1;
        if (argc > 1) sv.ring_amount[voice] = arg[1];
        else sv.ring_amount[voice] = 0.0;
      }
      break;
    case ATOM4('v?--'): // show-voice
    case ATOM4('?---'): // show-voice
      voice_show(ctx, voice, ' ', ctx->verbose); break;
    case ATOM4('\\---'): // verbose-show-voice
      voice_show(ctx, voice, ' ', 1); break;
    case ATOM4('v?\?-'): // show-active-voices
    case ATOM4('?\?--'): // show-active-voices
      voice_show_all(ctx, voice, ctx->verbose); break;
    case ATOM4('?r--'): // show track routing
      record_tracks_show(ctx); break;
    case ATOM4('?s--'): // show-skode-string
      ctx->printf(ctx, "# [%s]\n", ands_string(ctx->parse));
      break;
    case ATOM4('s?--'): // show parser-local string slot [index]
      if (argc && x_valid) {
        if (x >= 0 && x < SKODE_STRING_SLOT_MAX)
          ctx->printf(ctx, "# s%d [%s]\n", x, ctx->string_slot[x]);
      } else {
        for (int i = 0; i < SKODE_STRING_SLOT_MAX; i++) {
          if (ctx->string_slot[i][0])
            ctx->printf(ctx, "# s%d [%s]\n", i, ctx->string_slot[i]);
        }
      }
      break;
    case ATOM4('?m--'): // show-ands-macros
      skode_macros_show(ctx, 0);
      break;
    case ATOM4('?ce-'): // show control-plane event snapshot
      control_event_show(ctx, 0);
      break;
    case ATOM4('?ce!'): // clear outstanding control-plane events
      ctx->printf(ctx, "# control events cleared:%d\n",
        skred_control_event_clear());
      break;
    case ATOM4('?q--'): // show scheduled opcode queue
      opcode_queue_show(ctx);
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
    case ATOM4('/q--'): // quit
      ctx->quit = -1;
      return 0;
    case ATOM4('/sg-'): // start shared-memory scope publication
      {
        const char *name = ands_string_fresh(ctx->parse)
          ? ands_string(ctx->parse) : SKRED_SCOPE_DEFAULT_NAME;
        uint32_t channel_mask = SKRED_SCOPE_ALL_CHANNELS;
        double buffer_seconds = SKRED_SCOPE_DEFAULT_SECONDS;
        int mask = 0;
        if (!name || name[0] == '\0') name = SKRED_SCOPE_DEFAULT_NAME;
        if (argc > 0) {
          if (!skode_double_to_int(arg[0], &mask) || mask <= 0 ||
              (uint32_t)mask > SKRED_SCOPE_ALL_CHANNELS) {
            ctx->printf(ctx, "# /sg channel mask must be 1..%u\n",
                        SKRED_SCOPE_ALL_CHANNELS);
            break;
          }
          channel_mask = (uint32_t)mask;
        }
        if (argc > 1) buffer_seconds = arg[1];
        if (!isfinite(buffer_seconds) || buffer_seconds <= 0.0) {
          ctx->printf(ctx, "# /sg buffer seconds must be > 0\n");
        } else if (scope_ipc_start(name, channel_mask,
                                   buffer_seconds) == 0) {
          skred_scope_status_t status;
          scope_ipc_status(&status);
          ctx->printf(ctx,
            "# scope [%s] channels=%u mask=%u capacity=%u frames\n",
            status.name, status.channel_count, status.channel_mask,
            status.capacity_frames);
        } else {
          ctx->printf(ctx, "# scope start failed [%s]\n", name);
        }
      }
      break;
    case ATOM4('/ss-'): // stop shared-memory scope publication
      scope_ipc_stop();
      ctx->printf(ctx, "# scope stopped\n");
      break;
    case ATOM4('/s?-'): // shared-memory scope status
      {
        skred_scope_status_t status;
        scope_ipc_status(&status);
        if (status.active) {
          ctx->printf(ctx,
            "# scope state=publishing name=[%s] rate=%d channels=%d mask=%u capacity=%u frames=%llu\n",
            status.name, status.sample_rate, status.channel_count,
            status.channel_mask, status.capacity_frames,
            (unsigned long long)status.write_frame);
        } else {
          ctx->printf(ctx, "# scope state=stopped\n");
        }
      }
      break;
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
    case ATOM4('/r--'): // sample-to-wave slot one_shot channel
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 1;
        int channel = -1;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) break;
        if (argc > 1 &&
            (!skode_double_to_int(arg[1], &one_shot) ||
             (one_shot != 0 && one_shot != 1))) {
          ctx->printf(ctx, "# /r mode must be 0=cycle or 1=one-shot\n");
          break;
        }
        if (argc > 2 && !skode_double_to_int(arg[2], &channel)) break;
        if (argc > 3) {
          ctx->printf(ctx, "# usage: /r slot[,mode[,channel]]\n");
          break;
        }
        rec_load(ctx, wave_slot, one_shot, channel);
      }
      break;
                        //              x/0  1     2        3
                        //              300  rate one-shot offset
    case ATOM4('/d--'): // data-to-wave slot rate  one-shot offset
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = (float)MAIN_SAMPLE_RATE;
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
    case ATOM4('/ff-'): // foreign C function slot arg...
      {
        if (!argc) break;
        int index;
        if (!skode_double_to_int(arg[0], &index) ||
            index < 0 || index >= SKRED_FOREIGN_FUNCTION_MAX) break;
        (void)skode_foreign_function(ctx, index, arg + 1, argc - 1);
      }
      break;
    case ATOM4('/m--'): // remove-ands-macro [name]
      {
        const char *name = ands_string_fresh(ctx->parse) ? ands_string(ctx->parse) : "";
        if (name && name[0]) {
          int removed = ands_macro_remove(ctx->parse, name);
          ctx->printf(ctx, "# macro [%s] %s\n", name, removed ? "removed" : "not found");
        } else {
          ctx->printf(ctx, "# /m requires [name]\n");
        }
      }
      break;
    case ATOM4('/m!-'): // clear-ands-macros
      ands_macro_clear(ctx->parse);
      ctx->printf(ctx, "# macros cleared\n");
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
    case ATOM4('/cer'): // control-event responder bool
      if (argc && x_valid) skred_control_response_set_enabled(x != 0);
      ctx->printf(ctx, "%s", skred_control_response_status());
      break;
    case ATOM4('/ce?'): // control-event responder status
      ctx->printf(ctx, "%s", skred_control_response_status());
      break;
    case ATOM4('/th?'): // skred service/thread health
      ctx->printf(ctx, "%s", skred_thread_status());
      break;
    case ATOM4('/ce!'): // control-event responder remove/clear
      if (argc == 0) {
        skred_control_response_clear();
        ctx->printf(ctx, "# ce bindings cleared\n");
      } else if (argc > 1 && x_valid) {
        int key;
        if (skode_double_to_int(arg[1], &key)) {
          int removed = skred_control_response_remove((uint32_t)x, key);
          ctx->printf(ctx, "# ce bindings removed %d\n", removed);
        }
      }
      break;
    case ATOM4('/ceb'): // bind parser string to control event type key
      if (argc > 1 && x_valid && ands_string_len(ctx->parse) > 0) {
        int key;
        if (skode_double_to_int(arg[1], &key) &&
            skred_control_response_bind((uint32_t)x, key,
              ands_string(ctx->parse)) == 0) {
          ctx->printf(ctx, "# ce bound %d,%d -> %s\n", x, key,
            ands_string(ctx->parse));
        } else {
          ctx->printf(ctx, "# ce binding failed\n");
        }
      } else {
        ctx->printf(ctx, "# usage: [skode-command] /ceb type key\n");
      }
      break;
    case ATOM4('/cex'): // bind external string slot to control event type key
      if (argc > 2 && x_valid) {
        int index, type, key;
        char command[STRING_BUF_LEN];
        if (skode_double_to_int(arg[0], &index) &&
            skode_double_to_int(arg[1], &type) &&
            skode_double_to_int(arg[2], &key) &&
            skode_extra_copy(index, command, sizeof(command)) == 0 &&
            command[0] != '\0' &&
            skred_control_response_bind((uint32_t)type, key, command) == 0) {
          ctx->printf(ctx, "# ce bound %d,%d -> %s\n", type, key, command);
        } else {
          ctx->printf(ctx, "# ce binding failed\n");
        }
      } else {
        ctx->printf(ctx, "# usage: /cex external type key\n");
      }
      break;
    case ATOM4('<s--'): // parser-local string slot to parser string
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        ands_string_from_external(ctx->parse, ctx->string_slot[x],
                                  strlen(ctx->string_slot[x]));
      }
      break;
    case ATOM4('s>--'): // parser string to parser-local string slot
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        skode_copy_string(ctx->string_slot[x], SKODE_STRING_SLOT_LEN,
                          ands_string(ctx->parse));
      }
      break;
    case ATOM4('s%--'): // format parser string with numeric args
      {
        char formatted[SKODE_STRING_SLOT_LEN];
        skode_format_string_args(formatted, sizeof(formatted),
                                 ands_string(ctx->parse), arg, argc);
        ands_string_from_external(ctx->parse, formatted, strlen(formatted));
        return 1;
      }
    case ATOM4('<e--'): // external-string-to-skode external-index
      if (argc && skode_extra_valid(x)) {
        char macro[STRING_BUF_LEN];
        if (skode_extra_copy(x, macro, sizeof(macro)) == 0)
          ands_string_from_external(ctx->parse, macro, strlen(macro));
      }
      break;
    case ATOM4('e>--'): // skode-string-to-external external-index
      if (argc && skode_extra_valid(x)) {
        char *s = ands_string(ctx->parse);
        simple_mutex_lock(&skode_extra_mutex);
        skode_copy_string(EXTRA_PTR(x), STRING_BUF_LEN, s);
        simple_mutex_unlock(&skode_extra_mutex);
      }
      break;
    case ATOM4('e!--'): // execute-string num
      {
        char macro[STRING_BUF_LEN] = "";
        const char *s = "";
        if (argc == 0) {
          s = ands_string(ctx->parse);
        } else if (skode_extra_copy(x, macro, sizeof(macro)) == 0) {
          s = macro;
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
      simple_mutex_lock(&skode_extra_mutex);
      if (argc) {
        if (skode_extra_valid(x)) ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(x), x);
      } else {
        for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
          if (strlen(EXTRA_PTR(i)))
            ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
        }
      }
      simple_mutex_unlock(&skode_extra_mutex);
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
              simple_mutex_lock(&skode_extra_mutex);
              for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
                if (strlen(EXTRA_PTR(i)))
                  ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
              }
              simple_mutex_unlock(&skode_extra_mutex);
              break;
          }
        }
      }
      break;
    case ATOM4('/h--'): // show command help
      skode_help(ctx, arg, argc);
      break;
    case ATOM4('/l--'): // skode-load num
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        skode_load(ctx, voice, x, verbose);
      }
      break;
    case ATOM4('/ls-'): // skode-load-string filename
      if (strlen(ands_string(ctx->parse))) {
        int verbose = 0;
        if (argc > 0) skode_double_to_int(arg[0], &verbose);
        skode_load_name(ctx, ands_string(ctx->parse), verbose);
      } else {
        ctx->printf(ctx, "# /ls requires [filename]\n");
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
      if (!ands_string_fresh(ctx->parse) ||
          !ands_string(ctx->parse)[0]) {
        ctx->printf(ctx, "# >r requires [filename]\n");
      } else {
        int state = atomic_load_int(&sampling.state);
        if (state != SAMPLE_STATE_COMPLETE) {
          ctx->printf(ctx, "# recording buffer is not complete\n");
        } else {
          skode_write_wav(ctx, ands_string(ctx->parse),
                          sampling.where, sampling.len,
                          sampling.channels == 2 ? 2 : 1,
                          MAIN_SAMPLE_RATE, 1);
        }
      }
      break;
    case ATOM4('^r--'): // record duration ... markdown/html doesn't like <
    case ATOM4('<r--'): // record duration source voice
      if (argc && isfinite(arg[0]) && arg[0] > 0.0 &&
          arg[0] <= (double)(INT_MAX / AUDIO_CHANNELS) / MAIN_SAMPLE_RATE) {
        int source = SAMPLE_SOURCE_DRY;
        int sample_voice = -1;
        if (argc > 1 && !skode_double_to_int(arg[1], &source)) {
          ctx->printf(ctx, "# <r source must be 0=dry, 1=voice, or 2=master\n");
          break;
        }
        if (source < SAMPLE_SOURCE_DRY || source > SAMPLE_SOURCE_MASTER) {
          ctx->printf(ctx, "# <r source must be 0=dry, 1=voice, or 2=master\n");
          break;
        }
        if (source == SAMPLE_SOURCE_VOICE) {
          if (argc != 3 ||
              !skode_double_to_int(arg[2], &sample_voice) ||
              !skode_voice_valid(sample_voice)) {
            ctx->printf(ctx, "# usage: <r seconds,1,voice\n");
            break;
          }
        } else if (argc > 2) {
          ctx->printf(ctx, "# usage: <r seconds[,source[,voice]]\n");
          break;
        }
        if (!skode_sample_go((int)(arg[0] * (double)MAIN_SAMPLE_RATE),
                             source, sample_voice)) {
          ctx->printf(ctx, "# recording buffer busy or allocation failed\n");
        }
      } else {
        int state = atomic_load_int(&sampling.state);
        ctx->printf(ctx, "# sample state=%d source=%d voice=%d remaining=%d frames=%d channels=%d\n",
                    state, sampling.source, sampling.source_voice,
                    atomic_load_int(&sampling.frames),
                    state == SAMPLE_STATE_COMPLETE ? sampling.len : 0,
                    sampling.channels);
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
    case ATOM4('W*--'):  // get a wavetable parameter to a variable
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
          case 3: // loop start boundary
            val = sw.loop_start[wave];
            break;
          case 4: // loop end boundary
            val = sw.loop_end[wave];
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
          ctx->printf(ctx, "# W* %d %d -> %g\n", wave, param, val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      break;
    case ATOM4('v*--'):  // get a voice parameter to a variable
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
          ctx->printf(ctx, "# v* %d -> %g\n", x, val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      break;
    case ATOM4('*=--'):  // variable-times-equal slot val0 val1
      if (argc > 2) {
        double val = arg[1] * arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      break;
    case ATOM4('/=--'):  // variable-divide-equal slot val0 val1
      if (argc > 2 && arg[2] != 0.0) {
        double val = arg[1] / arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      break;
    case ATOM4('a=--'):  // variable-plus-equal slot val0 val1
      if (argc > 2) {
        double val = arg[1] + arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      break;
    case ATOM4('s=--'):  // variable-sub-equal slot val0 val1
      if (argc > 2) {
        double val = arg[1] - arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      break;
    case ATOM4('=---'):  // variable-set slot value
      if (argc > 1) {
        ands_set_local(ctx->parse, x, arg[1]);
        ands_arg_clear(s);
        ands_arg_push(s, arg[1]);
        return 1;
      }
      else if (argc == 1) {
        double f = ands_get_local(ctx->parse, x);
        ctx->printf(ctx, "# $%d %g\n", x, f);
        ands_arg_clear(s);
        ands_arg_push(s, f);
        return 1;
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
    case ATOM4('%z--'): // mount zip-or-directory asset root
      if (strlen(ands_string(ctx->parse))) {
        if (skred_vfs_mount(ands_string(ctx->parse)))
          ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
        else
          ctx->printf(ctx, "# cannot mount %s\n", ands_string(ctx->parse));
      } else {
        ctx->printf(ctx, "# %%z requires [zip-or-directory]\n");
      }
      break;
    case ATOM4('%zu-'): // unmount zip asset root
      skred_vfs_unmount();
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      break;
    case ATOM4('%pwd'): // show vfs working directory
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      break;
    case ATOM4('%cat'): // print a text file
      if (strlen(ands_string(ctx->parse))) {
        void *data = NULL;
        size_t size = 0;
        char resolved[1024];
        if (skode_asset_read(ands_string(ctx->parse), SKODE_ASSET_ANY,
            &data, &size, resolved, sizeof(resolved))) {
          const char *text = (const char *)data;
          size_t pos = 0;
          while (pos < size) {
            char line[1024];
            size_t start = pos;
            size_t len;
            while (pos < size && text[pos] != '\n' && text[pos] != '\r') pos++;
            len = pos - start;
            while (pos < size && (text[pos] == '\n' || text[pos] == '\r')) pos++;
            if (len >= sizeof(line)) len = sizeof(line) - 1;
            memcpy(line, text + start, len);
            line[len] = '\0';
            for (size_t i = 0; i < len; i++) {
              if (!isprint((unsigned char)line[i]) && line[i] != '\t') {
                line[i] = '\0';
                break;
              }
            }
            ctx->printf(ctx, "%s\n", line);
          }
          skred_vfs_free_file(data);
        }
      }
      break;
    case ATOM4('%cd-'): // change directory
      ctx->printf(ctx, "# [%s] %%cd\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        if (!skred_chdir(ands_string(ctx->parse)))
          ctx->printf(ctx, "# cannot cd %s\n", ands_string(ctx->parse));
      }
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      break;
    case ATOM4('%ls-'): // list directory (match-type)
      {
      /*
          types
          0 = .sk
          1 = .wav
          2 = .mp3
          3 = .ks
          4 = .flac
      */
      int p = -1;
      if (argc) p = x;
      SkredDirent *entry;
      SkredDir *dp = skred_opendir(".");
        if (dp) {
          while ((entry = skred_readdir(dp))) {
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
              case 4:
                f = (strstr(name, ".flac") != NULL);
                break;
            }
            if (f) ctx->printf(ctx, "# [%s%s]\n", name,
              entry->is_directory ? "/" : "");
          }
          skred_closedir(dp);
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
  if (mode == '+') delay *= (tempo_step_seconds_get() * 4.0f);
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
    case GOT_RETURN_REF: { if (ctx->trace) ctx->printf(ctx, "# -> @return\n"); } break;
    case MACRO_DEFINED: {
      int index = ands_last_macro_index(s);
      char name[ANDS_MACRO_NAME_LEN];
      char body[ANDS_MACRO_BODY_LEN];
      int argc = 0;
      if (index >= 0 && ands_macro_get(s, index, name, sizeof(name),
          body, sizeof(body), &argc)) {
        skode_vocab_t *vocab = skode_dict_global_vocab();
        skode_dict_unpromote_macro(vocab, name);
        skode_compile_result_t result =
          skode_dict_macro_compile_status(vocab, body);
        int status =
          result == SKODE_COMPILE_OK ? ANDS_MACRO_REALTIME :
          result == SKODE_COMPILE_IMMEDIATE_ONLY ? ANDS_MACRO_IMMEDIATE :
          result == SKODE_COMPILE_TOO_LARGE ? ANDS_MACRO_TOO_LARGE :
          ANDS_MACRO_INVALID;
        if (status == ANDS_MACRO_REALTIME &&
            !skode_dict_promote_macro(vocab, name, body))
          status = ANDS_MACRO_INVALID;
        ands_macro_set_status(s, index, status);
        if (ctx->trace)
          ctx->printf(ctx, "# macro [%s] %s\n", name,
            status == ANDS_MACRO_REALTIME ? "realtime" :
            status == ANDS_MACRO_IMMEDIATE ? "immediate" :
            status == ANDS_MACRO_TOO_LARGE ? "too-large" : "invalid");
      }
      break;
    }
    case MACRO_REMOVING: {
      int index = ands_last_macro_index(s);
      char name[ANDS_MACRO_NAME_LEN];
      if (index >= 0 && ands_macro_get(s, index, name, sizeof(name),
          NULL, 0, NULL))
        skode_dict_unpromote_macro(skode_dict_global_vocab(), name);
      break;
    }
    default: return skode_unknown(ctx, s, info);
  }
  return 0;
}

double global_var[ANDS_VAR_MAX];


int skode_consume(char *line, skode_t *ctx) {
  if (!line || !ctx) return -1;
  skode_global_init();
  /* SKODE_EMPTY() contexts intentionally initialize their parser lazily.
     Keep dictionary setup on that same path: API-owned contexts such as
     skred_command() are not passed through skode_init() first. */
  skode_dict_init();
  if (ctx->parse == NULL) {
    // TODO this should live in wire-init or similar
    ctx->parse = ands_new(skode_callback, (void *)ctx);
    if (!ctx->parse) return -1;
    ands_set_global(ctx->parse, global_var);
  }
  skode_log_reset(ctx);
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
  ctx->printf(ctx, "# %s\n", skred_performance_status());
  return 0;
}

void skode_init(skode_t *ctx) {
  skode_global_init();
  skode_dict_init();
  ctx->vocab = NULL;
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
  skode_log_reset(ctx);
  ctx->ks = NULL;
  ctx->ks_result = NULL;
  ctx->udp = 0;
  ctx->which = 0;
  ctx->ip = 0;
  ctx->port = 0;
}

void skode_free(skode_t *ctx) {
  if (!ctx) return;
  if (ctx->vocab) {
    skode_dict_vocab_destroy(ctx->vocab);
    ctx->vocab = NULL;
  }
  if (ctx->parse) {
    ands_free(ctx->parse);
    ctx->parse = NULL;
  }
  if (ctx->ks_result && ctx->ks) {
    k_free(ctx->ks, (K)ctx->ks_result);
    ctx->ks_result = NULL;
  }
  if (ctx->ks) {
    ks_destroy(ctx->ks);
    ctx->ks = NULL;
  }
}

/* Dictionary command documentation remains here so kit_tool includes it in
   the generated command reference alongside the legacy switch commands. */

/* API/device command records live after the legacy records so adding them
   cannot renumber the established numeric help categories. */


/* Keep this new category after the legacy help records. Numeric help category
   indices are part of the command interface and must remain stable. */
