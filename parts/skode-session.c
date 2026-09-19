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

int skode_session_save(skode_t *ctx, const char *filename) {
  if (!ctx || !ctx->parse || !filename || !filename[0]) return -1;
  mz_zip_archive zip;
  memset(&zip, 0, sizeof(zip));
  if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
    ctx->printf(ctx, "# session ZIP initialization failed\n");
    return -1;
  }
  int ok = 1;
  char manifest[1024];
  int manifest_len = snprintf(manifest, sizeof(manifest),
    "skred_session=%u\nskred_version=%s\nfeatures=%s\n"
    "sample_rate=%d\nvoices=%d\n"
    "waves=%d\nvoice=%d\npattern=%d\nstep=%d\nflag=%d\ntrace=%d\n"
    "verbose=%d\n",
    SKODE_SESSION_FORMAT, skred_version(), skred_features(),
    synth_sample_rate_get(),
    synth_config.voice_max, synth_config.wave_table_max,
    ctx->voice, ctx->pattern, ctx->step, ctx->flag, ctx->trace,
    ctx->verbose);
  ok = manifest_len > 0 && (size_t)manifest_len < sizeof(manifest) &&
    skode_session_zip_add(&zip, "manifest.txt", manifest,
      (size_t)manifest_len);

  skode_session_buffer_t state = {0};
  skode_t output = *ctx;
  output.puts = skode_session_puts;
  output.printf = skode_session_printf;
  output.output_user = &state;
  if (ok) {
    global_status_show(&output, 1);
    ok = !state.failed &&
      skode_session_zip_add(&zip, "state.sk", state.data, state.size);
  }
  free(state.data);

  if (ok) ok = skode_session_zip_add(&zip, "repl/variables.f64",
    global_var, sizeof(global_var));
  int data_len = ands_data_len(ctx->parse);
  if (ok && data_len > 0) {
    ok = skode_session_zip_add(&zip, "repl/data.f64",
      ands_data(ctx->parse), (size_t)data_len * sizeof(double));
  }
  for (int i = 0; ok && i < STRING_BUF_IDX_MAX; i++) {
    char entry[64];
    char command[STRING_BUF_LEN];
    if (skode_extra_copy(i, command, sizeof(command)) != 0) {
      ok = 0;
      break;
    }
    if (!command[0]) continue;
    snprintf(entry, sizeof(entry), "external/%03d.sk", i);
    ok = skode_session_zip_add(&zip, entry, command, strlen(command));
  }
  for (int i = 0; ok && i < SKODE_STRING_SLOT_MAX; i++) {
    if (!ctx->string_slot[i][0]) continue;
    char entry[64];
    snprintf(entry, sizeof(entry), "repl/strings/%02d.txt", i);
    ok = skode_session_zip_add(&zip, entry, ctx->string_slot[i],
      strlen(ctx->string_slot[i]));
  }

  #ifdef KSYNTH
  if (ok && ctx->ks) {
    simple_mutex_lock(&skode_ks_eval_mutex);
    for (int i = 0; ok && i < 26; i++) {
      char vname[2] = {(char)('A' + i), 0};
      K var = k_get_var_str(ctx->ks, vname);
      if (!var) continue;
      char entry[64];
      snprintf(entry, sizeof(entry), "ksynth/%c.ksv", 'A' + i);
      ok = skode_session_add_k(&zip, entry, var);
    }
    if (ok && ctx->ks_result)
      ok = skode_session_add_k(&zip, "ksynth/result.ksv",
        (K)ctx->ks_result);
    simple_mutex_unlock(&skode_ks_eval_mutex);
  }
  #endif

  for (int wave = 0; ok && wave < synth_config.wave_table_max; wave++) {
    if (!sw.data[wave] || sw.size[wave] <= 0 || sw.readonly[wave]) continue;
    skode_session_wave_t meta = {
      SKODE_SESSION_WAVE_MAGIC, SKODE_SESSION_FORMAT, wave, sw.size[wave],
      sw.rate[wave], sw.one_shot[wave], sw.loop_enabled[wave],
      sw.loop_start[wave], sw.loop_end[wave], sw.direction[wave],
      sw.midi_note[wave], sw.offset_hz[wave], {0}
    };
    snprintf(meta.name, sizeof(meta.name), "%s", sw.name[wave]);
    char metadata_entry[64], data_entry[64];
    snprintf(metadata_entry, sizeof(metadata_entry), "waves/%d.meta", wave);
    snprintf(data_entry, sizeof(data_entry), "waves/%d.f32", wave);
    ok = skode_session_zip_add(&zip, metadata_entry, &meta, sizeof(meta)) &&
      skode_session_zip_add(&zip, data_entry, sw.data[wave],
        (size_t)sw.size[wave] * sizeof(float));
  }

  if (ok && atomic_load_int(&sampling.state) == SAMPLE_STATE_COMPLETE &&
      sampling.where && sampling.len > 0) {
    int channels = sampling.channels == 2 ? 2 : 1;
    skode_session_record_t meta = {
      SKODE_SESSION_RECORD_MAGIC, SKODE_SESSION_FORMAT, sampling.len,
      channels, sampling.offset, sampling.trim, sampling.source,
      sampling.source_voice
    };
    ok = skode_session_zip_add(&zip, "recording/meta.bin",
      &meta, sizeof(meta)) &&
      skode_session_zip_add(&zip, "recording/audio.f32", sampling.where,
        (size_t)sampling.len * (size_t)channels * sizeof(float));
  }

  skred_control_response_snapshot_t responses[64];
  int response_count = skred_control_response_snapshot(responses, 64);
  int response_enabled = skred_control_response_enabled();
  if (ok && response_count >= 0) {
    ok = skode_session_zip_add(&zip, "control/responses.bin", responses,
      (size_t)response_count * sizeof(responses[0])) &&
      skode_session_zip_add(&zip, "control/enabled.i32",
        &response_enabled, sizeof(response_enabled));
  } else if (response_count < 0) {
    ok = 0;
  }

  skred_midi_route_snapshot_t routes[SKRED_MIDI_ROUTE_MAX];
  skred_midi_binding_snapshot_t bindings[SKRED_MIDI_BINDING_MAX];
  int route_count = skred_midi_route_snapshot(routes, SKRED_MIDI_ROUTE_MAX);
  int binding_count =
    skred_midi_binding_snapshot(bindings, SKRED_MIDI_BINDING_MAX);
  if (ok && route_count >= 0 && binding_count >= 0) {
    skode_session_midi_settings_t settings = {
      skred_midi_event_mask(), skred_midi_debug_get()
    };
    ok = skode_session_zip_add(&zip, "midi/routes.bin", routes,
      (size_t)route_count * sizeof(routes[0])) &&
      skode_session_zip_add(&zip, "midi/bindings.bin", bindings,
        (size_t)binding_count * sizeof(bindings[0])) &&
      skode_session_zip_add(&zip, "midi/settings.bin", &settings,
        sizeof(settings));
  } else if (route_count < 0 || binding_count < 0) {
    ok = 0;
  }

  void *archive = NULL;
  size_t archive_size = 0;
  if (ok) ok = mz_zip_writer_finalize_heap_archive(&zip,
    &archive, &archive_size);
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
    ctx->printf(ctx, "# session save failed [%s]\n", filename);
    return -1;
  }
  ctx->printf(ctx, "# session saved [%s] %zu bytes\n",
    filename, archive_size);
  return 0;
}

static int skode_session_parse_manifest(const char *manifest, size_t size,
    int *voice, int *pattern, int *step, int *sample_rate,
    int *voices, int *waves, int *flag, int *trace, int *verbose,
    char *features, size_t features_size) {
  if (!manifest || size == 0 || size > 64 * 1024) return 0;
  char *copy = (char *)malloc(size + 1);
  if (!copy) return 0;
  memcpy(copy, manifest, size);
  copy[size] = '\0';
  unsigned format = 0;
  char *line = strtok(copy, "\n");
  while (line) {
    (void)sscanf(line, "skred_session=%u", &format);
    (void)sscanf(line, "voice=%d", voice);
    (void)sscanf(line, "pattern=%d", pattern);
    (void)sscanf(line, "step=%d", step);
    (void)sscanf(line, "sample_rate=%d", sample_rate);
    (void)sscanf(line, "voices=%d", voices);
    (void)sscanf(line, "waves=%d", waves);
    (void)sscanf(line, "flag=%d", flag);
    (void)sscanf(line, "trace=%d", trace);
    (void)sscanf(line, "verbose=%d", verbose);
    if (strncmp(line, "features=", 9) == 0 && features &&
        features_size > 0)
      snprintf(features, features_size, "%s", line + 9);
    line = strtok(NULL, "\n");
  }
  free(copy);
  return format == SKODE_SESSION_FORMAT;
}

static int skode_session_restore_k(skode_t *ctx, mz_zip_archive *zip) {
  #ifdef KSYNTH
  int any = skode_session_zip_has(zip, "ksynth/result.ksv");
  char entry[64];
  for (int i = 0; i < 26 && !any; i++) {
    snprintf(entry, sizeof(entry), "ksynth/%c.ksv", 'A' + i);
    any = skode_session_zip_has(zip, entry);
  }
  if (!any) {
    if (ctx->ks) ks_clear_vars(ctx->ks);
    skode_ks_result_clear(ctx);
    return 1;
  }
  ks_ctx *ks = skode_ks_ctx(ctx);
  if (!ks) return 0;
  simple_mutex_lock(&skode_ks_eval_mutex);
  skode_ks_result_clear(ctx);
  int ok = 1;
  for (int i = 0; ok && i < 26; i++) {
    snprintf(entry, sizeof(entry), "ksynth/%c.ksv", 'A' + i);
    if (!skode_session_zip_has(zip, entry)) continue;
    size_t size = 0;
    void *data = skode_session_zip_read(zip, entry, &size);
    K value = skode_session_read_k(ks, data, size);
    mz_free(data);
    if (!value) ok = 0;
    else {
      char vname[2] = {(char)('A' + i), 0};
      k_set_var_str(ks, vname, value);
    }
  }
  if (ok && skode_session_zip_has(zip, "ksynth/result.ksv")) {
    size_t size = 0;
    void *data =
      skode_session_zip_read(zip, "ksynth/result.ksv", &size);
    K value = skode_session_read_k(ks, data, size);
    mz_free(data);
    if (!value) ok = 0;
    else ctx->ks_result = value;
  }
  simple_mutex_unlock(&skode_ks_eval_mutex);
  return ok;
  #else
  (void)ctx;
  return !skode_session_zip_has(zip, "ksynth/result.ksv") &&
    !skode_session_zip_has(zip, "ksynth/A.ksv");
  #endif
}

int skode_session_load(skode_t *ctx, const char *filename) {
  if (!ctx || !ctx->parse || !filename || !filename[0]) return -1;
  int live_capture_state = atomic_load_int(&sampling.state);
  if (live_capture_state == SAMPLE_STATE_ARMED ||
      live_capture_state == SAMPLE_STATE_RECORDING) {
    ctx->printf(ctx, "# cannot restore a session while <r is active\n");
    return -1;
  }
  int recorder_state = skred_record_state();
  if (recorder_state == 1 || recorder_state == 2) {
    ctx->printf(ctx, "# cannot restore a session while /rg is active\n");
    return -1;
  }
  void *archive = NULL;
  size_t archive_size = 0;
  if (!skode_asset_read(filename, SKODE_ASSET_ANY, &archive, &archive_size,
      NULL, 0)) {
    ctx->printf(ctx, "# cannot read session [%s]\n", filename);
    return -1;
  }
  mz_zip_archive zip;
  memset(&zip, 0, sizeof(zip));
  if (!mz_zip_reader_init_mem(&zip, archive, archive_size, 0)) {
    skred_vfs_free_file(archive);
    ctx->printf(ctx, "# invalid session ZIP [%s]\n", filename);
    return -1;
  }
  int ok = 1;
  int restore_control_enabled = 0;
  size_t manifest_size = 0;
  char *manifest =
    (char *)skode_session_zip_read(&zip, "manifest.txt", &manifest_size);
  int selected_voice = 0, selected_pattern = 0, selected_step = -1;
  int saved_sample_rate = 0, saved_voices = 0, saved_waves = 0;
  int saved_flag = 0, saved_trace = 0, saved_verbose = 0;
  char saved_features[512] = {0};
  ok = skode_session_parse_manifest(manifest, manifest_size,
    &selected_voice, &selected_pattern, &selected_step,
    &saved_sample_rate, &saved_voices, &saved_waves,
    &saved_flag, &saved_trace, &saved_verbose,
    saved_features, sizeof(saved_features)) &&
    strcmp(saved_features, skred_features()) == 0 &&
    saved_sample_rate == synth_sample_rate_get() &&
    saved_voices > 0 && saved_voices <= synth_config.voice_max &&
    saved_waves > 0 && saved_waves <= synth_config.wave_table_max &&
    skode_session_zip_has(&zip, "state.sk") &&
    skode_session_zip_has(&zip, "repl/variables.f64") &&
    skode_session_zip_has(&zip, "control/responses.bin") &&
    skode_session_zip_has(&zip, "control/enabled.i32") &&
    skode_session_zip_has(&zip, "midi/routes.bin") &&
    skode_session_zip_has(&zip, "midi/bindings.bin") &&
    skode_session_zip_has(&zip, "midi/settings.bin");
  mz_free(manifest);

  /* Validate all wave and recording payload sizes before changing live state. */
  for (int wave = 0; ok && wave < synth_config.wave_table_max; wave++) {
    char metadata_entry[64], data_entry[64];
    snprintf(metadata_entry, sizeof(metadata_entry), "waves/%d.meta", wave);
    if (!skode_session_zip_has(&zip, metadata_entry)) continue;
    size_t meta_size = 0, data_size = 0;
    skode_session_wave_t *meta = (skode_session_wave_t *)
      skode_session_zip_read(&zip, metadata_entry, &meta_size);
    snprintf(data_entry, sizeof(data_entry), "waves/%d.f32", wave);
    void *data = skode_session_zip_read(&zip, data_entry, &data_size);
    ok = meta && meta_size == sizeof(*meta) &&
      meta->magic == SKODE_SESSION_WAVE_MAGIC &&
      meta->format == SKODE_SESSION_FORMAT && meta->slot == wave &&
      meta->length > 0 &&
      (size_t)meta->length <= SIZE_MAX / sizeof(float) &&
      data && data_size == (size_t)meta->length * sizeof(float) &&
      !sw.readonly[wave];
    mz_free(meta);
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "recording/meta.bin")) {
    size_t meta_size = 0, data_size = 0;
    skode_session_record_t *meta = (skode_session_record_t *)
      skode_session_zip_read(&zip, "recording/meta.bin", &meta_size);
    void *data =
      skode_session_zip_read(&zip, "recording/audio.f32", &data_size);
    ok = meta && meta_size == sizeof(*meta) &&
      meta->magic == SKODE_SESSION_RECORD_MAGIC &&
      meta->format == SKODE_SESSION_FORMAT && meta->frames > 0 &&
      (meta->channels == 1 || meta->channels == 2) &&
      meta->offset >= 0 && meta->trim >= 0 &&
      meta->offset <= meta->frames &&
      meta->trim <= meta->frames - meta->offset &&
      data && data_size ==
        (size_t)meta->frames * (size_t)meta->channels * sizeof(float);
    mz_free(meta);
    mz_free(data);
  }
  if (ok) {
    size_t size = 0;
    void *data =
      skode_session_zip_read(&zip, "repl/variables.f64", &size);
    ok = data && size == sizeof(global_var);
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "repl/data.f64")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "repl/data.f64", &size);
    ok = data && size % sizeof(double) == 0 &&
      size / sizeof(double) <= INT_MAX;
    mz_free(data);
  }
  for (int i = 0; ok && i < STRING_BUF_IDX_MAX; i++) {
    char entry[64];
    snprintf(entry, sizeof(entry), "external/%03d.sk", i);
    if (!skode_session_zip_has(&zip, entry)) continue;
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, entry, &size);
    ok = data && size < STRING_BUF_LEN;
    mz_free(data);
  }
  for (int i = 0; ok && i < SKODE_STRING_SLOT_MAX; i++) {
    char entry[64];
    snprintf(entry, sizeof(entry), "repl/strings/%02d.txt", i);
    if (!skode_session_zip_has(&zip, entry)) continue;
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, entry, &size);
    ok = data && size < SKODE_STRING_SLOT_LEN;
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "control/responses.bin")) {
    size_t size = 0, enabled_size = 0;
    void *data =
      skode_session_zip_read(&zip, "control/responses.bin", &size);
    void *enabled =
      skode_session_zip_read(&zip, "control/enabled.i32", &enabled_size);
    ok = size % sizeof(skred_control_response_snapshot_t) == 0 &&
      size / sizeof(skred_control_response_snapshot_t) <= 64 &&
      enabled && enabled_size == sizeof(int);
    mz_free(data);
    mz_free(enabled);
  }
  if (ok && skode_session_zip_has(&zip, "midi/routes.bin")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "midi/routes.bin", &size);
    ok = size % sizeof(skred_midi_route_snapshot_t) == 0 &&
      size / sizeof(skred_midi_route_snapshot_t) <= SKRED_MIDI_ROUTE_MAX;
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "midi/bindings.bin")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "midi/bindings.bin", &size);
    ok = size % sizeof(skred_midi_binding_snapshot_t) == 0 &&
      size / sizeof(skred_midi_binding_snapshot_t) <=
        SKRED_MIDI_BINDING_MAX;
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "midi/settings.bin")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "midi/settings.bin", &size);
    ok = data && size == sizeof(skode_session_midi_settings_t);
    mz_free(data);
  }
  #ifdef KSYNTH
  for (int i = 0; ok && i < 26; i++) {
    char entry[64];
    snprintf(entry, sizeof(entry), "ksynth/%c.ksv", 'A' + i);
    if (!skode_session_zip_has(&zip, entry)) continue;
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, entry, &size);
    ok = skode_session_k_valid(data, size);
    mz_free(data);
  }
  if (ok && skode_session_zip_has(&zip, "ksynth/result.ksv")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "ksynth/result.ksv", &size);
    ok = skode_session_k_valid(data, size);
    mz_free(data);
  }
  #else
  if (ok) {
    for (int i = 0; i < 26; i++) {
      char entry[64];
      snprintf(entry, sizeof(entry), "ksynth/%c.ksv", 'A' + i);
      if (skode_session_zip_has(&zip, entry)) ok = 0;
    }
    if (skode_session_zip_has(&zip, "ksynth/result.ksv")) ok = 0;
  }
  #endif
  if (!ok) {
    mz_zip_reader_end(&zip);
    skred_vfs_free_file(archive);
    ctx->printf(ctx, "# incompatible or corrupt session [%s]\n", filename);
    return -1;
  }

  int audio_was_running = skred_audio_running();
  if (audio_was_running && skred_audio_disconnect() != 0) {
    mz_zip_reader_end(&zip);
    skred_vfs_free_file(archive);
    ctx->printf(ctx, "# cannot pause audio for session restore\n");
    return -1;
  }
  skred_control_response_set_enabled(0);
  seq_state_all(SEQ_STOPPED);
  seq_kill_all();
  for (int pattern = 0; pattern < PATTERNS_MAX; pattern++)
    pattern_reset(pattern);
  voice_init();
  skred_poly_reset();
  ands_macro_clear(ctx->parse);
  memset(global_var, 0, sizeof(global_var));
  simple_mutex_lock(&skode_extra_mutex);
  memset(_skode_extra, 0, sizeof(_skode_extra));
  simple_mutex_unlock(&skode_extra_mutex);
  memset(ctx->string_slot, 0, sizeof(ctx->string_slot));

  size_t variables_size = 0;
  void *variables =
    skode_session_zip_read(&zip, "repl/variables.f64", &variables_size);
  ok = variables && variables_size == sizeof(global_var);
  if (ok) memcpy(global_var, variables, sizeof(global_var));
  mz_free(variables);
  if (ok && skode_session_zip_has(&zip, "repl/data.f64")) {
    size_t size = 0;
    void *data = skode_session_zip_read(&zip, "repl/data.f64", &size);
    ok = data && size % sizeof(double) == 0 &&
      size / sizeof(double) <= INT_MAX;
    int count = ok ? (int)(size / sizeof(double)) : 0;
    if (ok && count > ands_data_cap(ctx->parse))
      ands_data_resize(ctx->parse, count);
    if (ok && count > ands_data_cap(ctx->parse)) ok = 0;
    if (ok) {
      if (count) memcpy(ands_data(ctx->parse), data, size);
      ands_data_len_set(ctx->parse, count);
    }
    mz_free(data);
  } else {
    ands_data_len_set(ctx->parse, 0);
  }
  for (int i = 0; ok && i < STRING_BUF_IDX_MAX; i++) {
    char entry[64];
    snprintf(entry, sizeof(entry), "external/%03d.sk", i);
    if (!skode_session_zip_has(&zip, entry)) continue;
    size_t size = 0;
    char *data = (char *)skode_session_zip_read(&zip, entry, &size);
    ok = data && size < STRING_BUF_LEN;
    if (ok) {
      simple_mutex_lock(&skode_extra_mutex);
      memcpy(_skode_extra[i], data, size);
      _skode_extra[i][size] = '\0';
      simple_mutex_unlock(&skode_extra_mutex);
    }
    mz_free(data);
  }
  for (int i = 0; ok && i < SKODE_STRING_SLOT_MAX; i++) {
    char entry[64];
    snprintf(entry, sizeof(entry), "repl/strings/%02d.txt", i);
    if (!skode_session_zip_has(&zip, entry)) continue;
    size_t size = 0;
    char *data = (char *)skode_session_zip_read(&zip, entry, &size);
    ok = data && size < SKODE_STRING_SLOT_LEN;
    if (ok) {
      memcpy(ctx->string_slot[i], data, size);
      ctx->string_slot[i][size] = '\0';
    }
    mz_free(data);
  }
  if (ok) ok = skode_session_restore_k(ctx, &zip);

  for (int wave = 0; ok && wave < synth_config.wave_table_max; wave++) {
    if (!sw.readonly[wave]) wave_free_one(wave);
    char metadata_entry[64], data_entry[64];
    snprintf(metadata_entry, sizeof(metadata_entry), "waves/%d.meta", wave);
    if (!skode_session_zip_has(&zip, metadata_entry)) continue;
    size_t meta_size = 0, data_size = 0;
    skode_session_wave_t *meta = (skode_session_wave_t *)
      skode_session_zip_read(&zip, metadata_entry, &meta_size);
    snprintf(data_entry, sizeof(data_entry), "waves/%d.f32", wave);
    float *data = (float *)
      skode_session_zip_read(&zip, data_entry, &data_size);
    float *samples = (float *)malloc(data_size);
    if (!meta || !data || !samples) ok = 0;
    if (ok) {
      memcpy(samples, data, data_size);
      wave_install_memory(wave, samples, meta->length, meta->rate,
        meta->one_shot, meta->name, meta->midi_note, meta->offset_hz);
      sw.loop_enabled[wave] = meta->loop_enabled;
      sw.loop_start[wave] = meta->loop_start;
      sw.loop_end[wave] = meta->loop_end;
      sw.direction[wave] = meta->direction;
    } else {
      free(samples);
    }
    mz_free(meta);
    mz_free(data);
  }

  if (ok && skode_session_zip_has(&zip, "recording/meta.bin")) {
    size_t meta_size = 0, data_size = 0;
    skode_session_record_t *meta = (skode_session_record_t *)
      skode_session_zip_read(&zip, "recording/meta.bin", &meta_size);
    float *data = (float *)
      skode_session_zip_read(&zip, "recording/audio.f32", &data_size);
    ok = meta && data && skode_sample_alloc(meta->frames);
    if (ok) {
      memcpy(sampling.where, data, data_size);
      sampling.len = meta->frames;
      sampling.ptr = meta->frames;
      sampling.channels = meta->channels;
      sampling.offset = meta->offset;
      sampling.trim = meta->trim;
      sampling.source = meta->source;
      sampling.source_voice = meta->source_voice;
      atomic_store_int(&sampling.frames, 0);
      atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
    }
    mz_free(meta);
    mz_free(data);
  } else if (ok) {
    atomic_store_int(&sampling.state, SAMPLE_STATE_IDLE);
    sampling.len = sampling.ptr = sampling.offset = sampling.trim = 0;
  }

  if (ok) {
    size_t state_size = 0;
    char *state = (char *)skode_session_zip_read(&zip, "state.sk",
      &state_size);
    ok = state &&
      skode_load_buffer(ctx, state, state_size, "session:state.sk", 0) == 0;
    mz_free(state);
  }

  if (ok && skode_session_zip_has(&zip, "control/responses.bin")) {
    size_t size = 0, enabled_size = 0;
    skred_control_response_snapshot_t *responses =
      (skred_control_response_snapshot_t *)skode_session_zip_read(&zip,
        "control/responses.bin", &size);
    int *enabled = (int *)skode_session_zip_read(&zip,
      "control/enabled.i32", &enabled_size);
    ok = size % sizeof(*responses) == 0 &&
      size / sizeof(*responses) <= 64 && enabled &&
      enabled_size == sizeof(*enabled) &&
      skred_control_response_restore(responses,
        (int)(size / sizeof(*responses)), 0) == 0;
    if (ok) restore_control_enabled = *enabled != 0;
    mz_free(responses);
    mz_free(enabled);
  }
  if (ok && skode_session_zip_has(&zip, "midi/routes.bin")) {
    size_t size = 0;
    skred_midi_route_snapshot_t *routes =
      (skred_midi_route_snapshot_t *)skode_session_zip_read(&zip,
        "midi/routes.bin", &size);
    ok = size % sizeof(*routes) == 0 &&
      size / sizeof(*routes) <= SKRED_MIDI_ROUTE_MAX &&
      skred_midi_route_restore(routes, (int)(size / sizeof(*routes))) == 0;
    mz_free(routes);
  }
  if (ok && skode_session_zip_has(&zip, "midi/bindings.bin")) {
    size_t size = 0;
    skred_midi_binding_snapshot_t *bindings =
      (skred_midi_binding_snapshot_t *)skode_session_zip_read(&zip,
        "midi/bindings.bin", &size);
    ok = size % sizeof(*bindings) == 0 &&
      size / sizeof(*bindings) <= SKRED_MIDI_BINDING_MAX &&
      skred_midi_binding_restore(bindings,
        (int)(size / sizeof(*bindings))) == 0;
    mz_free(bindings);
  }
  if (ok && skode_session_zip_has(&zip, "midi/settings.bin")) {
    size_t size = 0;
    skode_session_midi_settings_t *settings =
      (skode_session_midi_settings_t *)skode_session_zip_read(&zip,
        "midi/settings.bin", &size);
    if (!settings || size != sizeof(*settings)) {
      ok = 0;
    } else {
      skred_midi_event_mask_set(settings->event_mask);
      skred_midi_debug_set(settings->debug);
    }
    mz_free(settings);
  }

  if (ok) {
    ctx->voice = selected_voice >= 0 &&
      selected_voice < synth_config.voice_max ? selected_voice : 0;
    ctx->pattern = selected_pattern >= 0 &&
      selected_pattern < PATTERNS_MAX ? selected_pattern : 0;
    ctx->step = selected_step >= -1 && selected_step < SEQ_STEPS_MAX ?
      selected_step : -1;
    ctx->flag = saved_flag;
    ctx->trace = saved_trace;
    ctx->verbose = saved_verbose;
    ands_trace_set(ctx->parse, saved_trace > 1);
    skred_control_response_set_enabled(restore_control_enabled);
  }
  mz_zip_reader_end(&zip);
  skred_vfs_free_file(archive);
  int audio_reconnect_failed =
    audio_was_running && skred_audio_reconnect() != 0;
  if (!ok) {
    skred_control_response_set_enabled(0);
    ctx->printf(ctx, "# session restore failed [%s]\n", filename);
    if (audio_reconnect_failed)
      ctx->printf(ctx, "# audio reconnect also failed\n");
    return -1;
  }
  if (audio_reconnect_failed) {
    ctx->printf(ctx, "# session restored but audio reconnect failed\n");
    return -1;
  }
  ctx->printf(ctx, "# session restored [%s]\n", filename);
  return 0;
}
