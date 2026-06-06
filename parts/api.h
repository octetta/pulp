#ifndef SKRED_API_H
#define SKRED_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the audio engine, state, and networking
int skred_start(unsigned int req_audio_frames, unsigned int voices, int port);

// Send an ASCII control protocol message to the engine
int skred_command(char* cmd);

// Safely tear down resources
void skred_stop(void);

// RECORD feature: writes master plus four stereo stems to a 10-channel WAV.
int skred_record_start(const char *filename, double max_seconds);
int skred_record_stop(void);
int skred_record_state(void);
uint64_t skred_record_frames_written(void);
uint64_t skred_record_dropped_frames(void);

// list of included features
char *skred_features(void);

// did skode have anything to say?
char *skred_log(void);

// enable / disable logging
void skred_logger(int f);

// clumsy enumeration
int skred_devices(int isCapture);
int skred_device_idx(int isCapture, int idx);
char *skred_device_str(int isCapture, int idx);
int skred_enumerate_devices(int isCapture);
void skred_set_audio_device(int playback_idx, int capture_idx);

#ifdef __cplusplus
}
#endif

#endif // SKRED_API_H
