#include "skode-internal.h"
#include "miniaudio.h"
#include <math.h>



float record_frame_level(int frame) {
  int channels = sampling.channels == 2 ? 2 : 1;
  size_t index = (size_t)frame * (size_t)channels;
  float level = fabsf(sampling.where[index]);
  if (channels == 2) {
    float right = fabsf(sampling.where[index + 1]);
    if (right > level) level = right;
  }
  return level;
}


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

int skode_write_wav(skode_t *ctx, const char *filename,
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

