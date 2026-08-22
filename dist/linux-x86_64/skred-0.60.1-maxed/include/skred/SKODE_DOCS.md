# Skred/Pulp Scripting Reference

This document lists all available commands for `.skred` files loaded via CLAP plugin state, along with their C API mappings for developers.

## Category: Data

### `*=`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: variable-times-equal slot val0 val1

---

### `/=`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: variable-divide-equal slot val0 val1

---

### `/D`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: resize-data count

---

### `/d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: data-to-wave slot rate mode offset

---

### `=`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: variable-set slot value

---

### `=d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: assign a variable from an element of the d array =d var d-index

---

### `?d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-skode-data (summary)

---

### `D`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: data-size

---

### `W*`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: get a wavetable parameter to a variable

---

### `a=`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: variable-plus-equal slot val0 val1

---

### `d*`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show an element from d array

---

### `d>r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: data-to-rec

---

### `r>d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: recording-to-data channel

---

### `s=`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: variable-sub-equal slot val0 val1

---

### `v*`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: get a voice parameter to a variable

---

## Category: Events

### `/ce!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: control-event responder remove/clear

---

### `/ce?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: control-event responder status

---

### `/ceb`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: bind parser string to control event type key

---

### `/cer`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: control-event responder bool

---

### `/cex`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: bind external string slot to control event type key

---

### `?ce`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show control-plane event snapshot

---

### `?ce!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear outstanding control-plane events

---

### `ce`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: control-plane user event id [value0 [value1 [value2]]]

---

## Category: Files

### `%cat`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: print a text file

---

### `%cd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: change directory

---

### `%ls`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: list directory [match-type [index|-1] ]

---

### `%pwd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show vfs working directory

---

### `%z`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: mount zip-or-directory asset root

---

### `%zu`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: unmount zip asset root

---

### `/l`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: skode-load num

---

### `/ls`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: skode-load-string filename

---

### `/w`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-load num wave channel

---

### `/ws`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-load-string wave channel

---

### `GS<`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: restore complete repl session zip using parser string filename

---

### `GS>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: save complete repl session zip using parser string filename

---

## Category: Filter

### `J`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: filter-mode selector [mode] [character]; if character omitted, keeps current. mode 1=LP, 2=HP, 3=BP, 4=Notch, 5=Allpass. character 0=clean, 1=driven, 2=screamer

---

### `K`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: filter-cutoff freq

---

### `Q`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: filter resonance

---

### `fd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: filter-adsr depth

---

### `ft`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: filter-adsr A D S R

---

## Category: Ksynth

### `/k`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: ksynth-load num (verbose)

---

### `/ks`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: ksynth-load num (verbose)

---

### `d>k`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: data-to-ksynth-variable

---

### `k!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: run ksynth code in string buffer

---

### `k>d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: k results to d?

---

### `k>w`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: load latest ksynth result into wave slot rate? mode? offset?

---

### `k?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: k show last results

---

### `ks`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: run ksynth code in string buffer

---

### `kw`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wait for last ksynth request [timeout-ms]

---

### `kw>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: compatibility: copy latest ksynth result to data

---

### `w>k`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wavetable-to-ksynth-variable

---

## Category: Macros

### `/m`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: remove-ands-macro [name]

---

### `/m!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear-ands-macros

---

### `<e`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: external-string-to-skode external-index

---

### `<s`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: parser-local string slot to parser string

---

### `?m`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-ands-macros

---

### `e!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: execute-string num

---

### `e>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: skode-string-to-external external-index

---

### `e?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-execute-string [num]

---

### `s%`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: format parser string with numeric args

---

### `s>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: parser string to parser-local string slot

---

### `s?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show parser-local string slot [index]

---

## Category: Midi

### `/m?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show MIDI ports, mask, routes, and binding counts

---

### `/mC`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear all MIDI note and bend routes

---

### `/mL`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: initialize MIDI and list input and output ports

---

### `/mR`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: list MIDI note and bend routes

---

### `/mb`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: bind parser string Skode template to filtered MIDI event

---

### `/mb?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: list MIDI-to-Skode bindings

---

### `/mbC`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear all MIDI-to-Skode bindings

---

### `/mbd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: delete filtered MIDI-to-Skode binding

---

### `/mi`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: open enumerated MIDI input port index

---

### `/miV`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: create virtual MIDI input using parser string name

---

### `/mic`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: close active MIDI input

---

### `/mls`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: initialize MIDI and list input and output ports (alias for /mL)

---

### `/mo`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: open enumerated MIDI output port index

---

### `/moV`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: create virtual MIDI output using parser string name

---

### `/moc`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: close active MIDI output

---

### `/mp`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: route MIDI channel note and bend input to poly pool

---

### `/mpd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: delete MIDI channel-to-pool route

---

### `/mv`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: route MIDI channel note and bend input to voice

---

### `/mvd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: delete MIDI channel-to-voice route

---

### `MO`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: send one to three raw MIDI bytes

---

### `d>MO`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: send the data array as raw MIDI bytes

---

## Category: Misc

### `?s`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-skode-string

---

### `k`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: adsr-mode bool

---

### `wt`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: [name] wave-text-set wave-number

---

## Category: Modulation

### `A`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: AM voice depth

---

### `C`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: PD-mod voice depth

---

### `DD`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay damping track [damping] [hp]; darkens/thins the feedback repeats

---

### `DF`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay freeze track [0|1]; holds the current loop, stops writing new input

---

### `DG`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay grit track [bits] [native]; bits=0 default(12), 1-16 explicit depth, native=1 bypasses quantization entirely

---

### `DL`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay params track coarse fine feedback mod-freq mod-depth level

---

### `DL?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show track delay params

---

### `DP`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay pingpong track [0|1]; cross-feeds L/R feedback

---

### `DS`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay tempo-sync track bpm division; 1.0=quarter, 0.5=eighth, 0.75=dotted-eighth

---

### `DT`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay time-ms track ms; sets delay time directly in milliseconds

---

### `F`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: FM voice depth

---

### `FB`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: FF2 operator feedback amount

---

### `FF`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: FM mode

---

### `G`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: link-midi voice [voice]

---

### `H`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: link-velo voice [voice [voice [voice]]]

---

### `L`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: link-trigger-delay seconds

---

### `P`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pan-mod voice depth

---

### `XM`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: ring modulation osc amount

---

### `c`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: phase-distortion algo distortion

---

### `cd`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: phase-distortion envelope depth

---

### `ct`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: phase-distortion ADSR A D S R

---

### `ds`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-delay send amount; active only for routed, centered, unmodulated voices

---

### `g`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: glissando speed

---

### `s`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: volume-smooth bool

---

## Category: Parser

### `*R`

- **C API Opcode**: `None`
- **Arguments**: `0` to `ANDS_RETURN_MAX`
- **Summary**: return arguments as @0 through @9

---

### `?R`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show return registers without consuming them

---

### `EXEC`

- **C API Opcode**: `None`
- **Arguments**: `1` to `1`
- **Summary**: numeric opcode escape

---

### `clr`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear parser argument stack

---

### `drop`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: drop first parser argument

---

### `dup`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: duplicate first parser argument

---

### `over`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: duplicate second parser argument to front

---

### `rot`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: rotate first three parser arguments left

---

### `swap`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: swap first two parser arguments

---

### `wait`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: blocking msec wait

---

## Category: Polyphony

### `/pg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: define voice group group source width [root-offset]

---

### `/pg!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: refresh free instances using voice group

---

### `/pm`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pool mode pool mode [priority [articulation]]

---

### `/pp`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: define pool pool group base count [steal-policy]

---

### `/pp!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: refresh free instances in pool

---

### `/vg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice dependency graph voice [format [depth]]

---

### `?pg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show voice groups

---

### `?pp`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show voice pools and allocations

---

### `pb`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pool pitch bend pool key semitones [cents]

---

### `pn`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pool note-on pool key note velocity [cents]

---

### `pr`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pool note release pool key [release-velocity]

---

## Category: Recording

### `/r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: sample-to-wave slot mode channel

---

### `/r?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: multitrack file recording status

---

### `/rg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: start multitrack file recording

---

### `/rs`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: stop multitrack file recording

---

### `<r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: record duration source voice

---

### `>r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: normalize recording to string-named WAV file

---

### `^r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: record duration source voice ... markdown/html doesn't like <

---

## Category: Routing

### `?r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show track routing

---

### `r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: route voice to track, 0=master only, 1..4=track/delay bus

---

### `rt`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-name track

---

### `rv`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: track-volume track dB

---

## Category: Runtime

### `-restart`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: restart the audio engine [voices=v] [frames=f] [port=p]

---

### `-upwave`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: stream wave data via START DATA CANCEL COMMIT

---

### `-wave`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: dump wave slot as compressed base64

---

### `/a?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show audio device and performance status

---

### `/ai`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: select audio input index (-1 default, -2 off)

---

### `/als`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: refresh and list audio input and output devices

---

### `/ao`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: select audio output index (-1 default)

---

### `/f`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: flag-mode num

---

### `/ff`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: foreign C function slot arg...

---

### `/h`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show command help

---

### `/m_`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: benchmark voice

---

### `/md`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: midi-debug-mode bool

---

### `/q`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: quit

---

### `/s`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: system-show num

---

### `/t`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: trace-mode num

---

### `/th!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: reset skred performance counters and peak load tracking

---

### `/th?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: skred service/thread health

---

### `/v`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: verbose-mode num

---

### `I`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: log-event bool

---

### `log`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: log-enable bool

---

### `udp`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-udp

---

## Category: Scope

### `/s?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: shared-memory scope status

---

### `/sg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: start shared-memory scope publication

---

### `/ss`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: stop shared-memory scope publication

---

## Category: Sequencer

### `%`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pattern-modulus num

---

### `/SM`

- **C API Opcode**: `None`
- **Arguments**: `2` to `2`
- **Summary**: set stream mode (0=wrap fwd, 1=wrap rev, 2=pingpong, 3=clamp)

---

### `/SP`

- **C API Opcode**: `None`
- **Arguments**: `2` to `2`
- **Summary**: set stream position

---

### `/SS`

- **C API Opcode**: `None`
- **Arguments**: `1` to `1`
- **Summary**: set stream array from stack data

`/SS`
Populates a stream array with values parsed from the current stack context.
The maximum number of items that can be assigned to a stream is dictated by
`SKODE_STREAM_MAX_LEN` (default: 1024). Any elements beyond this limit will
be truncated to avoid realtime allocation faults.

---

### `<x`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: (pattern) step-string-to-skode step-number

---

### `>x`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: goto-step #

---

### `?S`

- **C API Opcode**: `None`
- **Arguments**: `0` to `1`
- **Summary**: show stream state and data

---

### `?o`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show compiled opcode queue or pattern

---

### `?q`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show scheduled opcode queue

---

### `DO?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: conditional-string-if-gt-zero number [tag]

---

### `M`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: tempo bpm

---

### `R`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: repeat-string count delay [tag]

---

### `R!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: remove-events tag

---

### `R!!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: remove all queued events

---

### `RR`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: repeat-string-tempo count delay [tag]

---

### `Y`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: clear-pattern which

---

### `Z`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: all-pattern-play-mode bool

---

### `Z?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show all patterns

---

### `eR`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: repeat-external-macro macro count seconds [tag]

---

### `eRR`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: repeat-external-macro-tempo macro count beats [tag]

---

### `x`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: set-step-string step

---

### `xa`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: append step

---

### `xg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: goto-step #

---

### `y`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: select-pattern which

---

### `yc`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pattern control-plane event publication bool

---

### `ym`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pattern-mute 0/1

---

### `ys?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: pattern dump for skrepl grid state

---

### `yt`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: {note} pattern-text

---

### `z`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: one-pattern-play-mode bool

---

### `z?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: one-pattern-play-mode bool

---

### `z??`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show all patterns

---

### `zg`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: goto-pattern-step step

---

### `zq`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: queue-pattern-start-stop mode

---

## Category: Voice

### `>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: copy-voice dest-voice

---

### `?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-voice

---

### `??`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-active-voices

---

### `GS`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show global synth status

---

### `N`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: detune-midi key cents

---

### `S`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice-reset voice

---

### `T`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: trigger

---

### `V`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: main-volume loudness

---

### `\`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: verbose-show-voice

---

### `___l`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: delayed velocity amount (doesn't propogate)

---

### `a`

- **C API Opcode**: `SKODE_OP_AMP`
- **Arguments**: `1` to `1`
- **Summary**: amp loudness

`a` set voice loudness (amplitude) in dB

---

### `ab`

- **C API Opcode**: `SKODE_OP_AMP_BEND`
- **Arguments**: `1` to `1`
- **Summary**: amp bend (-1..1)

---

### `abp`

- **C API Opcode**: `SKODE_OP_AMP_BEND_PARAM`
- **Arguments**: `1` to `2`
- **Summary**: amp bend range (dB) [offset]

---

### `f`

- **C API Opcode**: `SKODE_OP_FREQ`
- **Arguments**: `1` to `1`
- **Summary**: freq hz

---

### `fb`

- **C API Opcode**: `SKODE_OP_FREQ_BEND`
- **Arguments**: `1` to `1`
- **Summary**: freq bend (-1..1)

---

### `fbp`

- **C API Opcode**: `SKODE_OP_FREQ_BEND_PARAM`
- **Arguments**: `1` to `2`
- **Summary**: freq bend range (semitones) [offset]

---

### `l`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: velocity amount

---

### `m`

- **C API Opcode**: `SKODE_OP_MUTE`
- **Arguments**: `1` to `1`
- **Summary**: mute-audio bool

---

### `n`

- **C API Opcode**: `SKODE_OP_MIDI_NOTE`
- **Arguments**: `1` to `2`
- **Summary**: midi-freq note-number (cents)

---

### `p`

- **C API Opcode**: `SKODE_OP_PAN`
- **Arguments**: `1` to `1`
- **Summary**: pan value

---

### `t`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: adsr-set attack decay sustain release

---

### `v`

- **C API Opcode**: `SKODE_OP_VOICE`
- **Arguments**: `1` to `1`
- **Summary**: voice-select voice

---

### `v?`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-voice

---

### `v??`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: show-active-voices

---

### `vc`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice control-plane event publication bool

---

### `vt`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: [name] voice-text-set

---

## Category: Wave

### `/`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: default-wave voice

---

### `/wex`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-expand wave

---

### `B`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-loop bool

---

### `BC`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: bounded one-shot loop count

---

### `VL`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice-loop-points start end; no args resets from wave

---

### `VS`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice-set-points start end; no args resets from wave

---

### `VW`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: voice-wave-show [voice] [width height]

---

### `W`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-show which-wave

---

### `WL`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-loop-points wave start end

---

### `b`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-direction mode

---

### `h`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: sample-hold ratio [ratio] [mode]; if mode omitted, keeps current. ratio 0.0-1.0+, mode 0=hard, 1=smoothed, 2=jittered

---

### `q`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: bit-crush bit-depth [bits] [curve]; if curve omitted, keeps current. curve 0=linear, 1=companded, 2=dithered

---

### `w`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-select which-wave interpolate? mode-override?

---

### `w!`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-lock

---

### `w*`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-nudge-reset

---

### `w<`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-nudge-len

---

### `w<>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-auto-trim

---

### `w>`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-nudge-start

---

### `w>d`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-to-data

---

### `w>r`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-to-rec

---

### `w>w`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: write wavetable to string-named WAV file

---

## Category: Wave-Specto

### `WS`

- **C API Opcode**: `None`
- **Arguments**: `0` to `0`
- **Summary**: wave-show which-wave

---

