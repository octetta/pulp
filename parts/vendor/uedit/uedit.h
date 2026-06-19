#ifndef UEDIT_H
#define UEDIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define GETCH() _getch()
#else
    #include <termios.h>
    #include <unistd.h>
    static int uedit_getch(void) {
        unsigned char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        return n == 1 ? (int)ch : EOF;
    }
    #define GETCH() uedit_getch()
#endif

#define UEDIT_MAX_LINE 1024
#define CTRL_KEY(k) ((k) & 0x1f)

/* History State */
static char **uedit_hist = NULL;
static int uedit_h_max = 100;
static int uedit_h_cnt = 0;
static int uedit_h_head = 0;
static char uedit_h_tmp[UEDIT_MAX_LINE] = {0}; /* Saves current line during browsing */

#ifndef _WIN32
static struct termios uedit_orig_termios;
static int uedit_raw_enabled = 0;
static int uedit_atexit_registered = 0;
static void uedit_disable_raw_mode(void) {
    if (!uedit_raw_enabled) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &uedit_orig_termios);
    uedit_raw_enabled = 0;
}
static int uedit_enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) return -1;
    if (tcgetattr(STDIN_FILENO, &uedit_orig_termios) != 0) return -1;
    if (!uedit_atexit_registered) {
        atexit(uedit_disable_raw_mode);
        uedit_atexit_registered = 1;
    }
    struct termios raw = uedit_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) return -1;
    uedit_raw_enabled = 1;
    return 0;
}
#else
static DWORD uedit_orig_in_mode, uedit_orig_out_mode;
static void uedit_disable_raw_mode(void) {
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),  uedit_orig_in_mode);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), uedit_orig_out_mode);
}
static int uedit_enable_raw_mode(void) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE), hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(hIn, &uedit_orig_in_mode) ||
        !GetConsoleMode(hOut, &uedit_orig_out_mode)) return -1;
    SetConsoleMode(hIn, uedit_orig_in_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
    SetConsoleMode(hOut, uedit_orig_out_mode | 0x0004);
    return 0;
}
#endif

/* Sets history size. If called after init, it clears existing history. */
static void uedit_config_history(int n) {
    if (uedit_hist) {
        for (int i = 0; i < uedit_h_max; i++) free(uedit_hist[i]);
        free(uedit_hist);
    }
    uedit_h_max = n;
    uedit_hist = (char **)calloc(uedit_h_max, sizeof(char *));
    uedit_h_cnt = uedit_h_head = 0;
}

static void uedit_add_history(const char *line) {
    if (!uedit_hist) uedit_config_history(uedit_h_max);
    if (!line || !line[0]) return;
    /* Avoid duplicates at the end of history */
    int last = (uedit_h_head + uedit_h_max - 1) % uedit_h_max;
    if (uedit_h_cnt > 0 && strcmp(uedit_hist[last], line) == 0) return;

    if (!uedit_hist[uedit_h_head]) uedit_hist[uedit_h_head] = malloc(UEDIT_MAX_LINE);
    if (!uedit_hist[uedit_h_head]) return;
    strncpy(uedit_hist[uedit_h_head], line, UEDIT_MAX_LINE - 1);
    uedit_hist[uedit_h_head][UEDIT_MAX_LINE - 1] = '\0';
    uedit_h_head = (uedit_h_head + 1) % uedit_h_max;
    if (uedit_h_cnt < uedit_h_max) uedit_h_cnt++;
}

static void uedit_refresh_line(const char *prompt, const char *buf, int cur) {
    printf("\r\033[K%s%s\r", prompt, buf);
    int col = (int)strlen(prompt) + cur;
    if (col > 0) printf("\033[%dC", col);
    fflush(stdout);
}

static int uedit(const char *prompt, char *buf, int max_line) {
    int len = 0, cur = 0, c, h_idx = -1;
    if (max_line <= 0) return -1;
    memset(buf, 0, max_line);
    printf("%s", prompt); fflush(stdout);
    if (uedit_enable_raw_mode() != 0) {
        if (!fgets(buf, max_line, stdin)) return -1;
        len = (int)strlen(buf);
        if (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';
        uedit_add_history(buf);
        return len;
    }

    while (1) {
        c = GETCH();
        if (c == EOF) { len = -1; break; }
        if      (c == CTRL_KEY('a')) cur = 0;
        else if (c == CTRL_KEY('e')) cur = len;
        else if (c == CTRL_KEY('u')) { buf[0] = '\0'; len = cur = 0; }
        else if (c == CTRL_KEY('k')) { buf[cur] = '\0'; len = cur; }
        else if (c == CTRL_KEY('l')) { printf("\033[H\033[2J"); }
        else if (c == '\n' || c == '\r') {
            buf[len] = '\0'; putchar('\n');
            uedit_add_history(buf);
            break;
        } else if (c == 127 || c == 8) {
            if (cur > 0) {
                memmove(&buf[cur - 1], &buf[cur], len - cur);
                len--; cur--; buf[len] = '\0';
            }
        } else if (c == 27) {
            c = GETCH();
            if (c == '[') {
                c = GETCH();
                if (c == 'D' && cur > 0) cur--;
                else if (c == 'C' && cur < len) cur++;
                else if (c == 'A' || c == 'B') { /* Up / Down */
                    if (h_idx == -1) strncpy(uedit_h_tmp, buf, UEDIT_MAX_LINE - 1);
                    if (c == 'A') { if (h_idx < uedit_h_cnt - 1) h_idx++; }
                    else { if (h_idx > -1) h_idx--; }

                    if (h_idx == -1) strncpy(buf, uedit_h_tmp, max_line - 1);
                    else {
                        int pos = (uedit_h_head - 1 - h_idx + uedit_h_max) % uedit_h_max;
                        strncpy(buf, uedit_hist[pos], max_line - 1);
                    }
                    buf[max_line - 1] = '\0';
                    len = (int)strlen(buf); cur = len;
                }
            }
        } else if (c == CTRL_KEY('d')) {
            if (len > 0 && cur < len) {
                memmove(&buf[cur], &buf[cur + 1], len - cur - 1);
                len--; buf[len] = '\0';
            } else if (len == 0) { len = -1; break; }
        } else if (c >= 32 && c <= 126 && len < max_line - 1) {
            memmove(&buf[cur + 1], &buf[cur], len - cur);
            buf[cur] = (char)c; len++; cur++;
        }
        uedit_refresh_line(prompt, buf, cur);
    }
    uedit_disable_raw_mode();
    return len;
}

#endif
