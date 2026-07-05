#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "miniwav.h"

int mw_put(char *name, int16_t *capture, int frames) {
    wav_t wave = {
        .RIFFChunkID = {'R', 'I', 'F', 'F'},
        .RIFFChunkSize = frames + 36,
        .Format = {'W', 'A', 'V', 'E'},
        .FormatSubchunkID = {'f', 'm', 't', ' '},
        .FormatSubchunkSize = 16,
        .AudioFormat = 1,
        .Channels = 1,
        .SamplesRate = 44100,
        .ByteRate = 44100,
        .BlockAlign = 2,
        .BitsPerSample = 16,
        .DataSubchunkID = {'d', 'a', 't', 'a'},
        .DataSubchunkSize = frames,
    };
    FILE *out = NULL;
    out = fopen(name, "wb+");
    if (out) {
        fwrite(&wave, 1, sizeof(wave), out);
        fwrite(capture, 1, frames * sizeof(int16_t), out);
        fclose(out);
    } else {
        perror("! fopen");
        return -1;
    }
    return frames;
}

int mw_frames(char *name) {
    FILE *in = fopen(name, "rb");

    if (in) {
        wav_t wav;
        int frames = -1;
        int n = fread(&wav, sizeof(wav_t), 1, in);
        if (n > 0) {
            while (1) {
                if (strncmp(wav.RIFFChunkID, "RIFF", 4) != 0) break;
                if (strncmp(wav.Format, "WAVE", 4) != 0) break;
                if (strncmp(wav.FormatSubchunkID, "fmt ", 4) != 0) break;
                if (wav.Channels > 2) break;
                if (wav.SamplesRate != 44100) break;
                if (wav.BitsPerSample != 16) break;
                if (strncmp(wav.DataSubchunkID, "data", 4) != 0) break;
                frames = wav.DataSubchunkSize / wav.Channels / (wav.BitsPerSample / 8);
                break;
            }
        }
        fclose(in);
        return frames;
    }
    return -1;    
}

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

int mw_get_smpl_loop(const char *name, int frames, mw_smpl_loop_t *loop) {
  FILE *in;
  unsigned char riff[12];

  if (!loop) return 0;
  memset(loop, 0, sizeof(*loop));
  if (!name || frames <= 0) return 0;

  in = fopen(name, "rb");
  if (!in) return 0;

  if (fread(riff, 1, sizeof(riff), in) != sizeof(riff) ||
      memcmp(riff, "RIFF", 4) != 0 ||
      memcmp(riff + 8, "WAVE", 4) != 0) {
    fclose(in);
    return 0;
  }

  for (;;) {
    unsigned char chunk[8];
    uint32_t chunk_size;
    long data_start;
    long next_chunk;

    if (fread(chunk, 1, sizeof(chunk), in) != sizeof(chunk)) break;
    chunk_size = mw_le_u32(chunk + 4);
    data_start = ftell(in);
    if (data_start < 0) break;
    next_chunk = data_start + (long)chunk_size + (long)(chunk_size & 1u);

    if (memcmp(chunk, "smpl", 4) == 0) {
      unsigned char header[36];
      unsigned char sample_loop[24];
      uint32_t sample_loop_count;
      uint32_t type;
      uint32_t start;
      uint32_t end;
      uint32_t play_count;
      int end_exclusive;

      if (chunk_size < sizeof(header) + sizeof(sample_loop)) break;
      if (fread(header, 1, sizeof(header), in) != sizeof(header)) break;
      sample_loop_count = mw_le_u32(header + 28);
      if (sample_loop_count == 0) break;
      if (fread(sample_loop, 1, sizeof(sample_loop), in) != sizeof(sample_loop)) break;

      type = mw_le_u32(sample_loop + 4);
      start = mw_le_u32(sample_loop + 8);
      end = mw_le_u32(sample_loop + 12);
      play_count = mw_le_u32(sample_loop + 20);

      if (start >= (uint32_t)frames || end < start) break;
      if (end >= (uint32_t)frames) {
        end_exclusive = frames;
      } else {
        end_exclusive = (int)end + 1;
      }
      if (end_exclusive <= (int)start) break;

      loop->found = 1;
      loop->start = (int)start;
      loop->end = end_exclusive;
      loop->type = (int)type;
      loop->play_count = (int)play_count;
      fclose(in);
      return 1;
    }

    if (fseek(in, next_chunk, SEEK_SET) != 0) break;
  }

  fclose(in);
  return 0;
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

#include "miniaudio.h"

float _mw_safe[] = {0,0};

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
