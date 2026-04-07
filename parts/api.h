#ifndef SKRED_API_H
#define SKRED_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the audio engine, state, and networking
int skred_start(unsigned int req_audio_frames, unsigned int voices, int port);

// Send an ASCII control protocol message to the engine
int skred_command(char* cmd);

// Safely tear down resources
void skred_stop(void);

// list of included features
char *skred_features(void);

#ifdef __cplusplus
}
#endif

#endif // SKRED_API_H