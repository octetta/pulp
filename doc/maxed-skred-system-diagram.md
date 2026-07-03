# Maxed SKRED System Diagram

## One-Page Overview

```mermaid
flowchart TB
  classDef control fill:#f7f7f7,stroke:#555,color:#111
  classDef realtime fill:#eef6ff,stroke:#246,color:#111
  classDef audio fill:#f1fbf2,stroke:#275,color:#111
  classDef observe fill:#fff7e8,stroke:#874,color:#111

  host[Host / UI / mini-skred / UDP]:::control
  api[Public API]:::control
  parser[Skode parser]:::control
  control[Immediate control\nstate, files, devices,\nrecord/scope, macros]:::control
  program[Compiled opcode programs]:::control

  clock[(Sample counter)]:::realtime
  timeline[Sequencer + scheduled queue]:::realtime
  callback[Audio callback]:::realtime
  exec[Due opcode execution]:::realtime

  voices[Voice bank]:::audio
  delay[4 stem delay buses]:::audio
  bus[10-channel bus\nmaster + 4 stereo stems]:::audio
  out[Audio output]:::audio
  record[WAV recorder]:::observe
  scope[Shared-memory scope]:::observe

  events[Control-event ring]:::observe
  dispatch[Optional response dispatcher]:::observe

  host --> api --> parser
  parser --> control
  parser --> program
  control --> voices
  control --> timeline
  control --> record
  control --> scope
  program --> timeline

  callback --> clock --> timeline
  callback --> timeline
  timeline --> exec
  exec --> voices
  exec --> events

  callback --> voices
  voices --> bus
  voices --> delay --> bus
  bus --> out
  bus --> record
  bus --> scope

  voices --> events
  timeline --> events
  events --> host
  events --> dispatch --> parser
```

## Control Path

```mermaid
flowchart LR
  host[Host command text]
  parser[Skode / ANDS parser]
  immediate[Immediate commands]
  compiled[Compiled opcode program]
  seq[Pattern steps]
  queue[Scheduled event queue]
  state[Synth / system state]

  host --> parser
  parser --> immediate --> state
  parser --> compiled
  compiled --> seq
  compiled --> queue
  immediate --> seq
  immediate --> queue
```

Text commands enter through `skred_command()` and run on the caller's control
thread. Commands that are safe for the real-time side compile into bounded
opcode programs; the audio callback never parses Skode text.

## Real-Time Path

```mermaid
flowchart LR
  clock[(Atomic sample counter)]
  callback[Audio callback]
  seq[Sequencer]
  queue[Scheduled queue]
  exec[Run due opcodes]
  voices[Voice state]

  callback --> clock
  clock --> seq
  clock --> queue
  callback --> seq
  seq --> exec
  queue --> exec
  exec --> voices
```

The sequencer and scheduled queue share the sample counter. The callback splits
render blocks at event/pattern boundaries, runs due work, then renders the next
audio segment.

## Audio And Capture Path

```mermaid
flowchart LR
  voices[Voices]
  dry[Dry stereo mix]
  stems[Stem routing\nr1..r4]
  delay[Stem delay buses\nds send]
  bus[10-channel bus]
  out[Device output]
  wav[Recorder WAV]
  shm[Scope shared memory]

  voices --> dry --> bus
  voices --> stems --> bus
  stems --> delay --> bus
  bus --> out
  bus --> wav
  bus --> shm
```

Voices always feed the stereo master unless disconnected. `r1` through `r4`
also route voices into four stereo stems. `ds` sends a centered, non-pan-modulated
voice into the delay line for its current stem; the wet delay returns to both
the master and that stem. The record/scope bus is 10 channels: master L/R plus
four stereo stems.

## Notification Path

```mermaid
flowchart LR
  voices[Voice lifecycle]
  seq[Pattern yc1 boundaries]
  ce[Explicit ce markers]
  ring[Control-event ring]
  host[Host poll / wait]
  dispatch[Optional /cer dispatcher]
  parser[Skode parser]

  voices --> ring
  seq --> ring
  ce --> ring
  ring --> host
  ring --> dispatch --> parser
```

- Scheduled opcode events are engine work waiting to happen.
- Control-plane events are notifications after work happens: voice lifecycle,
  pattern boundaries, and explicit `ce` markers.
- Hosts consume notifications by polling or waiting on the control-event ring.
- The optional response dispatcher can consume matching events and submit bound
  Skode commands back through the control path.
