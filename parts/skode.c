double skode_stream_pull(void *ctx, int n);
#include "skode-internal.h"

char *skred_performance_status(void);




#ifdef KSYNTH
#include "vendor/ksynth/ksynth.h"
#endif
#include "recorder.h"
#ifdef SCOPE
#include "scope-ipc.h"
#endif

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




#define SKODE_CTX_MAX (100)
static skode_t *skode_ctx[SKODE_CTX_MAX];

char _skode_extra[STRING_BUF_IDX_MAX][STRING_BUF_LEN];
static char _skode_extra_invalid[STRING_BUF_LEN];
simple_mutex_t skode_extra_mutex;
#ifdef KSYNTH
simple_mutex_t skode_ks_eval_mutex;
#endif
static atomic_int_t skode_global_state;
int skode_extra_valid(int n) { return n >= 0 && n < STRING_BUF_IDX_MAX; }
char *skode_extra_ptr(int n) {
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
    #ifdef KSYNTH
    simple_mutex_init(&skode_ks_eval_mutex);
    #endif
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

int skode_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

int skode_wave_valid(int wave) {
  return wave >= 0 && wave < synth_config.wave_table_max;
}

int skode_double_to_int(double value, int *out) {
  if (!out || !isfinite(value) || value < INT_MIN || value > INT_MAX) return 0;
  *out = (int)value;
  return 1;
}


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

int skode_help_categories(char categories[][96],
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
  
  // Check skode_dict for arity
  const char *lookup_name = name[0] ? name : doc->key;
  const skode_word_t *word = skode_dict_find_by_name(ctx->vocab, lookup_name);
  if (word) {
    if (word->min_args == word->max_args) {
      if (word->min_args == 0) {
        ctx->printf(ctx, " [arity: 0 args]");
      } else if (word->min_args == 1) {
        ctx->printf(ctx, " [arity: 1 arg]");
      } else {
        ctx->printf(ctx, " [arity: %d args]", word->min_args);
      }
    } else {
      ctx->printf(ctx, " [arity: %d-%d args]", word->min_args, word->max_args);
    }
  }
  
  ctx->puts(ctx, "");
  if (summary[0]) ctx->printf(ctx, "#   %s\n", summary);
  if (word && word->help_detail) {
    ctx->printf(ctx, "#\n");
    const char *p = word->help_detail;
    while (*p) {
      const char *end = strchr(p, '\n');
      if (end) {
        ctx->printf(ctx, "#   %.*s\n", (int)(end - p), p);
        p = end + 1;
      } else {
        ctx->printf(ctx, "#   %s\n", p);
        break;
      }
    }
  }
  if (doc->file) ctx->printf(ctx, "#   %s:%d\n", doc->file, doc->line);
}


const char *skode_help_category_for_word(const char *name) {
  if (!name) return NULL;
  for (int i = 0; skode_doc_entries[i].key; i++) {
    if (!skode_help_is_command_doc(&skode_doc_entries[i])) continue;
    char doc_name[SKODE_HELP_FIELD_MAX];
    if (!skode_help_field(&skode_doc_entries[i], "name", doc_name, sizeof(doc_name))) continue;
    if (strcmp(doc_name, name) == 0) {
      static char cat_buffer[SKODE_HELP_FIELD_MAX];
      if (skode_help_field(&skode_doc_entries[i], "category", cat_buffer, sizeof(cat_buffer))) {
        return cat_buffer;
      }
      return NULL;
    }
  }
  return NULL;
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

void skode_help(skode_t *ctx, double *arg, int argc) {
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

int skode_seconds_to_samples(double seconds, uint64_t *out) {
  if (!out || !isfinite(seconds) || seconds < 0.0) return 0;
  long double samples = (long double)seconds * (long double)MAIN_SAMPLE_RATE;
  *out = samples >= (long double)UINT64_MAX ? UINT64_MAX : (uint64_t)samples;
  return 1;
}

uint64_t skode_u64_add(uint64_t a, uint64_t b) {
  return a > UINT64_MAX - b ? UINT64_MAX : a + b;
}

void skode_copy_string(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  snprintf(dst, dst_size, "%s", src ? src : "");
}

void skode_format_string_args(char *dst, size_t dst_size,
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

#ifdef UDP
#include "udp.h"
#endif






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











__attribute__((unused)) static const char *skode_wave_display_name(void) {
    return skode_wave_display_use_braille() ? "braille" : "ascii";
}




/**
 * Prints a connected audio waveform using Braille patterns.
 * Draws a single connected trace.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>






/* Tiny iterative radix-2 Cooley-Tukey FFT, in place. n must be a power of two. */



/* SKRED_SPECTROGRAM_COLOR: "256" (default) -- the 6x6x6 cube is plenty of
   steps for a heat ramp at glyph size and costs half the bytes of truecolor.
   "truecolor"/"24bit" opts into full RGB if you want it and can afford the
   line length. "none"/"mono"/"0" disables color. */

/* Inferno-ish heat ramp: t in [0,1] -> RGB */


/* 4x4 Bayer matrix, used to dither magnitude into on/off dots so the
   braille cells carry fine texture on top of the per-cell color. */


#define SPECTRO_NOISE_FLOOR_DB -60.0f

/* Conservative worst-case row length in bytes if EVERY cell happened to
   change color (never actually true after quantization, but cheap and
   safe to assume for sizing purposes). Set SPECTRO_LOG_LINE_BUDGET to a
   little under your actual SKODE_LOG_LINE_MAX (see skode.h), or override
   per-run with SKRED_SPECTROGRAM_LINE_BUDGET, if you want more headroom. */
#ifndef SPECTRO_LOG_LINE_BUDGET
#define SPECTRO_LOG_LINE_BUDGET 400
#endif



/* Steps color_mode down (2->1->0) until the worst-case row estimate fits
   the budget. Prints one short note on the way down so a truncated/garbled
   display isn't a silent mystery. */

/* Same prototype as wavetable_waveform_show(). loop_start/loop_end are
   drawn as a marker bar under the plot, same as the waveform view. */

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

int skode_sample_alloc(int frames) {
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

/* @doc
## enabled features
@enddoc */

/* @doc
### time-based ampltude envelope (ADSR)
@enddoc */

/* @doc
### amplitude modulation (AM)
@enddoc */

/* @doc
### phase distortion (CZ)
@enddoc */

/* @doc
### frequency modulation
@enddoc */

/* @doc
### multi-mode resonant filtering
@enddoc */

/* @doc
### time-based filter envelope (ADSR)
@enddoc */

/* @doc
### frequency sweeping (glide or portamento)
@enddoc */

/* @doc
### sample-and-hold
@enddoc */

/* @doc
### pan modulation
@enddoc */

/* @doc
### bit-depth reduction (bit-crush)
@enddoc */

/* @doc
### amplitude slew-rate (smoother)
@enddoc */

/* @doc
### recording
@enddoc */

#ifdef UDP
/* @doc
### skode UDP server
@enddoc */
#endif

/* @doc
### patterns and events
@enddoc */

#ifdef SCOPE
/* @doc
### waveform scope shared memory
@enddoc */
#endif

/* @doc
### benchmarking tools
@enddoc */



float record_frame_mono(int frame) {
  int channels = sampling.channels == 2 ? 2 : 1;
  size_t index = (size_t)frame * (size_t)channels;
  if (channels == 1) return sampling.where[index];
  return 0.5f * (sampling.where[index] + sampling.where[index + 1]);
}














    /* @doc(command./md)
    name: /md
    category: runtime
    summary: midi-debug-mode bool
    @enddoc */

































    /* @doc(command.wait)
    name: wait
    category: parser
    summary: blocking msec wait
    @enddoc */

    /* @doc(command.clr)
    name: clr
    category: parser
    summary: clear parser argument stack
    @enddoc */

    /* @doc(command.drop)
    name: drop
    category: parser
    summary: drop first parser argument
    @enddoc */

    /* @doc(command.dup)
    name: dup
    category: parser
    summary: duplicate first parser argument
    @enddoc */

    /* @doc(command.over)
    name: over
    category: parser
    summary: duplicate second parser argument to front
    @enddoc */

    /* @doc(command.rot)
    name: rot
    category: parser
    summary: rotate first three parser arguments left
    @enddoc */

    /* @doc(command.swap)
    name: swap
    category: parser
    summary: swap first two parser arguments
    @enddoc */

    /* @doc(command.a)
    name: a
    category: voice
    summary: amp loudness
    @enddoc */
/* @doc
`a` set voice loudness (amplitude) in dB
@enddoc */
    /* @doc(command.ab)
    name: ab
    category: voice
    summary: amp bend (-1..1)
    @enddoc */

    /* @doc(command.abp)
    name: abp
    category: voice
    summary: amp bend range (dB) [offset]
    @enddoc */

    /* @doc(command.A)
    name: A
    category: modulation
    summary: AM voice depth
    @enddoc */

    /* @doc(command.b)
    name: b
    category: wave
    summary: wave-direction mode
    @enddoc */

    /* @doc(command.B)
    name: B
    category: wave
    summary: wave-loop bool
    @enddoc */

    /* @doc(command.BC)
    name: BC
    category: wave
    summary: bounded one-shot loop count
    @enddoc */

    /* @doc(command.c)
    name: c
    category: modulation
    summary: phase-distortion algo distortion. Use w47 (Cosine) for authentic Casio CZ phase-distortion mimicking!
    @enddoc */

    /* @doc(command.C)
    name: C
    category: modulation
    summary: PD-mod voice depth
    @enddoc */

    /* @doc(command.ct)
    name: ct
    category: modulation
    summary: phase-distortion ADSR A D S R
    @enddoc */





    /* @doc(command.cte)
    name: cte
    category: modulation
    summary: phase-distortion envelope multistage set (array)
    @enddoc */

    /* @doc(command.cd)
    name: cd
    category: modulation
    summary: phase-distortion envelope depth
    @enddoc */

    /* @doc(command.D)
    name: D
    category: data
    summary: data-size
    @enddoc */

    /* @doc(command.MO)
    name: MO
    category: midi
    summary: send one to three raw MIDI bytes
    @enddoc */

    /* @doc(command.ce)
    name: ce
    category: events
    summary: control-plane user event id [value0 [value1 [value2]]]
    @enddoc */

    /* @doc(command.?d)
    name: ?d
    category: data
    summary: show-skode-data (summary)
    @enddoc */

    /* @doc(command.f)
    name: f
    category: voice
    summary: freq hz
    @enddoc */
    /* @doc(command.fb)
    name: fb
    category: voice
    summary: freq bend (-1..1)
    @enddoc */

    /* @doc(command.fbp)
    name: fbp
    category: voice
    summary: freq bend range (semitones) [offset]
    @enddoc */

    /* @doc(command.ft)
    name: ft
    category: filter
    summary: filter-adsr A D S R
    @enddoc */

    /* @doc(command.fte)
    name: fte
    category: filter
    summary: filter envelope multistage set (array)
    @enddoc */

    /* @doc(command.fd)
    name: fd
    category: filter
    summary: filter-adsr depth
    @enddoc */

    /* @doc(command.F)
    name: F
    category: modulation
    summary: FM voice depth
    @enddoc */

    /* @doc(command.FF)
    name: FF
    category: modulation
    summary: FM mode
    @enddoc */

    /* @doc(command.FB)
    name: FB
    category: modulation
    summary: FF2 operator feedback amount
    @enddoc */

    /* @doc(command.g)
    name: g
    category: modulation
    summary: glissando speed
    @enddoc */

    /* @doc(command.G)
    name: G
    category: modulation
    summary: link-midi voice [voice]
    @enddoc */

    /* @doc(command.h)
    name: h
    category: wave
    summary: sample-hold ratio [ratio] [mode]; if mode omitted, keeps current. ratio 0.0-1.0+, mode 0=hard, 1=smoothed, 2=jittered
    @enddoc */

    /* @doc(command.H)
    name: H
    category: modulation
    summary: link-velo voice [voice [voice [voice]]]
    @enddoc */

    /* @doc(command./D)
    name: /D
    category: data
    summary: resize-data count
    @enddoc */

    /* @doc(command.I)
    name: I
    category: runtime
    summary: log-event bool
    @enddoc */

    /* @doc(command.L)
    name: L
    category: modulation
    summary: link-trigger-delay seconds
    @enddoc */

    /* @doc(command.J)
    name: J
    category: filter
    summary: filter-mode selector [mode] [character]; if character omitted, keeps current. mode 1=LP, 2=HP, 3=BP, 4=Notch, 5=Allpass. character 0=clean, 1=driven, 2=screamer
    @enddoc */

    /* @doc(command.K)
    name: K
    category: filter
    summary: filter-cutoff freq
    @enddoc */

#ifdef KSYNTH
    /* @doc(command./ks)
    name: /ks
    category: ksynth
    summary: ksynth-load num (verbose)
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command./k)
    name: /k
    category: ksynth
    summary: ksynth-load num (verbose)
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.ks)
    name: ks
    category: ksynth
    summary: run ksynth code in string buffer
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.k!)
    name: k!
    category: ksynth
    summary: run ksynth code in string buffer
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.kw)
    name: kw
    category: ksynth
    summary: wait for last ksynth request [timeout-ms]
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.kw>)
    name: kw>
    category: ksynth
    summary: compatibility: copy latest ksynth result to data
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.k?)
    name: k?
    category: ksynth
    summary: k show last results
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.k>d)
    name: k>d
    category: ksynth
    summary: k results to d?
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.k>w)
    name: k>w
    category: ksynth
    summary: load latest ksynth result into wave slot rate? mode? offset?
    @enddoc */
#endif

    /* @doc(command.k)
    name: k
    category: misc
    summary: adsr-mode bool
    @enddoc */

#ifdef UDP
    /* @doc(command.udp)
    name: udp
    category: runtime
    summary: show-udp
    @enddoc */
#endif

    /* @doc(command.log)
    name: log
    category: runtime
    summary: log-enable bool
    @enddoc */

    /* @doc(command.___l)
    name: ___l
    category: voice
    summary: delayed velocity amount (doesn't propogate)
    @enddoc */

    /* @doc(command.l)
    name: l
    category: voice
    summary: velocity amount
    @enddoc */

    /* @doc(command.m)
    name: m
    category: voice
    summary: mute-audio bool
    @enddoc */
    /* @doc(command.M)
    name: M
    category: sequencer
    summary: tempo bpm
    @enddoc */

    /* @doc(command.n)
    name: n
    category: voice
    summary: midi-freq note-number (cents)
    @enddoc */
    /* @doc(command.N)
    name: N
    category: voice
    summary: detune-midi key cents
    @enddoc */

    /* @doc(command.p)
    name: p
    category: voice
    summary: pan value
    @enddoc */
    /* @doc(command.ds)
    name: ds
    category: modulation
    summary: track-delay send amount; active only for routed, centered, unmodulated voices
    @enddoc */

    /* @doc(command.DG)
    name: DG
    category: modulation
    summary: track-delay grit track [bits] [native]; bits=0 default(12), 1-16 explicit depth, native=1 bypasses quantization entirely
    @enddoc */

    /* @doc(command.DL)
    name: DL
    category: modulation
    summary: track-delay params track coarse fine feedback mod-freq mod-depth level
    @enddoc */

    /* @doc(command.DL?)
    name: DL?
    category: modulation
    summary: show track delay params
    @enddoc */

    /* @doc(command.DD)
    name: DD
    category: modulation
    summary: track-delay damping track [damping] [hp]; darkens/thins the feedback repeats
    @enddoc */

    /* @doc(command.DF)
    name: DF
    category: modulation
    summary: track-delay freeze track [0|1]; holds the current loop, stops writing new input
    @enddoc */

    /* @doc(command.DP)
    name: DP
    category: modulation
    summary: track-delay pingpong track [0|1]; cross-feeds L/R feedback
    @enddoc */

    /* @doc(command.DT)
    name: DT
    category: modulation
    summary: track-delay time-ms track ms; sets delay time directly in milliseconds
    @enddoc */

    /* @doc(command.DS)
    name: DS
    category: modulation
    summary: track-delay tempo-sync track bpm division; 1.0=quarter, 0.5=eighth, 0.75=dotted-eighth
    @enddoc */

    /* @doc(command.GS)
    name: GS
    category: voice
    summary: show global synth status
    @enddoc */

    /* @doc(command.GS>)
    name: GS>
    category: files
    summary: save complete repl session zip using parser string filename
    @enddoc */

    /* @doc(command.GS<)
    name: GS<
    category: files
    summary: restore complete repl session zip using parser string filename
    @enddoc */

    /* @doc(command.P)
    name: P
    category: modulation
    summary: pan-mod voice depth
    @enddoc */

    /* @doc(command.q)
    name: q
    category: wave
    summary: bit-crush bit-depth [bits] [curve]; if curve omitted, keeps current. curve 0=linear, 1=companded, 2=dithered
    @enddoc */

    /* @doc(command.Q)
    name: Q
    category: filter
    summary: filter resonance
    @enddoc */

    /* @doc(command.r)
    name: r
    category: routing
    summary: route voice to track, 0=master only, 1..4=track/delay bus
    @enddoc */

    /* @doc(command.rt)
    name: rt
    category: routing
    summary: track-name track
    @enddoc */

    /* @doc(command.rv)
    name: rv
    category: routing
    summary: track-volume track dB
    @enddoc */

    /* @doc(command.R!)
    name: R!
    category: sequencer
    summary: remove-events tag
    @enddoc */

    /* @doc(command.R!!)
    name: R!!
    category: sequencer
    summary: remove all queued events
    @enddoc */

    /* @doc(command.RR)
    name: RR
    category: sequencer
    summary: repeat-string-tempo count delay [tag]
    @enddoc */

    /* @doc(command.eRR)
    name: eRR
    category: sequencer
    summary: repeat-external-macro-tempo macro count beats [tag]
    @enddoc */

    /* @doc(command.eR)
    name: eR
    category: sequencer
    summary: repeat-external-macro macro count seconds [tag]
    @enddoc */

    /* @doc(command.DO?)
    name: DO?
    category: sequencer
    summary: conditional-string-if-gt-zero number [tag]
    @enddoc */

    /* @doc(command.R)
    name: R
    category: sequencer
    summary: repeat-string count delay [tag]
    @enddoc */

    /* @doc(command.s)
    name: s
    category: modulation
    summary: volume-smooth bool
    @enddoc */

    /* @doc(command.S)
    name: S
    category: voice
    summary: voice-reset voice
    @enddoc */

    /* @doc(command.t)
    name: t
    category: voice
    summary: adsr-set attack decay sustain release
    @enddoc */

    /* @doc(command.te)
    name: te
    category: voice
    summary: envelope multistage set (array)
    @enddoc */

    /* @doc(command.T)
    name: T
    category: voice
    summary: trigger
    @enddoc */

    /* @doc(command.v)
    name: v
    category: voice
    summary: voice-select voice
    @enddoc */
    /* @doc(command.vc)
    name: vc
    category: voice
    summary: voice control-plane event publication bool
    @enddoc */

    /* @doc(command.V)
    name: V
    category: voice
    summary: main-volume loudness
    @enddoc */

    /* @doc(command.vt)
    name: vt
    category: voice
    summary: [name] voice-text-set
    @enddoc */

    /* @doc(command.wt)
    name: wt
    category: misc
    summary: [name] wave-text-set wave-number
    @enddoc */

    /* @doc(command.WL)
    name: WL
    category: wave
    summary: wave-loop-points wave start end
    @enddoc */

    /* @doc(command.VS)
    name: VS
    category: wave
    summary: voice-set-points start end; no args resets from wave
    @enddoc */

    /* @doc(command.VL)
    name: VL
    category: wave
    summary: voice-loop-points start end; no args resets from wave
    @enddoc */

    /* @doc(command.VW)
    name: VW
    category: wave
    summary: voice-wave-show [voice] [width height]
    @enddoc */

    /* @doc(command.w)
    name: w
    category: wave
    summary: wave-select which-wave interpolate? mode-override?
    @enddoc */

    /* @doc(command.=d)
    name: =d
    category: data
    summary: assign a variable from an element of the d array =d var d-index
    @enddoc */

    /* @doc(command.d!)
    name: d!
    category: data
    summary: write a value into the d array at index: val index d!
    @enddoc */

    /* @doc(command.d*)
    name: d*
    category: data
    summary: show an element from d array
    @enddoc */

    /* @doc(command.d>r)
    name: d>r
    category: data
    summary: data-to-rec
    @enddoc */

    /* @doc(command.r>d)
    name: r>d
    category: data
    summary: recording-to-data channel
    @enddoc */

    /* @doc(command.d>MO)
    name: d>MO
    category: midi
    summary: send the data array as raw MIDI bytes
    @enddoc */

#ifdef KSYNTH
    /* @doc(command.d>k)
    name: d>k
    category: ksynth
    summary: data-to-ksynth-variable
    @enddoc */
#endif

#ifdef KSYNTH
    /* @doc(command.w>k)
    name: w>k
    category: ksynth
    summary: wavetable-to-ksynth-variable
    @enddoc */
#endif

    /* @doc(command.w>d)
    name: w>d
    category: wave
    summary: wave-to-data
    @enddoc */

    /* @doc(command.w>r)
    name: w>r
    category: wave
    summary: wave-to-rec
    @enddoc */

    /* @doc(command.w>w)
    name: w>w
    category: wave
    summary: write wavetable to string-named WAV file
    @enddoc */

    /* @doc(command.w!)
    name: w!
    category: wave
    summary: wave-lock
    @enddoc */

    /* @doc(command.w*)
    name: w*
    category: wave
    summary: wave-nudge-reset
    @enddoc */

    /* @doc(command.w>)
    name: w>
    category: wave
    summary: wave-nudge-start
    @enddoc */

    /* @doc(command.w<)
    name: w<
    category: wave
    summary: wave-nudge-len
    @enddoc */

    /* @doc(command.w<>)
    name: w<>
    category: wave
    summary: wave-auto-trim
    @enddoc */

    /* @doc(command.WS)
    name: WS
    category: wave-specto
    summary: wave-show which-wave
    @enddoc */

    /* @doc(command.W)
    name: W
    category: wave
    summary: wave-show which-wave
    @enddoc */

    /* @doc(command.xg)
    name: xg
    category: sequencer
    summary: goto-step #
    @enddoc */

    /* @doc(command.>x)
    name: >x
    category: sequencer
    summary: goto-step #
    @enddoc */

    /* @doc(command.xa)
    name: xa
    category: sequencer
    summary: append step
    @enddoc */

    /* @doc(command.<x)
    name: <x
    category: sequencer
    summary: (pattern) step-string-to-skode step-number
    @enddoc */

    /* @doc(command.EXEC)
    name: EXEC
    category: parser
    summary: numeric opcode escape
    @enddoc */
    /* @doc(command.x)
    name: x
    category: sequencer
    summary: set-step-string step
    @enddoc */

    /* @doc(command.y)
    name: y
    category: sequencer
    summary: select-pattern which
    @enddoc */

    /* @doc(command.ys?)
    name: ys?
    category: sequencer
    summary: pattern dump for skrepl grid state
    @enddoc */

    /* @doc(command.yt)
    name: yt
    category: sequencer
    summary: {note} pattern-text
    @enddoc */

    /* @doc(command.ym)
    name: ym
    category: sequencer
    summary: pattern-mute 0/1
    @enddoc */

    /* @doc(command.yc)
    name: yc
    category: sequencer
    summary: pattern control-plane event publication bool
    @enddoc */

    /* @doc(command.Y)
    name: Y
    category: sequencer
    summary: clear-pattern which
    @enddoc */

    /* @doc(command.z)
    name: z
    category: sequencer
    summary: one-pattern-play-mode bool
    @enddoc */

    /* @doc(command.zg)
    name: zg
    category: sequencer
    summary: goto-pattern-step step
    @enddoc */

    /* @doc(command.zq)
    name: zq
    category: sequencer
    summary: queue-pattern-start-stop mode
    @enddoc */

    /* @doc(command.z?)
    name: z?
    category: sequencer
    summary: one-pattern-play-mode bool
    @enddoc */

    /* @doc(command.Z)
    name: Z
    category: sequencer
    summary: all-pattern-play-mode bool
    @enddoc */

    /* @doc(command.z??)
    name: z??
    category: sequencer
    summary: show all patterns
    @enddoc */

    /* @doc(command.Z?)
    name: Z?
    category: sequencer
    summary: show all patterns
    @enddoc */

    /* @doc(command.XM)
    name: XM
    category: modulation
    summary: ring modulation osc amount
    @enddoc */

    /* @doc(command.v?)
    name: v?
    category: voice
    summary: show-voice
    @enddoc */

    /* @doc(command.?)
    name: ?
    category: voice
    summary: show-voice
    @enddoc */

    /* @doc(command.backslash)
    name: \
    category: voice
    summary: verbose-show-voice
    @enddoc */

    /* @doc(command.v??)
    name: v??
    category: voice
    summary: show-active-voices
    @enddoc */

    /* @doc(command.??)
    name: ??
    category: voice
    summary: show-active-voices
    @enddoc */

    /* @doc(command.?r)
    name: ?r
    category: routing
    summary: show track routing
    @enddoc */

    /* @doc(command.?s)
    name: ?s
    category: misc
    summary: show-skode-string
    @enddoc */

    /* @doc(command.s?)
    name: s?
    category: macros
    summary: show parser-local string slot [index]
    @enddoc */

    /* @doc(command.?m)
    name: ?m
    category: macros
    summary: show-ands-macros
    @enddoc */

    /* @doc(command.?ce)
    name: ?ce
    category: events
    summary: show control-plane event snapshot
    @enddoc */

    /* @doc(command.?ce!)
    name: ?ce!
    category: events
    summary: clear outstanding control-plane events
    @enddoc */

    /* @doc(command.?q)
    name: ?q
    category: sequencer
    summary: show scheduled opcode queue
    @enddoc */

    /* @doc(command.?o)
    name: ?o
    category: sequencer
    summary: show compiled opcode queue or pattern
    @enddoc */

    /* @doc(command./m_)
    name: /m_
    category: runtime
    summary: benchmark voice
    @enddoc */

    /* @doc(command./q)
    name: /q
    category: runtime
    summary: quit
    @enddoc */

#ifdef SCOPE
    /* @doc(command./sg)
    name: /sg
    category: scope
    summary: start shared-memory scope publication
    @enddoc */
#endif

#ifdef SCOPE
    /* @doc(command./ss)
    name: /ss
    category: scope
    summary: stop shared-memory scope publication
    @enddoc */
#endif

#ifdef SCOPE
    /* @doc(command./s?)
    name: /s?
    category: scope
    summary: shared-memory scope status
    @enddoc */
#endif

    /* @doc(command./rg)
    name: /rg
    category: recording
    summary: start multitrack file recording
    @enddoc */

    /* @doc(command./rs)
    name: /rs
    category: recording
    summary: stop multitrack file recording
    @enddoc */

    /* @doc(command./r?)
    name: /r?
    category: recording
    summary: multitrack file recording status
    @enddoc */

    /* @doc(command./r)
    name: /r
    category: recording
    summary: sample-to-wave slot mode channel
    @enddoc */

    /* @doc(command./d)
    name: /d
    category: data
    summary: data-to-wave slot rate mode offset
    @enddoc */

    /* @doc(command./f)
    name: /f
    category: runtime
    summary: flag-mode num
    @enddoc */

    /* @doc(command./ff)
    name: /ff
    category: runtime
    summary: foreign C function slot arg...
    @enddoc */

    /* @doc(command./m)
    name: /m
    category: macros
    summary: remove-ands-macro [name]
    @enddoc */

    /* @doc(command./m!)
    name: /m!
    category: macros
    summary: clear-ands-macros
    @enddoc */

    /* @doc(command./t)
    name: /t
    category: runtime
    summary: trace-mode num
    @enddoc */

    /* @doc(command./v)
    name: /v
    category: runtime
    summary: verbose-mode num
    @enddoc */

    /* @doc(command./cer)
    name: /cer
    category: events
    summary: control-event responder bool
    @enddoc */

    /* @doc(command./ce?)
    name: /ce?
    category: events
    summary: control-event responder status
    @enddoc */

    /* @doc(command./th?)
    name: /th?
    category: runtime
    summary: skred service/thread health
    @enddoc */

    /* @doc(command./th!)
    name: /th!
    category: runtime
    summary: reset skred performance counters and peak load tracking
    @enddoc */

    /* @doc(command./ce!)
    name: /ce!
    category: events
    summary: control-event responder remove/clear
    @enddoc */

    /* @doc(command./ceb)
    name: /ceb
    category: events
    summary: bind parser string to control event type key
    @enddoc */

    /* @doc(command./cex)
    name: /cex
    category: events
    summary: bind external string slot to control event type key
    @enddoc */

    /* @doc(command.<s)
    name: <s
    category: macros
    summary: parser-local string slot to parser string
    @enddoc */

    /* @doc(command.s>)
    name: s>
    category: macros
    summary: parser string to parser-local string slot
    @enddoc */

    /* @doc(command.s%)
    name: s%
    category: macros
    summary: format parser string with numeric args
    @enddoc */

    /* @doc(command.<e)
    name: <e
    category: macros
    summary: external-string-to-skode external-index
    @enddoc */

    /* @doc(command.e>)
    name: e>
    category: macros
    summary: skode-string-to-external external-index
    @enddoc */

    /* @doc(command.e!)
    name: e!
    category: macros
    summary: execute-string num
    @enddoc */

    /* @doc(command.e?)
    name: e?
    category: macros
    summary: show-execute-string [num]
    @enddoc */

    /* @doc(command./s)
    name: /s
    category: runtime
    summary: system-show num
    @enddoc */

    /* @doc(command./h)
    name: /h
    category: runtime
    summary: show command help
    @enddoc */

    /* @doc(command./l)
    name: /l
    category: files
    summary: skode-load num
    @enddoc */

    /* @doc(command./ls)
    name: /ls
    category: files
    summary: skode-load-string filename
    @enddoc */

    /* @doc(command./ws)
    name: /ws
    category: files
    summary: wave-load-string wave channel
    @enddoc */

    /* @doc(command./w)
    name: /w
    category: files
    summary: wave-load num wave channel
    @enddoc */

    /* @doc(command.>r)
    name: >r
    category: recording
    summary: normalize recording to string-named WAV file
    @enddoc */

    /* @doc(command.^r)
    name: ^r
    category: recording
    summary: record duration source voice ... markdown/html doesn't like <
    @enddoc */

    /* @doc(command.<r)
    name: <r
    category: recording
    summary: record duration source voice
    @enddoc */

    /* @doc(command.>)
    name: >
    category: voice
    summary: copy-voice dest-voice
    @enddoc */

    /* @doc(command./)
    name: /
    category: wave
    summary: default-wave voice
    @enddoc */

    /* @doc(command.%)
    name: %
    category: sequencer
    summary: pattern-modulus num
    @enddoc */

    /* @doc(command.W*)
    name: W*
    category: data
    summary: get a wavetable parameter to a variable
    @enddoc */

    /* @doc(command.v*)
    name: v*
    category: data
    summary: get a voice parameter to a variable
    @enddoc */

    /* @doc(command.*=)
    name: *=
    category: data
    summary: variable-times-equal slot val0 val1
    @enddoc */

    /* @doc(command./=)
    name: /=
    category: data
    summary: variable-divide-equal slot val0 val1
    @enddoc */

    /* @doc(command.a=)
    name: a=
    category: data
    summary: variable-plus-equal slot val0 val1
    @enddoc */

    /* @doc(command.s=)
    name: s=
    category: data
    summary: variable-sub-equal slot val0 val1
    @enddoc */

    /* @doc(command.=)
    name: =
    category: data
    summary: variable-set slot value
    @enddoc */

    /* @doc(command./wex)
    name: /wex
    category: wave
    summary: wave-expand wave
    @enddoc */

    /* @doc(command.%z)
    name: %z
    category: files
    summary: mount zip-or-directory asset root
    @enddoc */

    /* @doc(command.%zu)
    name: %zu
    category: files
    summary: unmount zip asset root
    @enddoc */

    /* @doc(command.%pwd)
    name: %pwd
    category: files
    summary: show vfs working directory
    @enddoc */

    /* @doc(command.%cat)
    name: %cat
    category: files
    summary: print a text file
    @enddoc */

    /* @doc(command.%cd)
    name: %cd
    category: files
    summary: change directory
    @enddoc */

    /* @doc(command.%ls)
    name: %ls
    category: files
    summary: list directory [match-type [index|-1] ]
    @enddoc */

void skode_register_immediate_words(skode_vocab_t *vocab) {
  skode_register_words_dsp(vocab);
  skode_register_words_seq(vocab);
  skode_register_words_data(vocab);
  skode_register_words_system(vocab);
  skode_register_words_misc(vocab);
}
int skode_function(ands_t *s, int info) {
  uint32_t atom = ands_atom_num(s);
  int argc = ands_arg_len(s);
  skode_t *ctx = (skode_t*)ands_user(s);
  double *arg = ands_arg(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  if (ctx->trace) {
    ctx->printf(ctx, "# SKODE_FUNCTION ");
    ctx->printf(ctx, "%s", ands_atom_string(s));
    if (argc) {
      for (int i=0; i<argc; i++) ctx->printf(ctx, " %g", arg[i]);
    }
    ctx->puts(ctx, "");
  }
  for (int i=0; i<argc; i++) {\
    int var = ands_arg_var(s, i);\
    if (var >= 0 && (var & ANDS_STREAM_FLAG)) {\
      arg[i] = skode_stream_pull(ctx, var & ~ANDS_STREAM_FLAG);\
    }\
  }
  int dict_result;
  if (skode_execute_word(ctx, s, atom, arg, argc, &dict_result))
    return dict_result;
  switch (atom) {
  default:
    ctx->printf(ctx, "# SKODE_UNKNOWN_FUNCTION %d [%x] :: %d", info, atom, argc);
    return -1;
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
skode_stream_t global_stream[ANDS_VAR_MAX];

void skode_stream_set(void *ctx, int n, const double *data, int len) {
    if (n < 0 || n >= 128) return;
    if (len > SKODE_STREAM_MAX_LEN) len = SKODE_STREAM_MAX_LEN;
    if (len > 0 && data) {
        memcpy(global_stream[n].data, data, len * sizeof(double));
    }
    global_stream[n].len = len;
    global_stream[n].pos = 0; global_stream[n].dir = 1;
}

void skode_stream_copy(void *ctx, int dst, int src) {
    if (dst < 0 || dst >= 128 || src < 0 || src >= 128) return;
    skode_stream_t *s_dst = &global_stream[dst];
    skode_stream_t *s_src = &global_stream[src];
    s_dst->len = s_src->len;
    s_dst->mode = s_src->mode;
    s_dst->pos = s_src->pos;
    s_dst->dir = s_src->dir;
    if (s_src->len > 0) {
        memcpy(s_dst->data, s_src->data, s_src->len * sizeof(double));
    }
}

void skode_stream_mode(void *ctx, int n, int mode) {
    if (n >= 0 && n < 128) global_stream[n].mode = mode;
}

void skode_stream_pos(void *ctx, int n, int pos) {
    if (n < 0 || n >= 128) return;
    global_stream[n].pos = pos;
    if (pos >= 0 && global_stream[n].len > 0) {
        global_stream[n].pos = pos % global_stream[n].len;
    } else {
        global_stream[n].pos = 0; global_stream[n].dir = 1;
    }
    global_stream[n].dir = 1;
}

double skode_stream_pull(void *ctx, int n) {
    if (n < 0 || n >= 128) return NAN;
    skode_stream_t *s = &global_stream[n];
    if (s->len <= 0 ) return 0.0;
    
    double val = s->data[s->pos];
    
    if (s->mode == 0) { // wrap forward
        s->pos = (s->pos + 1) % s->len;
    } else if (s->mode == 1) { // wrap backward
        s->pos = (s->pos - 1 + s->len) % s->len;
    } else if (s->mode == 2) { // ping-pong
        s->pos += s->dir;
        if (s->pos >= s->len) {
            s->pos = s->len > 1 ? s->len - 2 : 0;
            s->dir = -1;
        } else if (s->pos < 0) {
            s->pos = s->len > 1 ? 1 : 0;
            s->dir = 1;
        }
    } else if (s->mode == 3) { // clamp forward
        if (s->pos < s->len - 1) s->pos++;
    }
    return val;
}



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
  memset(&ctx->stack, 0, sizeof(ctx->stack));
  ctx->defer_sample_time = 0;
  ctx->defer_last = 0.0;
  ctx->pattern = 0;
  ctx->step = -1;
  ctx->trace = 0;
  ctx->verbose = 0;
  ctx->parse = NULL;
  ctx->quit = 0;
  ctx->puts = skode_puts;
  ctx->printf = skode_printf;
  ctx->output_user = NULL;
  ctx->log_enable = 0;
  ctx->log_max = SKODE_LOG_MAX;
  skode_log_reset(ctx);
  memset(ctx->string_slot, 0, sizeof(ctx->string_slot));
  ctx->flag = 0;
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
  if (ctx->upload_buffer) {
    free(ctx->upload_buffer);
    ctx->upload_buffer = NULL;
    ctx->upload_len = 0;
    ctx->upload_cap = 0;
  }
  if (ctx->parse) {
    ands_free(ctx->parse);
    ctx->parse = NULL;
  }
  #ifdef KSYNTH
  if (ctx->ks_result && ctx->ks) {
    k_free(ctx->ks, (K)ctx->ks_result);
    ctx->ks_result = NULL;
  }
  if (ctx->ks) {
    ks_destroy(ctx->ks);
    ctx->ks = NULL;
  }
  #endif
}

/* Dictionary command documentation remains here so kit_tool includes it in
   the generated command reference alongside the legacy switch commands. */
/* @doc(command.*R)
name: *R
category: parser
summary: return arguments as @0 through @9
@enddoc */
/* @doc(command.?R)
name: ?R
category: parser
summary: show return registers without consuming them
@enddoc */

/* API/device command records live after the legacy records so adding them
   cannot renumber the established numeric help categories. */
/* @doc
## API Commands (Runtime & Device Integration)

### Wave Upload Protocol
Allows interactive streaming of raw wave data into a wave slot using base64.
1. `-upwave START <compressed_size> <uncompressed_size>`
   Allocates a temporary session buffer for the UDP connection or WASM client.
2. `-upwave DATA <base64>`
   Streams compressed base64 chunks directly into the session buffer.
3. `-upwave COMMIT <slot>`
   Safely checks if the slot is in-use. If free, decompresses the payload using miniz and commits to the slot lock-free.
4. `-upwave CANCEL`
   Aborts the upload and safely frees the session buffer.

### Wave Download Protocol
1. `-wave <slot>`
   Dumps a wave slot to `stdout` in base64 format using `~WAVE:START`, `~WAVE:<data>`, and `~WAVE:END` boundaries for easy parsing by external API clients.
@enddoc */

/* @doc(command.-restart)
name: -restart
category: runtime
summary: restart the audio engine [voices=v] [frames=f] [port=p]
@enddoc */
/* @doc(command.-wave)
name: -wave
category: runtime
summary: dump wave slot as compressed base64
@enddoc */
/* @doc(command.-upwave)
name: -upwave
category: runtime
summary: stream wave data via START DATA CANCEL COMMIT
@enddoc */
/* @doc(command./als)
name: /als
category: runtime
summary: refresh and list audio input and output devices
@enddoc */
/* @doc(command./a?)
name: /a?
category: runtime
summary: show audio device and performance status
@enddoc */
/* @doc(command./ai)
name: /ai
category: runtime
summary: select audio input index (-1 default, -2 off)
@enddoc */
/* @doc(command./ao)
name: /ao
category: runtime
summary: select audio output index (-1 default)
@enddoc */

/* @doc(command./mL)
name: /mL
category: midi
summary: initialize MIDI and list input and output ports
@enddoc */

/* @doc(command./mls)
name: /mls
category: midi
summary: initialize MIDI and list input and output ports (alias for /mL)
@enddoc */
/* @doc(command./m?)
name: /m?
category: midi
summary: show MIDI ports, mask, routes, and binding counts
@enddoc */
/* @doc(command./mi)
name: /mi
category: midi
summary: open enumerated MIDI input port index
@enddoc */
/* @doc(command./mo)
name: /mo
category: midi
summary: open enumerated MIDI output port index
@enddoc */
/* @doc(command./miV)
name: /miV
category: midi
summary: create virtual MIDI input using parser string name
@enddoc */
/* @doc(command./moV)
name: /moV
category: midi
summary: create virtual MIDI output using parser string name
@enddoc */
/* @doc(command./mic)
name: /mic
category: midi
summary: close active MIDI input
@enddoc */
/* @doc(command./moc)
name: /moc
category: midi
summary: close active MIDI output
@enddoc */
/* @doc(command./mv)
name: /mv
category: midi
summary: route MIDI channel note and bend input to voice
@enddoc */
/* @doc(command./mp)
name: /mp
category: midi
summary: route MIDI channel note and bend input to poly pool
@enddoc */
/* @doc(command./mvd)
name: /mvd
category: midi
summary: delete MIDI channel-to-voice route
@enddoc */
/* @doc(command./mpd)
name: /mpd
category: midi
summary: delete MIDI channel-to-pool route
@enddoc */
/* @doc(command./mR)
name: /mR
category: midi
summary: list MIDI note and bend routes
@enddoc */
/* @doc(command./mC)
name: /mC
category: midi
summary: clear all MIDI note and bend routes
@enddoc */
/* @doc(command./mb)
name: /mb
category: midi
summary: bind parser string Skode template to filtered MIDI event
@enddoc */
/* @doc(command./mbd)
name: /mbd
category: midi
summary: delete filtered MIDI-to-Skode binding
@enddoc */
/* @doc(command./mb?)
name: /mb?
category: midi
summary: list MIDI-to-Skode bindings
@enddoc */
/* @doc(command./mbC)
name: /mbC
category: midi
summary: clear all MIDI-to-Skode bindings
@enddoc */

/* Keep this new category after the legacy help records. Numeric help category
   indices are part of the command interface and must remain stable. */
/* @doc(command./pg)
name: /pg
category: polyphony
summary: define voice group group source width [root-offset]
@enddoc */
/* @doc(command./pg!)
name: /pg!
category: polyphony
summary: refresh free instances using voice group
@enddoc */
/* @doc(command./pp)
name: /pp
category: polyphony
summary: define pool pool group base count [steal-policy]
@enddoc */
/* @doc(command./pp!)
name: /pp!
category: polyphony
summary: refresh free instances in pool
@enddoc */
/* @doc(command./pm)
name: /pm
category: polyphony
summary: pool mode pool mode [priority [articulation]]
@enddoc */
/* @doc(command.?pg)
name: ?pg
category: polyphony
summary: show voice groups
@enddoc */
/* @doc(command.?pp)
name: ?pp
category: polyphony
summary: show voice pools and allocations
@enddoc */
/* @doc(command./vg)
name: /vg
category: polyphony
summary: voice dependency graph voice [format [depth]]
@enddoc */
/* @doc(command.pn)
name: pn
category: polyphony
summary: pool note-on pool key note velocity [cents]
@enddoc */
/* @doc(command.pr)
name: pr
category: polyphony
summary: pool note release pool key [release-velocity]
@enddoc */
/* @doc(command.pb)
name: pb
category: polyphony
summary: pool pitch bend pool key semitones [cents]
@enddoc */


// Generate HTML representation of all help documentation
static char* skred_help_html_buffer = NULL;

const char* skred_help_as_html(void) {
  if (skred_help_html_buffer) return skred_help_html_buffer;
  
  char categories[32][96];
  int cat_count = skode_help_categories(categories, 32);
  
  // Allocate a generous buffer
  size_t size = 65536;
  skred_help_html_buffer = (char*)malloc(size);
  if (!skred_help_html_buffer) return "";
  skred_help_html_buffer[0] = '\0';
  
  char *ptr = skred_help_html_buffer;
  size_t rem = size;
  
  for (int i = 0; i < cat_count; i++) {
    int n = snprintf(ptr, rem, "<h3 align='center'>Skode Commands: %s</h3><table width='100%%' border='0' cellpadding='4'>", categories[i]);
    if (n < 0 || n >= (int)rem) break;
    ptr += n; rem -= n;
    
    for (int j = 0; skode_doc_entries[j].key; j++) {
      char doc_category[96] = "";
      char name[96] = "";
      char summary[256] = "";
      
      if (!skode_help_is_command_doc(&skode_doc_entries[j])) continue;
      if (!skode_help_field(&skode_doc_entries[j], "category", doc_category, sizeof(doc_category))) continue;
      if (strcmp(doc_category, categories[i]) != 0) continue;
      
      skode_help_field(&skode_doc_entries[j], "name", name, sizeof(name));
      skode_help_field(&skode_doc_entries[j], "summary", summary, sizeof(summary));
      
      const char* disp_name = name[0] ? name : skode_doc_entries[j].key;
      
      n = snprintf(ptr, rem, "<tr><td align='right' width='25%%'><b>%s</b></td><td>%s</td></tr>", disp_name, summary);
      if (n < 0 || n >= (int)rem) break;
      ptr += n; rem -= n;
    }
    
    n = snprintf(ptr, rem, "</table>");
    if (n < 0 || n >= (int)rem) break;
    ptr += n; rem -= n;
  }
  
  return skred_help_html_buffer;
}
