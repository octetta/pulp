#define SPECTRO_NOISE_FLOOR_DB -90.0f
#include "skode-internal.h"


static const float spectro_bayer4[4][4] = {
    {  0.0f/16.0f,  8.0f/16.0f,  2.0f/16.0f, 10.0f/16.0f },
    { 12.0f/16.0f,  4.0f/16.0f, 14.0f/16.0f,  6.0f/16.0f },
    {  3.0f/16.0f, 11.0f/16.0f,  1.0f/16.0f,  9.0f/16.0f },
    { 15.0f/16.0f,  7.0f/16.0f, 13.0f/16.0f,  5.0f/16.0f }
};

int wave_display_dim(double value, int fallback, int min, int max) {
    if (!isfinite(value) || value <= 0.0) return fallback;
    if (value >= max) return max;
    int n = (int)value;
    if (n < min) return min;
    if (n > max) return max;
    return n;
}

wave_stats_t wave_stats(float *data, int n) {
    wave_stats_t s = {0};
    if (!data || n <= 0) return s;

    s.min = data[0];
    s.max = data[0];
    double sum = 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        float v = data[i];
        float av = fabsf(v);
        if (v < s.min) s.min = v;
        if (v > s.max) s.max = v;
        if (av > s.peak) s.peak = av;
        if (av >= 0.999f) s.clipped++;
        if (i > 0 && ((data[i - 1] < 0.0f && v >= 0.0f) || (data[i - 1] > 0.0f && v <= 0.0f))) {
            s.zero_crossings++;
        }
        sum += v;
        sum_sq += (double)v * (double)v;
    }
    s.dc = (float)(sum / (double)n);
    s.rms = (float)sqrt(sum_sq / (double)n);
    return s;
}

void print_wave_stats(skode_t *ctx, const char *label, float *data, int n, float rate) {
    wave_stats_t s = wave_stats(data, n);
    float ms = (rate > 0.0f) ? ((float)n / rate * 1000.0f) : 0.0f;
    ctx->printf(ctx,
        "# %s |%d| (%gms) min %+0.3f max %+0.3f peak %0.3f rms %0.3f dc %+0.4f zc %d",
        label, n, ms, s.min, s.max, s.peak, s.rms, s.dc, s.zero_crossings);
    if (s.clipped) ctx->printf(ctx, " clip %d", s.clipped);
    ctx->puts(ctx, "");
}

float wave_samples_to_ms(int samples, float rate) {
    if (samples <= 0 || rate <= 0.0f) return 0.0f;
    return (float)samples / rate * 1000.0f;
}

int wave_boundary_col(int boundary, int n, int width) {
    if (width <= 1 || n <= 0) return 0;
    if (boundary < 0) boundary = 0;
    if (boundary > n) boundary = n;
    return (int)(((long long)boundary * (long long)(width - 1) + (n / 2)) / n);
}

void print_braille_cell(skode_t *ctx, unsigned int pattern) {
    ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | ((pattern >> 6) & 0x03),
                0x80 | (pattern & 0x3F));
}

void print_wave_marker_row(skode_t *ctx, int n, int width, int start, int end,
                                  int braille) {
    if (!ctx || n <= 0 || width <= 0) return;
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > n) start = n;
    if (end > n) end = n;
    if (end < start) {
        int tmp = end;
        end = start;
        start = tmp;
    }

    int col_start = wave_boundary_col(start, n, width);
    int col_end = wave_boundary_col(end, n, width);
    if (col_end < col_start) {
        int tmp = col_end;
        col_end = col_start;
        col_start = tmp;
    }

    if (braille) print_braille_cell(ctx, 0xFF); /* physical start */
    else ctx->printf(ctx, "|");
    for (int x = 0; x < width; x++) {
        if (braille) {
            unsigned int pattern = 0;
            if (x >= col_start && x <= col_end) pattern |= 0x12; /* dots 2,5 */
            if (x == col_start) pattern |= 0x47; /* left column */
            if (x == col_end) pattern |= 0xB8;   /* right column */
            print_braille_cell(ctx, pattern);
        } else {
            char ch = ' ';
            if (x >= col_start && x <= col_end) ch = '-';
            if (x == col_start) ch = '[';
            if (x == col_end) ch = ']';
            if (col_start == col_end && x == col_start) ch = '|';
            ctx->printf(ctx, "%c", ch);
        }
    }
    if (braille) print_braille_cell(ctx, 0xFF); /* physical end */
    else ctx->printf(ctx, "|");
    ctx->puts(ctx, "");
}

int skode_env_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

int skode_wave_display_use_braille(void) {
    const char *mode = getenv("SKRED_WAVE_DISPLAY");
    if (mode && mode[0]) {
        if (skode_env_eq(mode, "braille") ||
            skode_env_eq(mode, "unicode") ||
            skode_env_eq(mode, "utf8") ||
            skode_env_eq(mode, "1")) {
            return 1;
        }
        if (skode_env_eq(mode, "ascii") ||
            skode_env_eq(mode, "plain") ||
            skode_env_eq(mode, "0")) {
            return 0;
        }
    }
#if SKODE_WINDOWS_BUILD
    return 0;
#else
    return 1;
#endif
}

int wave_y_from_value(float value, float max_abs, int rows) {
    int y = (int)((value / max_abs + 1.0f) * 0.5f * (float)(rows - 1));
    if (y < 0) y = 0;
    if (y >= rows) y = rows - 1;
    return y;
}

int wave_ascii_points(float *data, int n, int width, int rows, int *y_coords, int *y_peak_min, int *y_peak_max) {
    if (!data || n <= 0 || width <= 0 || rows <= 0) return 0;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    for (int x = 0; x < width; x++) {
        float val = data[0];
        if (n > 1 && width > 1) {
            float data_pos = (float)x * (float)(n - 1) / (float)(width - 1);
            int idx = (int)data_pos;
            float fract = data_pos - (float)idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx + 1] * fract;
        }
        y_coords[x] = wave_y_from_value(val, max_abs, rows);

        int start = (int)((long long)x * n / width);
        int end = (int)((long long)(x + 1) * n / width);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = wave_y_from_value(bucket_min, max_abs, rows);
        y_peak_max[x] = wave_y_from_value(bucket_max, max_abs, rows);
    }
    return 1;
}

void print_audio_ascii_wave(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim, int labeled) {
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    int width = width_chars;
    int rows = height_chars * 2;
    if (rows < 1) rows = 1;
    int zero_y = (rows - 1) / 2;

    int *y_coords = (int *)malloc(width * sizeof(int));
    int *y_peak_min = (int *)malloc(width * sizeof(int));
    int *y_peak_max = (int *)malloc(width * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }
    if (!wave_ascii_points(data, n, width, rows, y_coords, y_peak_min, y_peak_max)) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }

    for (int y = rows - 1; y >= 0; y--) {
        ctx->printf(ctx, ":");
        for (int x = 0; x < width; x++) {
            int y_curr = y_coords[x];
            int y_prev = (x > 0) ? y_coords[x - 1] : y_curr;
            int lo = (y_curr < y_prev) ? y_curr : y_prev;
            int hi = (y_curr > y_prev) ? y_curr : y_prev;
            char ch = ' ';

            if (y >= lo && y <= hi) ch = '*';
            if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y == y_peak_min[x] && ch == ' ') ch = ':';
            if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y == y_peak_max[x] && ch == ' ') ch = ':';
            if (y == zero_y && ch == ' ') ch = '-';

            ctx->printf(ctx, "%c", ch);
        }
        ctx->printf(ctx, ":\n");
    }

    if (labeled) {
        print_wave_marker_row(ctx, n, width, offset, trim, 0);
    }

    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

void print_audio_braille_connected(skode_t *ctx, float *data, int n, int width_chars, int height_chars) {
    if (!skode_wave_display_use_braille()) {
        print_audio_ascii_wave(ctx, data, n, width_chars, height_chars, 0, n - 1, 0);
        return;
    }
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    int total_dots_y = height_chars * 4;
    int total_dots_x = width_chars * 2;
    int zero_y = (total_dots_y - 1) / 2;

    const int masks[2][4] = {
        {0x40, 0x04, 0x02, 0x01},
        {0x80, 0x20, 0x10, 0x08}
    };

    int *y_coords = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_min = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_max = (int *)malloc(total_dots_x * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }

    for (int x = 0; x < total_dots_x; x++) {
        float val = data[0];
        if (n > 1 && total_dots_x > 1) {
            float data_pos = (float)x * (n - 1) / (total_dots_x - 1);
            int idx = (int)data_pos;
            float fract = data_pos - idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx+1] * fract;
        }
        y_coords[x] = (int)((val / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));

        int start = (int)((long long)x * n / total_dots_x);
        int end = (int)((long long)(x + 1) * n / total_dots_x);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = (int)((bucket_min / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
        y_peak_max[x] = (int)((bucket_max / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
    }

    for (int r = height_chars - 1; r >= 0; r--) {
        ctx->printf(ctx, ":");
        for (int c = 0; c < width_chars; c++) {
            int pattern = 0;

            for (int dx = 0; dx < 2; dx++) {
                int x = c * 2 + dx;
                int y_curr = y_coords[x];
                int y_prev = (x > 0) ? y_coords[x - 1] : y_curr;
                int lo = (y_curr < y_prev) ? y_curr : y_prev;
                int hi = (y_curr > y_prev) ? y_curr : y_prev;

                for (int y = lo; y <= hi; y++) {
                    if (y / 4 == r) {
                        pattern |= masks[dx][y % 4];
                    }
                }
                if (zero_y / 4 == r && pattern == 0) {
                    pattern |= masks[dx][zero_y % 4];
                }
                if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_min[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_min[x] % 4];
                }
                if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_max[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_max[x] % 4];
                }
            }
            ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | (pattern >> 6), 0x80 | (pattern & 0x3F));
        }
        ctx->printf(ctx, ":\n");
    }
    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

void print_audio_braille_labeled(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim) {
    if (!skode_wave_display_use_braille()) {
        print_audio_ascii_wave(ctx, data, n, width_chars, height_chars, offset, trim, 1);
        return;
    }
    if (!data || n <= 0 || width_chars <= 0 || height_chars <= 0) return;

    float max_abs = 0.01f;
    for (int i = 0; i < n; i++) {
        float val = fabsf(data[i]);
        if (val > max_abs) max_abs = val;
    }

    int total_dots_y = height_chars * 4;
    int total_dots_x = width_chars * 2;
    int zero_y = (total_dots_y - 1) / 2;

    const int masks[2][4] = {
        {0x40, 0x04, 0x02, 0x01},
        {0x80, 0x20, 0x10, 0x08}
    };

    // Pre-calculate Y positions
    int *y_coords = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_min = (int *)malloc(total_dots_x * sizeof(int));
    int *y_peak_max = (int *)malloc(total_dots_x * sizeof(int));
    if (!y_coords || !y_peak_min || !y_peak_max) {
        free(y_coords);
        free(y_peak_min);
        free(y_peak_max);
        return;
    }
    for (int x = 0; x < total_dots_x; x++) {
        float val = data[0];
        if (n > 1 && total_dots_x > 1) {
            float data_pos = (float)x * (n - 1) / (total_dots_x - 1);
            int idx = (int)data_pos;
            float fract = data_pos - idx;
            val = data[idx];
            if (idx < n - 1) val = data[idx] * (1.0f - fract) + data[idx+1] * fract;
        }
        
        y_coords[x] = (int)((val / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));

        int start = (int)((long long)x * n / total_dots_x);
        int end = (int)((long long)(x + 1) * n / total_dots_x);
        if (end <= start) end = start + 1;
        if (end > n) end = n;
        float bucket_min = data[start];
        float bucket_max = data[start];
        for (int i = start + 1; i < end; i++) {
            if (data[i] < bucket_min) bucket_min = data[i];
            if (data[i] > bucket_max) bucket_max = data[i];
        }
        y_peak_min[x] = (int)((bucket_min / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
        y_peak_max[x] = (int)((bucket_max / max_abs + 1.0f) * 0.5f * (total_dots_y - 1));
    }

    // 1. Draw the Waveform rows
    for (int r = height_chars - 1; r >= 0; r--) {
        ctx->printf(ctx, ":");
        for (int c = 0; c < width_chars; c++) {
            int pattern = 0;
            for (int dx = 0; dx < 2; dx++) {
                int x = c * 2 + dx;
                int y_curr = y_coords[x];
                int y_prev = (x > 0) ? y_coords[x-1] : y_curr;
                int y_min = (y_curr < y_prev) ? y_curr : y_prev;
                int y_max = (y_curr > y_prev) ? y_curr : y_prev;

                for (int y = y_min; y <= y_max; y++) {
                    if (y / 4 == r) pattern |= masks[dx][y % 4];
                }
                if (zero_y / 4 == r && pattern == 0) {
                    pattern |= masks[dx][zero_y % 4];
                }
                if (abs(y_peak_min[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_min[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_min[x] % 4];
                }
                if (abs(y_peak_max[x] - y_curr) >= WAVE_PEAK_ACCENT_MIN_DELTA && y_peak_max[x] / 4 == r) {
                    pattern |= masks[dx][y_peak_max[x] % 4];
                }
            }
            ctx->printf(ctx, "%c%c%c", 0xE2, 0xA0 | (pattern >> 6), 0x80 | (pattern & 0x3F));
        }
        ctx->printf(ctx, ":\n");
    }

    print_wave_marker_row(ctx, n, width_chars, offset, trim, 1);

    free(y_coords);
    free(y_peak_min);
    free(y_peak_max);
}

int wavetable_summary_show(skode_t *ctx, int n) {
  if (skode_wave_valid(n) && sw.data[n] && sw.size[n]) {
    int readonly = sw.readonly[n];
    int refcount = sw.refcount[n];
    int size = sw.size[n];
    float rate = sw.rate[n] > 0.0f ? sw.rate[n] : (float)MAIN_SAMPLE_RATE;
    float ms = (float)size * 1000.0f / rate;
    
    if (sw.one_shot[n]) {
      ctx->printf(ctx, "# W%-3d | %6d smp | %6.1f ms | %s | ref#%-2d | 1-shot %5gHz | [%s]\n", 
        n, size, ms, readonly ? "R/O" : "R/W", refcount, rate, sw.name[n]);
    } else {
      ctx->printf(ctx, "# W%-3d | %6d smp | %6.1f ms | %s | ref#%-2d | cycle          | [%s]\n", 
        n, size, ms, readonly ? "R/O" : "R/W", refcount, sw.name[n]);
    }
  }
  return 0;
}

int wavetable_show(skode_t *ctx, int n) {
  if (skode_wave_valid(n) && sw.data[n] && sw.size[n]) {
    int readonly = sw.readonly[n];
    int refcount = sw.refcount[n];
    int size = sw.size[n];
    int loop_start = sw.loop_start[n];
    int loop_end = sw.loop_end[n];
    if (loop_start < 0) loop_start = 0;
    if (loop_end < loop_start) loop_end = loop_start;
    if (loop_end > size) loop_end = size;
    int loop_len = loop_end - loop_start;
    float rate = sw.rate[n] > 0.0f ? sw.rate[n] : (float)MAIN_SAMPLE_RATE;
    wave_stats_t stats = wave_stats(sw.data[n], size);
    ctx->printf(ctx, "# W%d", n);
    WTWFS(wave_samples_to_ms(size, rate));
    if (readonly) ctx->printf(ctx, " R/O"); else ctx->printf(ctx, " R/W");
    ctx->printf(ctx, " ref#%d", refcount);
    ctx->printf(ctx, " [%s]", sw.name[n]);
    ctx->puts(ctx, "");
    ctx->printf(ctx, "# playback rate %gHz offset %+gHz MIDI %g mode %s\n",
      sw.rate[n], sw.offset_hz[n], sw.midi_note[n],
      sw.one_shot[n] ? "one-shot" : "cycle");
    ctx->printf(ctx, "# loop %d..%d |%d| %gms\n", loop_start, loop_end, loop_len, wave_samples_to_ms(loop_len, rate));
    ctx->printf(ctx,
      "# min %+0.3f max %+0.3f peak %0.3f rms %0.3f dc %+0.4f zc %d",
      stats.min, stats.max, stats.peak, stats.rms, stats.dc, stats.zero_crossings);
    if (stats.clipped) ctx->printf(ctx, " clip %d", stats.clipped);
    ctx->puts(ctx, "");
  }
  return 0;
}

void wavetable_waveform_show(skode_t *ctx, int wave, int width, int height,
                                    int loop_start, int loop_end,
                                    const char *label) {
  if (!skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) return;
  wavetable_show(ctx, wave);
  if (label && label[0]) ctx->printf(ctx, "# %s\n", label);
  print_audio_braille_labeled(ctx, sw.data[wave], sw.size[wave], width, height,
    loop_start, loop_end);
  ctx->printf(ctx, "# wave [0..%d) baseline", sw.size[wave]);
  double rate = sw.rate[wave] > 0.0f ? sw.rate[wave] : (float)MAIN_SAMPLE_RATE;
  double ms = wave_samples_to_ms(sw.size[wave], rate);
  WTWFS(ms);
  ctx->printf(ctx, "\n# loop [%d..%d)", loop_start, loop_end);
  double lms = wave_samples_to_ms(loop_end - loop_start, rate);
  WTWFS(lms);
  ctx->puts(ctx, "");
}

void spectro_fft(spectro_cplx_t *buf, int n) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { spectro_cplx_t tmp = buf[i]; buf[i] = buf[j]; buf[j] = tmp; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -6.28318530717958647692f / (float)len;
        spectro_cplx_t wlen = { cosf(ang), sinf(ang) };
        for (int i = 0; i < n; i += len) {
            spectro_cplx_t w = { 1.0f, 0.0f };
            for (int k = 0; k < len / 2; k++) {
                spectro_cplx_t u = buf[i + k];
                spectro_cplx_t t = buf[i + k + len / 2];
                spectro_cplx_t v = { t.re * w.re - t.im * w.im, t.re * w.im + t.im * w.re };
                buf[i + k].re         = u.re + v.re;
                buf[i + k].im         = u.im + v.im;
                buf[i + k + len/2].re = u.re - v.re;
                buf[i + k + len/2].im = u.im - v.im;
                float nwre = w.re * wlen.re - w.im * wlen.im;
                float nwim = w.re * wlen.im + w.im * wlen.re;
                w.re = nwre; w.im = nwim;
            }
        }
    }
}

int spectro_next_pow2(int x) {
    int p = 1;
    while (p < x) p <<= 1;
    return p;
}

int skode_spectrogram_color_mode(void) {
    const char *mode = getenv("SKRED_SPECTROGRAM_COLOR");
    if (mode && mode[0]) {
        if (skode_env_eq(mode, "none") || skode_env_eq(mode, "mono") || skode_env_eq(mode, "0")) return 0;
        if (skode_env_eq(mode, "256")) return 1;
        if (skode_env_eq(mode, "truecolor") || skode_env_eq(mode, "24bit") || skode_env_eq(mode, "1")) return 2;
    }
    if (getenv("NO_COLOR")) return 0;
#if SKODE_WINDOWS_BUILD
    return 0;
#else
    return 1;
#endif
}

void spectro_heat_rgb(float t, int *r, int *g, int *b) {
    static const float stops[5][3] = {
        {0.0f,   0.0f,   4.0f},
        {87.0f,  16.0f,  110.0f},
        {188.0f, 55.0f,  84.0f},
        {249.0f, 142.0f, 8.0f},
        {252.0f, 255.0f, 164.0f}
    };
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float pos = t * 4.0f;
    int i0 = (int)pos;
    if (i0 > 3) i0 = 3;
    int i1 = i0 + 1;
    float frac = pos - (float)i0;
    *r = (int)(stops[i0][0] + (stops[i1][0] - stops[i0][0]) * frac);
    *g = (int)(stops[i0][1] + (stops[i1][1] - stops[i0][1]) * frac);
    *b = (int)(stops[i0][2] + (stops[i1][2] - stops[i0][2]) * frac);
}

void spectro_reset_color(skode_t *ctx, int mode) {
    if (mode) ctx->printf(ctx, "\x1b[0m");
}

int skode_spectrogram_line_budget(void) {
    const char *v = getenv("SKRED_SPECTROGRAM_LINE_BUDGET");
    if (v && v[0]) {
        int n = atoi(v);
        if (n > 0) return n;
    }
    return SPECTRO_LOG_LINE_BUDGET;
}

int spectro_row_worst_case_bytes(int mode, int width, int use_braille) {
    int glyph_bytes = use_braille ? width * 3 + 2 : width; /* +2 for ':' borders */
    int per_cell_escape = (mode == 2) ? 20 : (mode == 1 ? 12 : 0); /* "\x1b[38;2;255;255;255m" / "\x1b[38;5;231m" */
    int color_bytes = mode ? width * per_cell_escape + 4 /* trailing reset */ : 0;
    return glyph_bytes + color_bytes;
}

int spectro_fit_color_mode(skode_t *ctx, int color_mode, int width, int use_braille) {
    int budget = skode_spectrogram_line_budget();
    int original = color_mode;
    while (color_mode > 0 && spectro_row_worst_case_bytes(color_mode, width, use_braille) > budget) {
        color_mode--;
    }
    if (color_mode != original) {
        ctx->printf(ctx, "# spectrogram: width %d too wide for %s at line budget %d, using %s\n",
                    width,
                    original == 2 ? "truecolor" : "256-color",
                    budget,
                    color_mode == 1 ? "256-color" : "mono");
    }
    return color_mode;
}

void wavetable_spectrogram_show(skode_t *ctx, int wave, int width, int height,
                                        int loop_start, int loop_end,
                                        const char *label) {
    if (!skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) return;

    float *data = sw.data[wave];
    int n = sw.size[wave];
    int use_braille = skode_wave_display_use_braille();
    int color_mode = skode_spectrogram_color_mode();
    color_mode = spectro_fit_color_mode(ctx, color_mode, width, use_braille);

    wavetable_show(ctx, wave);
    if (label && label[0]) ctx->printf(ctx, "# %s\n", label);

    if (width <= 0 || height <= 0) return;

    int total_dots_x = use_braille ? width * 2 : width;
    int total_dots_y = use_braille ? height * 4 : height;

    int fft_size = spectro_next_pow2(total_dots_y * 2);
    if (fft_size < 64) fft_size = 64;
    if (fft_size > 4096) fft_size = 4096;
    int half = fft_size / 2;

    float *win = (float *)malloc((size_t)fft_size * sizeof(float));
    spectro_cplx_t *buf = (spectro_cplx_t *)malloc((size_t)fft_size * sizeof(spectro_cplx_t));
    float *mag = (float *)malloc((size_t)total_dots_x * total_dots_y * sizeof(float));
    if (!win || !buf || !mag) {
        free(win); free(buf); free(mag);
        return;
    }

    for (int i = 0; i < fft_size; i++) {
        win[i] = 0.5f - 0.5f * cosf(6.28318530717958647692f * i / (fft_size - 1));
    }

    int hop = total_dots_x > 0 ? n / total_dots_x : n;
    if (hop < 1) hop = 1;

    float max_mag = 1e-9f;
    for (int x = 0; x < total_dots_x; x++) {
        int center = x * hop + hop / 2;
        int start = center - fft_size / 2;
        for (int i = 0; i < fft_size; i++) {
            int idx = start + i;
            float s = (idx >= 0 && idx < n) ? data[idx] : 0.0f;
            buf[i].re = s * win[i];
            buf[i].im = 0.0f;
        }
        spectro_fft(buf, fft_size);
        for (int y = 0; y < total_dots_y; y++) {
            int bin = (int)(((float)y + 0.5f) * half / total_dots_y);
            if (bin >= half) bin = half - 1;
            float re = buf[bin].re, im = buf[bin].im;
            float m = sqrtf(re * re + im * im);
            mag[y * total_dots_x + x] = m;
            if (m > max_mag) max_mag = m;
        }
    }

    /* normalize to 0..1 using dB scale against the loudest bin found */
    for (int i = 0; i < total_dots_x * total_dots_y; i++) {
        float db = 20.0f * log10f(mag[i] / max_mag + 1e-9f);
        float t = (db - SPECTRO_NOISE_FLOOR_DB) / (0.0f - SPECTRO_NOISE_FLOOR_DB);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        mag[i] = t;
    }

    static const char ascii_ramp[] = " .:-=+*#%@";
    int ascii_levels = (int)sizeof(ascii_ramp) - 2;

    for (int r = height - 1; r >= 0; r--) {
        /* Track the last color actually emitted so we only send an escape
           code on a real change, and reset once at end-of-row instead of
           after every cell. Emitting set+reset per cell can blow a colored
           row up to ~10x the byte length of the plain glyphs, which is
           enough to overrun a fixed-size line buffer in some logging/
           console layers (they'll force-split or truncate long lines,
           often mid-escape-sequence, which is what corrupted output looks
           like). Coalescing keeps escape codes down to one per color
           transition instead of one per cell. */
        int last_r = -1, last_g = -1, last_b = -1, last_idx = -1;
        int row_has_color = 0;

        if (use_braille) ctx->printf(ctx, ":");
        for (int c = 0; c < width; c++) {
            if (use_braille) {
                unsigned int pattern = 0;
                float cell_sum = 0.0f;
                int cell_n = 0;
                for (int dx = 0; dx < 2; dx++) {
                    int x = c * 2 + dx;
                    for (int dy = 0; dy < 4; dy++) {
                        int y = r * 4 + dy;
                        float level = mag[y * total_dots_x + x];
                        cell_sum += level;
                        cell_n++;
                        float threshold = spectro_bayer4[y % 4][x % 4];
                        if (level > threshold * 0.6f) {
                            static const unsigned char masks[2][4] = {
                                {0x40, 0x04, 0x02, 0x01},
                                {0x80, 0x20, 0x10, 0x08}
                            };
                            pattern |= masks[dx][dy];
                        }
                    }
                }
                float cell_level = cell_n ? cell_sum / cell_n : 0.0f;
                if (color_mode && pattern) {
                    /* Quantize to 20 steps (5% granularity) so visually-flat
                       regions actually repeat the same RGB triple instead of
                       drifting by a rounding error every cell -- that's what
                       lets the "only emit on change" logic above coalesce
                       runs into one escape code instead of dozens. */
                    float cell_level_q = roundf(cell_level * 20.0f) / 20.0f;
                    int r8, g8, b8;
                    spectro_heat_rgb(cell_level_q, &r8, &g8, &b8);
                    if (color_mode == 2) {
                        if (r8 != last_r || g8 != last_g || b8 != last_b) {
                            ctx->printf(ctx, "\x1b[38;2;%d;%d;%dm", r8, g8, b8);
                            last_r = r8; last_g = g8; last_b = b8;
                        }
                    } else {
                        int ri = r8 * 5 / 255, gi = g8 * 5 / 255, bi = b8 * 5 / 255;
                        int idx = 16 + 36 * ri + 6 * gi + bi;
                        if (idx != last_idx) {
                            ctx->printf(ctx, "\x1b[38;5;%dm", idx);
                            last_idx = idx;
                        }
                    }
                    row_has_color = 1;
                }
                print_braille_cell(ctx, pattern);
            } else {
                float level = mag[r * total_dots_x + c];
                int idx = (int)(level * ascii_levels + 0.5f);
                if (idx < 0) idx = 0;
                if (idx > ascii_levels) idx = ascii_levels;
                if (color_mode && idx > 0) {
                    float level_q = roundf(level * 20.0f) / 20.0f;
                    int r8, g8, b8;
                    spectro_heat_rgb(level_q, &r8, &g8, &b8);
                    if (color_mode == 2) {
                        if (r8 != last_r || g8 != last_g || b8 != last_b) {
                            ctx->printf(ctx, "\x1b[38;2;%d;%d;%dm", r8, g8, b8);
                            last_r = r8; last_g = g8; last_b = b8;
                        }
                    } else {
                        int ri = r8 * 5 / 255, gi = g8 * 5 / 255, bi = b8 * 5 / 255;
                        int cidx = 16 + 36 * ri + 6 * gi + bi;
                        if (cidx != last_idx) {
                            ctx->printf(ctx, "\x1b[38;5;%dm", cidx);
                            last_idx = cidx;
                        }
                    }
                    row_has_color = 1;
                }
                ctx->printf(ctx, "%c", ascii_ramp[idx]);
            }
        }
        if (row_has_color) spectro_reset_color(ctx, color_mode);
        if (use_braille) ctx->printf(ctx, ":");
        ctx->printf(ctx, "\n");
    }

    print_wave_marker_row(ctx, n, width, loop_start, loop_end, use_braille);

    free(win);
    free(buf);
    free(mag);

    ctx->printf(ctx, "# spectrogram [0..%d) fft=%d", n, fft_size);
    double rate = sw.rate[wave] > 0.0f ? sw.rate[wave] : (float)MAIN_SAMPLE_RATE;
    double ms = wave_samples_to_ms(n, rate);
    WTWFS(ms);
    ctx->printf(ctx, " floor %gdB\n", SPECTRO_NOISE_FLOOR_DB);
}

