#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <processthreadsapi.h>
#endif

#include "skode-internal.h"
#ifdef UDP
#include "udp.h"
#endif

void voice_show(skode_t *ctx, int v, char c, int verbose) {
  char s[1024];
  char e[8] = "";
  if (c != ' ') sprintf(e, " # *");
  voice_format(v, s, sizeof(s), verbose);
  if (strlen(s)) ctx->printf(ctx, "%s%s\n", s, e);
}

int voice_show_all(skode_t *ctx, int voice, int verbose) {
  for (int i=0; i<synth_config.voice_max; i++) {
    if (sv.user_amp[i] <= SILENT) continue;
    char t = ' ';
    if (i == voice) t = '*';
    voice_show(ctx, i, t, verbose);
  }
  return 0;
}

void record_tracks_show(skode_t *ctx) {
  for (int track = 1; track <= RECORD_TRACK_MAX; track++) {
    const char *name = synth_track_name_get(track);
    ctx->printf(ctx, "[%s] rt%d rv%d,%g #",
      name && name[0] ? name : "",
      track, track, synth_track_volume_db_get(track));
    for (int voice = 0; voice < synth_config.voice_max; voice++) {
      if (synth_record_track_get(voice) == track)
        ctx->printf(ctx, " v%d", voice);
    }
    ctx->puts(ctx, "");
  }
}

void system_show(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  #ifdef UDP
  int u = udp_info();
  int e = skred_udp_events_info();
  ctx->printf(ctx, "# udp_port %d (%s) udp_events_port %d (%s)\n", 
              u, u > 0 ? "OK" : "Off/Fail",
              e, e > 0 ? "OK" : "Off/Fail");
  #endif
}

void skode_macros_show(skode_t *ctx, int pasteable) {
  char name[ANDS_MACRO_NAME_LEN];
  char body[ANDS_MACRO_BODY_LEN];
  int arg_count = 0;
  int count = ands_macro_count(ctx->parse);
  if (count == 0) {
    if (!pasteable) ctx->printf(ctx, "# macros empty\n");
  } else {
    for (int i = 0; i < count; i++) {
      if (ands_macro_get(ctx->parse, i, name, sizeof(name),
                         body, sizeof(body), &arg_count)) {
        int status = ands_macro_status(ctx->parse, i);
        const char *status_name =
          status == ANDS_MACRO_REALTIME ? "realtime" :
          status == ANDS_MACRO_IMMEDIATE ? "immediate" :
          status == ANDS_MACRO_INVALID ? "invalid" :
          status == ANDS_MACRO_TOO_LARGE ? "too-large" : "unchecked";
        if (pasteable) {
          ctx->printf(ctx, "[%s] :%s ;\n", name, body);
        } else {
          ctx->printf(ctx, "# [%s] :%s ; # @%d %s\n",
            name, body, arg_count, status_name);
        }
      }
    }
  }
}

void wave_labels_show(skode_t *ctx) {
  for (int wave = 0; wave < synth_config.wave_table_max; wave++) {
    if (!sw.data[wave] || sw.size[wave] <= 0 || sw.readonly[wave])
      continue;
    if (sw.name[wave][0] != '\0')
      ctx->printf(ctx, "[%s] wt%d\n", sw.name[wave], wave);
    ctx->printf(ctx, "WL%d,%d,%d\n",
      wave, sw.loop_start[wave], sw.loop_end[wave]);
  }
}

void global_status_show(skode_t *ctx, int full) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
  ctx->printf(ctx, "# skred_version %s\n", skred_version());
  ctx->printf(ctx, "V%g\n", volume_get());
  ctx->printf(ctx, "M%g\n", tempo_bpm_get());
  ctx->printf(ctx, "# sample_rate %d voices %d waves %d\n",
    synth_sample_rate_get(), synth_config.voice_max, synth_config.wave_table_max);
  ctx->printf(ctx, "%s", delay_format());
  if (!full) return;

  ctx->printf(ctx, "# skred_text_state 1\n");
  ctx->printf(ctx, "# wavetable sample data is not embedded in this text snapshot\n");
  skode_macros_show(ctx, 1);
  wave_labels_show(ctx);
  record_tracks_show(ctx);
  voice_show_all(ctx, ctx->voice, 0);
  ctx->printf(ctx, "%s", skred_poly_group_status(-1));
  ctx->printf(ctx, "%s", skred_poly_pool_status(-1));
  for (int pattern = 0; pattern < PATTERNS_MAX; pattern++) {
    if (seq_pattern_length[pattern] > 0 || seq_state[pattern] != 0)
      pattern_show(ctx, pattern, 1);
  }
  for (int i = 0; i < ANDS_VAR_MAX; i++) {
    if (global_var[i] != 0.0) {
      ctx->printf(ctx, "%d %g =\n", i, global_var[i]);
    }
  }
  for (int i = 0; i < ANDS_VAR_MAX; i++) {
    if (global_stream[i].len > 0) {
      ctx->printf(ctx, "( ");
      for (int j = 0; j < global_stream[i].len; j++) {
        if (isnan(global_stream[i].data[j])) {
          ctx->printf(ctx, "- ");
        } else {
          ctx->printf(ctx, "%g ", global_stream[i].data[j]);
        }
      }
      ctx->printf(ctx, ") /SS %d ", i);
      if (global_stream[i].mode != 0) {
        ctx->printf(ctx, "/SM %d %d ", i, global_stream[i].mode);
      }
      if (global_stream[i].pos != 0) {
        ctx->printf(ctx, "/SP %d %d ", i, global_stream[i].pos);
      }
      ctx->printf(ctx, "# len=%d\n", global_stream[i].len);
    }
  }
}

int show_stats_cb(int n, uint64_t timestamp, uint64_t id, int tag, const event_t *e, void *user) {
  uint64_t now = SAMPLE_COUNT_GET();
  uint64_t then = timestamp - now;
  double ms = (double)then / (double)MAIN_SAMPLE_RATE * 1000.0;
  skode_t *ctx = user;
  ctx->printf(ctx, "# (%d,%ld,%d) %ld +%g ms %d opcode:%u argc:%u\n",
    n,
    id,
    tag,
    timestamp,
    ms,
    e->voice,
    e->opcode.code,
    (unsigned)e->opcode.argc);
  return 0;
}

void show_stats(skode_t *ctx) {
  ctx->printf(ctx, "# synth frames per callback %d : %gms\n",
    synth_frames_per_callback, SAMPLES_TO_MSEC(synth_frames_per_callback));
  ctx->printf(ctx, "# seq frames per callback %d : %gms\n",
    seq_frames_per_callback, SAMPLES_TO_MSEC(seq_frames_per_callback));
  ctx->printf(ctx, "# queue_size %d\n", seq_queued());
  seq_foreach(show_stats_cb, ctx);
}

const char *control_event_type_name(uint32_t type) {
  switch (type) {
    case SKRED_CONTROL_EVENT_VOICE_TRIGGER: return "VOICE_TRIGGER";
    case SKRED_CONTROL_EVENT_VOICE_RELEASE: return "VOICE_RELEASE";
    case SKRED_CONTROL_EVENT_VOICE_FINISHED: return "VOICE_FINISHED";
    case SKRED_CONTROL_EVENT_USER: return "USER";
    case SKRED_CONTROL_EVENT_PATTERN_START: return "PATTERN_START";
    case SKRED_CONTROL_EVENT_PATTERN_END: return "PATTERN_END";
    case SKRED_CONTROL_EVENT_PATTERN_WAIT: return "PATTERN_WAIT";
    case SKRED_CONTROL_EVENT_PATTERN_STEP: return "PATTERN_STEP";
    case SKRED_CONTROL_EVENT_PATTERN_CHANGE: return "PATTERN_CHANGE";
    case SKRED_CONTROL_EVENT_TEMPO_CHANGE: return "TEMPO_CHANGE";
    case SKRED_CONTROL_EVENT_PATTERN_QUEUE: return "PATTERN_QUEUE";
    case SKRED_CONTROL_EVENT_MUTE_CHANGE: return "MUTE_CHANGE";
    case SKRED_CONTROL_EVENT_ERROR: return "ERROR";
    case SKRED_CONTROL_EVENT_PATTERN_DOWNBEAT_SWITCH: return "PATTERN_DOWNBEAT_SWITCH";
    case SKRED_CONTROL_EVENT_MIDI: return "MIDI";
    default: return "UNKNOWN";
  }
}

void control_event_show(skode_t *ctx, int consume) {
  skred_control_event_t events[128];
  int count = consume ?
    skred_control_event_poll(events, 128) :
    skred_control_event_snapshot(events, 128);
  if (count <= 0) {
    ctx->puts(ctx, "# control events empty");
    return;
  }
  ctx->printf(ctx, "# control events:%d dropped:%" PRIu64 "%s\n",
    count, skred_control_event_dropped(),
    consume ? "" : " snapshot");
  for (int i = 0; i < count; i++) {
    skred_control_event_t *event = &events[i];
    ctx->printf(ctx,
      "# control %02d seq:%" PRIu64 " type:%s sample:%" PRIu64
      " voice:%d pattern:%d step:%d tag:%d opcode:%d%s%s\n",
      i,
      event->sequence,
      control_event_type_name(event->type),
      event->sample,
      event->voice,
      event->pattern,
      event->step,
      event->tag,
      (int)event->opcode,
      skode_opcode_name((uint8_t)event->opcode) ? " " : "",
      skode_opcode_name((uint8_t)event->opcode) ? skode_opcode_name((uint8_t)event->opcode) : "");
    if (event->type == SKRED_CONTROL_EVENT_USER) {
      ctx->printf(ctx, "#   id:%d", event->id);
      for (uint32_t a = 0; a < event->value_count && a < 3; a++)
        ctx->printf(ctx, " value%u:%g", (unsigned)a, event->value[a]);
      ctx->puts(ctx, "");
    }
  }
}

void opcode_arg_show(skode_t *ctx, const opcode_event_t *opcode,
    int n) {
  if (opcode->var_mask & (1U << n)) {
    ctx->printf(ctx, " $%d", (int)opcode->arg[n]);
  } else if (isnan(opcode->arg[n]) &&
      ((uint8_t)opcode->mode & (1U << n))) {
    ctx->printf(ctx, " -");
  } else {
    ctx->printf(ctx, " %g", opcode->arg[n]);
  }
}

void opcode_show(skode_t *ctx, int index,
    const opcode_event_t *opcode) {
  const char *name = skode_opcode_name(opcode->code);
  ctx->printf(ctx, "#   %02d %d%s%s", index,
    opcode->code, name ? " " : "", name ? name : "");
  if (opcode->code == SKODE_OP_DELAY)
    ctx->printf(ctx, " %c", opcode->mode);
  for (int i = 0; i < opcode->argc; i++)
    opcode_arg_show(ctx, opcode, i);
  ctx->puts(ctx, "");
}

void opcode_queue_show(skode_t *ctx) {
  int total = skred_scheduled_event_count();
  skred_scheduled_event_t events[128];
  int count = skred_scheduled_event_snapshot(events, 128);
  if (count < 0) {
    ctx->puts(ctx, "# opcode queue snapshot failed");
    return;
  }
  ctx->printf(ctx, "# opcode queue size:%d\n", total);
  int shown = count < 128 ? count : 128;
  for (int n = 0; n < shown; n++) {
    skred_scheduled_event_t *event = &events[n];
    uint64_t now = SAMPLE_COUNT_GET();
    double ms = event->timestamp >= now ?
      (double)(event->timestamp - now) * 1000.0 / MAIN_SAMPLE_RATE :
      -(double)(now - event->timestamp) * 1000.0 / MAIN_SAMPLE_RATE;
    ctx->printf(ctx, "# queue %02d id:%" PRIu64 " tag:%d at:%" PRIu64
      " %+.3fms voice:", event->index, event->id, event->tag,
      event->timestamp, ms);
    if (event->voice_var)
      ctx->printf(ctx, "$%u", (unsigned)event->voice_var - 1);
    else
      ctx->printf(ctx, "%d", event->voice);
    opcode_event_t opcode = {
      .code = event->opcode,
      .argc = event->opcode_argc,
      .mode = event->opcode_mode,
      .var_mask = event->opcode_var_mask,
    };
    for (int i = 0; i < SEQ_OPCODE_ARG_MAX; i++)
      opcode.arg[i] = event->opcode_arg[i];
    const char *name = skode_opcode_name(opcode.code);
    ctx->printf(ctx, " %d%s%s", opcode.code, name ? " " : "", name ? name : "");
    for (int i = 0; i < opcode.argc; i++)
      opcode_arg_show(ctx, &opcode, i);
    ctx->puts(ctx, "");
  }
  if (count > shown)
    ctx->printf(ctx, "# queue snapshot truncated: %d shown of %d\n",
      shown, count);
}

void opcode_pattern_step_show(skode_t *ctx, int pattern, int step) {
  const event_program_t *program = &seq_program[pattern][step];
  ctx->printf(ctx, "# pattern:%d step:%d source:[%s]\n",
    pattern, step, seq_pattern[pattern][step]);
  if (program->count == 0) {
    ctx->puts(ctx, "#   (no-op)");
    return;
  }
  for (int i = 0; i < program->count; i++)
    opcode_show(ctx, i, &program->op[i].opcode);
}

void opcode_pattern_show(skode_t *ctx, int pattern, int step) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) {
    ctx->printf(ctx, "# invalid opcode pattern:%d\n", pattern);
    return;
  }
  seq_edit_lock();
  if (step >= 0) {
    if (step >= SEQ_STEPS_MAX) {
      ctx->printf(ctx, "# invalid opcode step:%d\n", step);
      seq_edit_unlock();
      return;
    }
    opcode_pattern_step_show(ctx, pattern, step);
    seq_edit_unlock();
    return;
  }
  ctx->printf(ctx, "# opcode pattern:%d length:%d\n",
    pattern, seq_pattern_length[pattern]);
  for (int s = 0; s < seq_pattern_length[pattern]; s++)
    opcode_pattern_step_show(ctx, pattern, s);
  seq_edit_unlock();
}

void show_threads(skode_t *ctx) {
  skode_t wprime;
  if (ctx == NULL) {
    ctx = &wprime;
    skode_init(ctx);
  }
#ifdef _WIN32
  DWORD processId = GetCurrentProcessId();
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) {
      return;
  }

  THREADENTRY32 te32;
  te32.dwSize = sizeof(THREADENTRY32);

  if (Thread32First(hSnapshot, &te32)) {
    do {
      if (te32.th32OwnerProcessID == processId) {
        HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te32.th32ThreadID);
        if (hThread) {
          PWSTR threadName = NULL;
          HRESULT hr = GetThreadDescription(hThread, &threadName);
          if (FAILED(hr)) {
            ctx->printf(ctx, "# %lu <GetThreadDescription failed>\n", te32.th32ThreadID);
          } else if (threadName == NULL || wcslen(threadName) == 0) {
            ctx->printf(ctx, "# %lu <unnamed>\n", te32.th32ThreadID);
          } else {
            char narrowName[256];
            WideCharToMultiByte(CP_UTF8, 0, threadName, -1, narrowName, sizeof(narrowName), NULL, NULL);
            ctx->printf(ctx, "# %lu %s\n", te32.th32ThreadID, narrowName);
            LocalFree(threadName);
          }
          CloseHandle(hThread);
        } else {
          ctx->printf(ctx, "# %lu <cannot open thread>\n", te32.th32ThreadID);
        }
      }
    } while (Thread32Next(hSnapshot, &te32));
  }

  CloseHandle(hSnapshot);
#else
#ifndef __APPLE__
  DIR* dir = opendir("/proc/self/task");
  struct dirent* entry;
  if (dir == NULL) {
    perror("# failed to open /proc/self/task");
    return;
  }

  // Iterate through each thread directory
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    char path[4096], name[4096];
    name[0] = '\0';
    snprintf(path, sizeof(path), "/proc/self/task/%s/comm", entry->d_name);
    FILE* f = fopen(path, "r");
    if (f) {
      if (fgets(name, sizeof(name), f)) {
        unsigned long n = strlen(name);
        if (name[n-1] == '\r' || name[n-1] == '\n') {
          name[n-1] = '\0';
        }
      }
      fclose(f);
    }
    ctx->printf(ctx, "# %s %s\n", entry->d_name, name);
  }

  closedir(dir);
#endif
#endif
}

void pattern_show(skode_t *ctx, int pattern_pointer, int verbose);
















void pattern_show(skode_t *ctx, int pattern_pointer, int verbose) {
  if (pattern_pointer < 0 || pattern_pointer >= PATTERNS_MAX) return;
  seq_edit_lock();
  int len = seq_pattern_length[pattern_pointer];
  if (len == 0 && seq_state[pattern_pointer] == 0 && seq_mute[pattern_pointer] == 0 && verbose == 0) {
    seq_edit_unlock();
    return;
  }
  ctx->printf(ctx, "y%d %%%d z%d ym%d",
    pattern_pointer,
    seq_modulo[pattern_pointer] > 0 ? seq_modulo[pattern_pointer] : 1,
    seq_state[pattern_pointer],
    seq_mute[pattern_pointer]);
  if (seq_control_events[pattern_pointer]) ctx->printf(ctx, " yc1");
  if (seq_master_pattern_get() == pattern_pointer) ctx->printf(ctx, " yp%d", pattern_pointer);
  if (seq_text[pattern_pointer][0] != '\0') ctx->printf(ctx, " [%s] yt", seq_text[pattern_pointer]);
  ctx->puts(ctx, "");
  if (verbose) {
    for (int s = 0; s < len; s++) {
      char *line = seq_pattern[pattern_pointer][s];
      ctx->printf(ctx, "[%s] x%d", line, s);
      ctx->puts(ctx, "");
    }
  }
  seq_edit_unlock();
}

