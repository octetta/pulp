#include "api.h"
#include "synth-state.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vendor/uedit/uedit.h"

#include "miniz.h"

void usage(void) {
  printf("# skred\n");
  printf("-v<voice-count> (1 to 64)\n");
  printf("-r<requested-frame-size> (128 to ?)\n");
  printf("-n = do not use editor (for use as a subprocess)\n");
  printf("-p<udp-port> (0 means no udp)\n");
  printf("-l show output/input devices\n");
  exit(1);
}

// Standard Base64 Encoding Table
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Helper to base64 encode a buffer
static void base64_encode(const unsigned char *src, size_t len, char *out) {
    size_t i = 0, j = 0;
    for (i = 0; i < len; i += 3) {
        uint32_t octet_a = i < len ? src[i] : 0;
        uint32_t octet_b = i + 1 < len ? src[i + 1] : 0;
        uint32_t octet_c = i + 2 < len ? src[i + 2] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = i + 1 < len ? b64_table[(triple >> 6) & 0x3F] : '=';
        out[j++] = i + 2 < len ? b64_table[triple & 0x3F] : '=';
    }
    out[j] = '\0';
}

#define WAVE_CHUNK_SIZE 512 
extern void skode_log_message_raw(const char *msg); // or we use printf or skred_log directly. Wait, skred_log_append is not exposed. Let's just use printf, because mini-skred's stdout is captured by elixir! If we want it to go over UDP, we need to inject it. Wait, the user said they would rebuild. I will leave it as `printf` which goes to stdout, and they can route it to UDP if needed, OR I can use the same path they use for other logs. I'll just use printf for now, which boomboombeam captures!

static int handle_wave_command(const char *line) {
    int wave_idx = 0;
    sscanf(line, "-wave %d", &wave_idx);

    // Bounds check (assume wave_idx 0-255 is safe, or check sw.data)
    if (wave_idx < 0 || wave_idx >= synth_config.wave_table_max) return 1;

    float *wave_data_f = sw.data[wave_idx];
    size_t wave_len = sw.size[wave_idx] * sizeof(float); 

    if (!wave_data_f || wave_len == 0) {
        printf("~WAVE:START\n~WAVE:END\n");
        return 1;
    }

    uLongf comp_len = compressBound(wave_len);
    unsigned char *comp_data = malloc(comp_len);

    if (compress(comp_data, &comp_len, (const unsigned char*)wave_data_f, wave_len) == MZ_OK) {
        size_t b64_len = 4 * ((comp_len + 2) / 3) + 1;
        char *b64_str = malloc(b64_len);
        base64_encode(comp_data, comp_len, b64_str);

        size_t remaining = strlen(b64_str);
        size_t offset = 0;
        char chunk_buf[WAVE_CHUNK_SIZE + 32];

        printf("~WAVE:START\n");

        while (remaining > 0) {
            size_t to_copy = remaining > WAVE_CHUNK_SIZE ? WAVE_CHUNK_SIZE : remaining;
            snprintf(chunk_buf, sizeof(chunk_buf), "~WAVE:%.*s\n", (int)to_copy, b64_str + offset);
            printf("%s", chunk_buf);
            offset += to_copy;
            remaining -= to_copy;
        }

        printf("~WAVE:END\n");
        free(b64_str);
    }
    free(comp_data);
    return 1;
}

static int mini_run_command(const char *line) {
  if (strncmp(line, "-wave", 5) == 0) {
      return handle_wave_command(line);
  }

  char buf[1024];
  snprintf(buf, sizeof(buf), "%s", line);
  int r = skred_command(buf);
  char *log = skred_log();
  if (strlen(log)) printf("%s", log);
  if (r > 0) printf("r = %d\n", r);
  return r;
}

int main(int argc, char **argv) {
  int useue = 1;
  unsigned int vc = 64;
  unsigned int req = 128;
  int udp_port = 60440;
  int output = -1;
  int input = -1;

  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'n': useue = 0; break;
        case 'v': vc = atoi(&argv[i][2]); break;
        case 'r': req = atoi(&argv[i][2]); break;
        case 'p': udp_port = (int)strtol(&(argv[i][2]), NULL, 0); break;
        case 'l': {
          skred_enumerate_devices(0);
          skred_enumerate_devices(1);
          printf("# Output Devices\n");
          for (int i=0; i<skred_devices(0); i++) printf("%d %s\n", skred_device_idx(0, i), skred_device_str(0, i));
          printf("# Input Devices\n");
          for (int i=0; i<skred_devices(1); i++) printf("%d %s\n", skred_device_idx(1, i), skred_device_str(1, i));
          exit(1);
        } break;
        case 'i': {
          skred_enumerate_devices(1);
          input = atoi(&argv[i][2]);
          // NEED TO VALIDATE
        } break;
        case 'o': {
          skred_enumerate_devices(0);
          output = atoi(&argv[i][2]);
          // NEED TO VALIDATE
        } break;
        default:
          usage();
          break;
      }
    }
  }

  if (!useue) {
    // Force line buffering: buffers are flushed at every '\n'
    setvbuf(stdin,  NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
  }

  printf("# ( %s)\n", skred_features());
  printf("# frames/callback %d\n", req);
  printf("# voices %d\n", vc);

  skred_set_audio_device(output, input);

  if (skred_start(req, vc, udp_port) != 0) {
    return 1;
  }

  skred_logger(1);

  while (1) {
    char line[1024];
    char *out = NULL;
    if (useue) {
      int r = uedit("# ", line, sizeof(line)-1);
      if (r == 0) continue;
      if (r < 0) break;
      out = line;
    } else {
      out = fgets(line, (int)sizeof(line)-1, stdin);
      if (out == NULL) break;
      if (strlen(line) == 0) continue;
    }
    if (strlen(out) == 0) continue;
    int r = mini_run_command(out);
    if (r == 0) continue;
    if (r < 0) break;
  }

  skred_control_dispatch_stop();
  skred_stop();
  return 0;
}
