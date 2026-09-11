#include "skode-internal.h"


#include "miniaudio.h"

float *mw_free(float *f) {
    if (f) free(f);
    return NULL;
}

static uint32_t mw_le_u32(const unsigned char *p) {
  return ((uint32_t)p[0]) |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

int mw_get_smpl_loop_mem(const void *data, size_t data_size, int frames,
                         mw_smpl_loop_t *loop) {
  const unsigned char *bytes = (const unsigned char *)data;
  size_t pos = 12;

  if (!loop) return 0;
  memset(loop, 0, sizeof(*loop));
  if (!bytes || data_size < 12 || frames <= 0) return 0;
  if (memcmp(bytes, "RIFF", 4) != 0 || memcmp(bytes + 8, "WAVE", 4) != 0)
    return 0;

  while (pos + 8 <= data_size) {
    const unsigned char *chunk = bytes + pos;
    uint32_t chunk_size = mw_le_u32(chunk + 4);
    size_t data_start = pos + 8;
    size_t next_chunk = data_start + (size_t)chunk_size + (size_t)(chunk_size & 1u);

    if (next_chunk < data_start || data_start > data_size) break;

    if (memcmp(chunk, "smpl", 4) == 0) {
      const unsigned char *header;
      const unsigned char *sample_loop;
      uint32_t sample_loop_count;
      uint32_t type;
      uint32_t start;
      uint32_t end;
      uint32_t play_count;
      int end_exclusive;

      if (chunk_size < 60 || data_start + 60 > data_size) break;
      header = bytes + data_start;
      sample_loop_count = mw_le_u32(header + 28);
      if (sample_loop_count == 0) break;
      sample_loop = header + 36;

      type = mw_le_u32(sample_loop + 4);
      start = mw_le_u32(sample_loop + 8);
      end = mw_le_u32(sample_loop + 12);
      play_count = mw_le_u32(sample_loop + 20);

      if (start >= (uint32_t)frames || end < start) break;
      end_exclusive = end >= (uint32_t)frames ? frames : (int)end + 1;
      if (end_exclusive <= (int)start) break;

      loop->found = 1;
      loop->start = (int)start;
      loop->end = end_exclusive;
      loop->type = (int)type;
      loop->play_count = (int)play_count;
      return 1;
    }

    if (next_chunk <= pos) break;
    pos = next_chunk;
  }

  return 0;
}

static float *mw_decode(ma_decoder *decoder, const char *label,
                        int *frames_out, wav_t *w, int ch,
                        char *out, int len, float *pSamples,
                        ma_uint64 frameCount) {
  ma_uint64 j = 0;
  if (out != NULL && len > 0) {
    snprintf(out, len, "Loaded %llu frames / %d channels / %d sample rate\n",
      frameCount,
      decoder->outputChannels,
      decoder->outputSampleRate);
  }
  if (ch >= (int)decoder->outputChannels) ch = (int)decoder->outputChannels - 1;
  for (ma_uint64 i = 0; i < frameCount * decoder->outputChannels; i += decoder->outputChannels) {
    if (ch == -1) {
      float a = 0;
      for (ma_uint32 k = 0; k < decoder->outputChannels; k++) a += pSamples[i + k];
      pSamples[j] = a / (float)decoder->outputChannels;
    } else {
      pSamples[j] = pSamples[i + ch];
    }
    j++;
  }
  (void)label;
  w->SamplesRate = decoder->outputSampleRate;
  w->Channels = decoder->outputChannels;
  *frames_out = (int)frameCount;
  return pSamples;
}

float *mw_get_str(char *filename, int *frames_out, wav_t *w, int ch, char *out, int len) {
  ma_result result;
  ma_decoder decoder;
  ma_decoder_config decoderConfig;

  // We want interleaved float32 output
  decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);

  result = ma_decoder_init_file(filename, &decoderConfig, &decoder);
  if (result != MA_SUCCESS) {
    if (out != NULL && len > 0) {
      snprintf(out, len, "Could not load file: %s\n", filename);
    }
    *frames_out = 0;
    return NULL;
  }
  float* pSamples = NULL;
  ma_uint64 frameCount = 0;
  result = ma_decode_file(filename, &decoderConfig, &frameCount, (void**)&pSamples);
  if (result == MA_SUCCESS) {
    // pSamples is now your interleaved float32 array
    // frameCount * channels = total number of floats
    if (out != NULL && len > 0) {
      snprintf(out, len, "Loaded %llu frames / %d channels / %d sample rate\n",
        frameCount,
        decoder.outputChannels,
        decoder.outputSampleRate);
    }
  } else {
    *frames_out = 0;
    return NULL;
  }
  pSamples = mw_decode(&decoder, filename, frames_out, w, ch, out, len,
                       pSamples, frameCount);
  ma_decoder_uninit(&decoder);
  return pSamples;
}

float *mw_get_mem(const void *data, size_t data_size, const char *label,
                  int *frames_out, wav_t *w, int ch, char *out, int len) {
  ma_result result;
  ma_decoder decoder;
  ma_decoder_config decoderConfig;
  float* pSamples = NULL;
  ma_uint64 frameCount = 0;

  decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
  result = ma_decoder_init_memory(data, data_size, &decoderConfig, &decoder);
  if (result != MA_SUCCESS) {
    if (out != NULL && len > 0) {
      snprintf(out, len, "Could not load memory file: %s\n",
               label ? label : "(memory)");
    }
    *frames_out = 0;
    return NULL;
  }

  result = ma_decode_memory(data, data_size, &decoderConfig, &frameCount,
                            (void**)&pSamples);
  if (result != MA_SUCCESS) {
    ma_decoder_uninit(&decoder);
    *frames_out = 0;
    return NULL;
  }

  pSamples = mw_decode(&decoder, label, frames_out, w, ch, out, len,
                       pSamples, frameCount);
  ma_decoder_uninit(&decoder);
  return pSamples;
}

float *mw_get(char *filename, int *frames_out, wav_t *w, int ch) {
  return mw_get_str(filename, frames_out, w, ch, NULL, 0);
}


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

int skode_asset_read(const char *path, skode_asset_kind_t kind,
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

  if (has_dir) {
    char parent_candidate[1024];
    snprintf(parent_candidate, sizeof(parent_candidate), "../%s", path);
    if (skode_asset_try_read(parent_candidate, 1, data, size, resolved, resolved_size))
      return 1;
    return 0;
  }

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
  char vfs_candidate[2048];
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

int skode_load_buffer(skode_t *ctx, const char *text, size_t text_len,
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

void wave_install_memory(int wave_slot, float *table, int len,
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
    wave_install_memory(wave_slot, table, len, (float)MAIN_SAMPLE_RATE,
      one_shot, wave_name, one_shot ? 69.0f : 0.0f, 0.0f);
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
