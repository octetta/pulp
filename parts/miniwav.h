#ifndef _MINIWAV_H_
#define _MINIWAV_H_
#include <stdio.h>
#include <stdint.h>

typedef struct {
    char      RIFFChunkID[4];
    uint32_t  RIFFChunkSize;
    char      Format[4];
    char      FormatSubchunkID[4];
    uint32_t  FormatSubchunkSize;
    uint16_t  AudioFormat;
    uint16_t  Channels;
    uint32_t  SamplesRate;
    uint32_t  ByteRate;
    uint16_t  BlockAlign;
    uint16_t  BitsPerSample;
    char      DataSubchunkID[4];
    uint32_t  DataSubchunkSize;
    // double* Data; 
} wav_t;

// interesting chunk that i might look for one day
typedef struct {
    char      RIFFChunkID[4];
    uint32_t  RIFFChunkSize;
    uint32_t  Manufacturer;
    uint32_t  Product;
    uint32_t  SamplePeriod;
    uint32_t  MIDIUnityNote;
    uint32_t  MIDIPitchFraction;
    uint32_t  SMPTEFormat;
    uint32_t  SMPTEOffset;
    uint32_t  SampleLoops;
    uint32_t  SamplerData;
    // hardcoded for one loop but SampleLoops by
    // the standard supports more than one
    // typedef struct {
    uint32_t Identifier;
    uint32_t Type;
    uint32_t Start;
    uint32_t End;
    uint32_t Fraction;
    uint32_t PlayCount;
    // } SampleLoop;
} sampler_t;

typedef struct {
    int found;
    int start;
    int end;
    int type;
    int play_count;
} mw_smpl_loop_t;

FILE *mw_header(char *name, wav_t *wav);

float *mw_get(char *name, int *frames_out, wav_t *w, int ch);
float *mw_get_str(char *name, int *frames_out, wav_t *w, int ch, char *out, int len);
float *mw_get_mem(const void *data, size_t data_size, const char *label,
                  int *frames_out, wav_t *w, int ch, char *out, int len);
int mw_get_smpl_loop(const char *name, int frames, mw_smpl_loop_t *loop);
int mw_get_smpl_loop_mem(const void *data, size_t data_size, int frames,
                         mw_smpl_loop_t *loop);

float *mw_free(float *f);

#endif
