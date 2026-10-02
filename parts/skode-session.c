#include "skode-internal.h"

/*
 * GS session archives deliberately use an ordinary ZIP. Textual synth state
 * remains inspectable as state.sk, while arrays and command strings are kept
 * in exact, length-delimited entries so saving never changes sample values or
 * loses parser syntax through quoting.
 */
#define SKODE_SESSION_FORMAT 1U
#define SKODE_SESSION_K_MAGIC 0x4b535652U
#define SKODE_SESSION_WAVE_MAGIC 0x53574156U
#define SKODE_SESSION_RECORD_MAGIC 0x53524543U


typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t slot;
  int32_t length;
  float rate;
  int32_t one_shot;
  int32_t loop_enabled;
  int32_t loop_start;
  int32_t loop_end;
  float direction;
  float midi_note;
  float offset_hz;
  char name[WAVE_NAME_MAX];
} skode_session_wave_t;

typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t frames;
  int32_t channels;
  int32_t offset;
  int32_t trim;
  int32_t source;
  int32_t source_voice;
} skode_session_record_t;

typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t kind; /* 0 vector, 1 function */
  int32_t count; /* doubles for vectors, bytes for function source */
} skode_session_k_t;

typedef struct {
  uint32_t event_mask;
  int32_t debug;
} skode_session_midi_settings_t;

static int skode_session_buffer_reserve(skode_session_buffer_t *buffer,
    size_t extra) {
  if (!buffer || buffer->failed || extra > SIZE_MAX - buffer->size) return 0;
  size_t needed = buffer->size + extra;
  if (needed <= buffer->capacity) return 1;
  size_t capacity = buffer->capacity ? buffer->capacity : 4096;
  while (capacity < needed) {
    if (capacity > SIZE_MAX / 2) {
      capacity = needed;
      break;
    }
    capacity *= 2;
  }
  unsigned char *next = (unsigned char *)realloc(buffer->data, capacity);
  if (!next) {
    buffer->failed = 1;
    return 0;
  }
  buffer->data = next;
  buffer->capacity = capacity;
  return 1;
}

static int skode_session_buffer_append(skode_session_buffer_t *buffer,
    const void *data, size_t size) {
  if (size == 0) return 1;
  if (!data || !skode_session_buffer_reserve(buffer, size)) return 0;
  memcpy(buffer->data + buffer->size, data, size);
  buffer->size += size;
  return 1;
}

static int skode_session_puts(skode_t *ctx, const char *text) {
  skode_session_buffer_t *buffer =
    ctx ? (skode_session_buffer_t *)ctx->output_user : NULL;
  if (!buffer || !text) return -1;
  if (!skode_session_buffer_append(buffer, text, strlen(text)) ||
      !skode_session_buffer_append(buffer, "\n", 1)) return -1;
  return 0;
}

static int skode_session_printf(skode_t *ctx, const char *format, ...) {
  skode_session_buffer_t *buffer =
    ctx ? (skode_session_buffer_t *)ctx->output_user : NULL;
  if (!buffer || !format) return -1;
  va_list ap, copy;
  va_start(ap, format);
  va_copy(copy, ap);
  int needed = vsnprintf(NULL, 0, format, copy);
  va_end(copy);
  if (needed < 0 ||
      !skode_session_buffer_reserve(buffer, (size_t)needed + 1)) {
    va_end(ap);
    return -1;
  }
  (void)vsnprintf((char *)buffer->data + buffer->size,
    buffer->capacity - buffer->size, format, ap);
  va_end(ap);
  buffer->size += (size_t)needed;
  return 0;
}

static int skode_session_zip_add(mz_zip_archive *zip, const char *name,
    const void *data, size_t size) {
  static const unsigned char empty = 0;
  return zip && name &&
    mz_zip_writer_add_mem(zip, name, size ? data : &empty, size,
      MZ_BEST_COMPRESSION);
}

static void *skode_session_zip_read(mz_zip_archive *zip, const char *name,
    size_t *size) {
  if (size) *size = 0;
  if (!zip || !name) return NULL;
  return mz_zip_reader_extract_file_to_heap(zip, name, size, 0);
}

static int skode_session_zip_has(mz_zip_archive *zip, const char *name) {
  return zip && name &&
    mz_zip_reader_locate_file(zip, name, NULL, 0) >= 0;
}

#ifdef KSYNTH
static int skode_session_k_valid(const void *data, size_t size) {
  if (!data || size < sizeof(skode_session_k_t)) return 0;
  skode_session_k_t header;
  memcpy(&header, data, sizeof(header));
  if (header.magic != SKODE_SESSION_K_MAGIC ||
      header.format != SKODE_SESSION_FORMAT || header.count < 0) return 0;
  size_t payload_size = size - sizeof(header);
  if (header.kind == 0)
    return (size_t)header.count <= SIZE_MAX / sizeof(double) &&
      payload_size == (size_t)header.count * sizeof(double);
  return header.kind == 1 && header.count > 0 &&
    payload_size == (size_t)header.count &&
    ((const unsigned char *)data)[size - 1] == '\0';
}

static int skode_session_add_k(mz_zip_archive *zip, const char *name, K value) {
  if (!value) return 1;
  skode_session_k_t header = {
    SKODE_SESSION_K_MAGIC, SKODE_SESSION_FORMAT, 0, 0
  };
  const void *payload;
  size_t payload_size;
  if (k_is_func(value)) {
    header.kind = 1;
    const char *body = k_func_body(value);
    payload_size = body ? strlen(body) + 1 : 1;
    if (payload_size > INT32_MAX) return 0;
    header.count = (int32_t)payload_size;
    payload = body ? (const void *)body : (const void *)"";
  } else {
    if (value->n < 0 ||
        (size_t)value->n > SIZE_MAX / sizeof(double)) return 0;
    header.count = value->n;
    payload_size = (size_t)value->n * sizeof(double);
    payload = value->f;
  }
  skode_session_buffer_t buffer = {0};
  int ok = skode_session_buffer_append(&buffer, &header, sizeof(header)) &&
    skode_session_buffer_append(&buffer, payload, payload_size) &&
    skode_session_zip_add(zip, name, buffer.data, buffer.size);
  free(buffer.data);
  return ok;
}

static K skode_session_read_k(ks_ctx *ks, const void *data, size_t size) {
  if (!ks || !data || size < sizeof(skode_session_k_t)) return NULL;
  skode_session_k_t header;
  memcpy(&header, data, sizeof(header));
  if (header.magic != SKODE_SESSION_K_MAGIC ||
      header.format != SKODE_SESSION_FORMAT || header.count < 0) return NULL;
  const unsigned char *payload =
    (const unsigned char *)data + sizeof(header);
  size_t payload_size = size - sizeof(header);
  if (header.kind == 0) {
    if ((size_t)header.count > SIZE_MAX / sizeof(double) ||
        payload_size != (size_t)header.count * sizeof(double)) return NULL;
    K value = k_new_perm(ks, header.count);
    if (!value) return NULL;
    if (payload_size) memcpy(value->f, payload, payload_size);
    return value;
  }
  if (header.kind == 1 && header.count > 0 &&
      payload_size == (size_t)header.count &&
      payload[payload_size - 1] == '\0') {
    size_t doubles = (payload_size + sizeof(double) - 1) / sizeof(double);
    if (doubles > INT_MAX) return NULL;
    K value = k_new_perm(ks, (int)doubles);
    if (!value) return NULL;
    value->n = -1;
    memcpy(value->f, payload, payload_size);
    return value;
  }
  return NULL;
}
#endif

typedef struct __attribute__((packed)) {
  char riff[4];
  uint32_t riff_sz;
  char wave[4];
  char fmt[4];
  uint32_t fmt_sz;
  uint16_t audio_fmt;
  uint16_t channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  char data_chunk[4];
  uint32_t data_sz;
} skode_wav_header_t;

int skode_session_save(skode_t *ctx, const char *filename) {
  if (!ctx || !ctx->parse || !filename || !filename[0]) return -1;
  mz_zip_archive zip;
  memset(&zip, 0, sizeof(zip));
  if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
    ctx->printf(ctx, "# session ZIP initialization failed\n");
    return -1;
  }
  int ok = 1;

  skode_session_buffer_t state = {0};
  skode_t output = *ctx;
  output.puts = skode_session_puts;
  output.printf = skode_session_printf;
  output.output_user = &state;

  for (int wave = 0; ok && wave < synth_config.wave_table_max; wave++) {
    if (!sw.data[wave] || sw.size[wave] <= 0 || sw.readonly[wave]) continue;
    
    char entry_name[128];
    const char *wave_name = sw.name[wave][0] ? sw.name[wave] : NULL;
    if (wave_name) {
      snprintf(entry_name, sizeof(entry_name), "waves/%s.wav", wave_name);
    } else {
      snprintf(entry_name, sizeof(entry_name), "waves/wave%d.wav", wave);
    }
    
    double stored_rate = sw.rate[wave];
    uint32_t sample_rate = MAIN_SAMPLE_RATE;
    if (isfinite(stored_rate) && stored_rate >= 1.0 && stored_rate <= (double)UINT32_MAX - 0.5) {
      sample_rate = (uint32_t)(stored_rate + 0.5);
    }

    uint32_t data_bytes = sw.size[wave] * sizeof(float);
    uint32_t total_size = sizeof(skode_wav_header_t) + data_bytes;
    char *buf = malloc(total_size);
    if (!buf) {
      ok = 0;
      break;
    }
    
    skode_wav_header_t *h = (skode_wav_header_t *)buf;
    memcpy(h->riff, "RIFF", 4);
    h->riff_sz = 36 + data_bytes;
    memcpy(h->wave, "WAVE", 4);
    memcpy(h->fmt, "fmt ", 4);
    h->fmt_sz = 16;
    h->audio_fmt = 3; // IEEE float
    h->channels = 1;
    h->sample_rate = sample_rate;
    h->byte_rate = sample_rate * 4;
    h->block_align = 4;
    h->bits_per_sample = 32;
    memcpy(h->data_chunk, "data", 4);
    h->data_sz = data_bytes;
    
    memcpy(buf + sizeof(skode_wav_header_t), sw.data[wave], data_bytes);
    
    ok = skode_session_zip_add(&zip, entry_name, buf, total_size);
    free(buf);
    
    if (ok) {
      char load_cmd[256];
      snprintf(load_cmd, sizeof(load_cmd), "[%s] %d /ws\n", entry_name, wave);
      skode_session_buffer_append(&state, load_cmd, strlen(load_cmd));
    }
  }

  if (ok) {
    global_status_show(&output, 1);
  }

  if (ok) {
    ok = !state.failed && skode_session_zip_add(&zip, "main.sk", state.data, state.size);
  }
  free(state.data);

  void *archive = NULL;
  size_t archive_size = 0;
  if (ok) ok = mz_zip_writer_finalize_heap_archive(&zip, &archive, &archive_size);
  mz_zip_writer_end(&zip);
  if (ok) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
      ok = 0;
    } else {
      if (fwrite(archive, 1, archive_size, file) != archive_size) ok = 0;
      if (fclose(file) != 0) ok = 0;
    }
  }
  mz_free(archive);
  if (!ok) {
    ctx->printf(ctx, "# session save failed\n");
    return -1;
  }
  ctx->printf(ctx, "# session saved to %s\n", filename);
  return 0;
}

int skode_session_load(skode_t *ctx, const char *filename) {
  char saved_filename[1024];
  if (filename) snprintf(saved_filename, sizeof(saved_filename), "%s", filename);

  if (!ctx || !ctx->parse || !filename || !filename[0]) return -1;
  int live_capture_state = atomic_load_int(&sampling.state);
  if (live_capture_state == SAMPLE_STATE_ARMED || live_capture_state == SAMPLE_STATE_RECORDING) {
    ctx->printf(ctx, "# cannot restore a session while <r is active\n");
    return -1;
  }
  int recorder_state = skred_record_state();
  if (recorder_state == 1 || recorder_state == 2) {
    ctx->printf(ctx, "# cannot restore a session while /rg is active\n");
    return -1;
  }
  
  if (skred_vfs_mount(saved_filename)) {
    ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
    void *data = NULL;
    size_t size = 0;
    char resolved[1024];
    if (skode_asset_read("main.sk", SKODE_ASSET_SKODE, &data, &size, resolved, sizeof(resolved))) {
      ctx->printf(ctx, "# auto-loading %s\n", resolved[0] ? resolved : "main.sk");
      
      const char *text = (const char *)data;
      size_t text_len = size;
      size_t pos = 0;
      int r = 0;
      while (pos < text_len) {
        size_t start = pos;
        int in_string = 0;
        int in_comment = 0;
        
        while (pos < text_len) {
          char c = text[pos];
          if (in_comment) {
            if (c == '\n' || c == '\r') break;
          } else if (in_string) {
            if (c == ']') in_string = 0;
          } else {
            if (c == '[') in_string = 1;
            else if (c == '#') in_comment = 1;
            else if (c == '\n' || c == '\r') break;
          }
          pos++;
        }
        size_t len = pos - start;
        char *chunk = (char *)malloc(len + 1);
        if (chunk) {
          memcpy(chunk, text + start, len);
          chunk[len] = '\0';
          r = skode_consume(chunk, ctx);
          free(chunk);
          if (r != 0) break;
        }
        
        while (pos < text_len && (text[pos] == '\n' || text[pos] == '\r')) {
            pos++;
        }
      }
      
      skred_vfs_free_file(data);
      if (r == 0) ctx->printf(ctx, "# session restored [%s]\n", saved_filename);
      else ctx->printf(ctx, "# session restore failed\n");
      return r;
    } else {
      ctx->printf(ctx, "# session missing main.sk\n");
      return -1;
    }
  } else {
    ctx->printf(ctx, "# cannot mount session [%s]\n", saved_filename);
    return -1;
  }
  return 0;
}
