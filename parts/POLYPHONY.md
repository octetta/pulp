# Skode Voice Groups, Pools, and Dependency Graphs

Skode's ordinary `v`, `n`, `l`, modulation, routing, and copy commands remain
the lowest-level voice interface. Voice groups and pools are an optional layer
above that interface. They make a multi-voice sound playable as a polyphonic or
monophonic instrument without hiding its physical voices.

## Mental Model

A **group** describes a contiguous prototype block of physical voices. It has
one root voice. Pitch and gate commands enter through the root, then the
prototype's existing `G` and `H` links decide which other members receive those
changes.

A **pool** reserves one or more equally sized blocks as playable instances of a
group. Pool configuration copies the prototype synthesis settings into every
instance and remaps dependencies that point to another prototype member.
References outside the prototype remain absolute, which permits shared LFOs or
other global modulation voices.

Groups and pools do not prevent direct `vN` editing. Pool ownership is
advisory: direct edits take effect immediately, and a later pool refresh or
allocation may replace the synthesis state of a free instance.

Serialize `/pg`, `/pp`, `/pm`, and refresh calls with the host's other Skode
configuration work. They mutate shared voice layouts and are intended for the
control path, not for concurrent calls from several host threads. The compiled
`pn`, `pr`, and `pb` operations are the bounded performance path.

All limits are fixed and allocation-free during performance:

- 16 groups
- 16 pools
- 16 voices per group
- 64 held-note records per monophonic pool
- no more physical instances than the configured synth voice count permits

## Defining a Prototype

First construct a sound using ordinary voices. This two-voice example layers
an octave above the root:

```text
v0 w0 a0 t.01,.15,.7,.35 N0 G1 H1
v1 w0 a0 t.005,.1,.5,.25 N12
```

Define group `0` from voices `0` and `1`:

```text
/pg 0,0,2,0
```

The arguments are:

```text
/pg group,source,width[,root-offset]
```

`root-offset` defaults to `0`. It is relative to `source`, so the example root
is voice `0`.

The group records the prototype layout, not a second private copy of every
synth field. Pool construction and explicit refresh read the current prototype
settings. Editing the prototype does not silently rewrite sounding instances.

Show one group or all groups:

```text
?pg 0
?pg
```

## Constructing a Polyphonic Pool

Create four two-voice instances in voices `8` through `15`:

```text
/pp 0,0,8,4,0
```

The arguments are:

```text
/pp pool,group,base,count[,steal-policy]
```

The numeric steal policies are:

| Value | Name | Behavior after checking for a free instance |
| ---: | --- | --- |
| `0` | release-oldest | Oldest releasing instance, then oldest held instance. This is the default. |
| `1` | oldest | Oldest allocated instance. |
| `2` | round-robin | Next instance in pool order. |
| `3` | quietest | Instance whose loudest member envelope currently has the lowest amplitude. |
| `4` | no-steal | Reject note-on when every instance is occupied. |

Unknown policy values reject pool construction. Every policy always prefers a
free instance.

The destination range must fit inside the runtime voice count. It may begin at
the prototype source, allowing a one-instance group to use its original
voices. Other partial overlaps with the prototype are rejected, as are
overlapping destination ranges owned by two pools.

A pool index can be redefined; sounding allocations in the old definition are
released first. A group can be restated with the same geometry, but changing
its source, width, or root is rejected while a pool still refers to it. This
prevents an existing allocator from silently acquiring the wrong physical
voice layout. Rebuild those pools with another group index when a live layout
migration is needed.

Show current allocations:

```text
?pp 0
?pp
```

The output includes copy/pasteable `/pp` and `/pm` commands, numeric values,
readable policy/mode comments, physical root voices, allocation state, keys,
notes, and velocities.

## Note Identity and Performance Commands

Pool note commands use an integer **key** that identifies a note lifetime. The
key is intentionally separate from pitch. This permits overlapping instances
of the same MIDI note and makes a late release harmless after an instance has
been stolen.

Keys must be exactly representable by the compiled float opcode format, so the
documented range is `-16777216..16777216`. Key `-1` is reserved as the
pool-wide target for pitch bend.

```text
pn pool,key,note,velocity[,cents]
pr pool,key[,release-velocity]
pb pool,key,semitones[,cents]
```

Examples:

```text
pn 0,1001,60,.8
pn 0,1002,64,.7
pb 0,1001,2
pr 0,1001
pr 0,1002
```

`pn` applies `n` and then `l` to the allocated instance root. Its cloned `G`
and `H` links propagate pitch and gate exactly as they do under manual voice
control. Member `N` values continue to provide intervals and fine detuning.

`pr` applies `l0`. Releasing an unknown or stolen key is a successful no-op,
so delayed MIDI note-off messages cannot release a newer allocation.
The optional release-velocity argument is retained for MIDI-shaped call sites;
it is currently accepted but does not alter the release envelope.

`pb` changes pitch without retriggering. A key of `-1` stores a pool-wide bend,
applies it to every held instance, and also affects later note-ons:

```text
pb 0,-1,-2,25
```

That bends the pool down 175 cents. Per-note bend remains available with a
normal key.

`pn`, `pr`, and `pb` are bounded numeric opcodes. They are valid in pattern
steps, defers, repeats, external compiled macros, and the timestamped event
queue. Group and pool configuration commands are immediate-only.

## Monophonic Mode

A monophonic pool uses its first group instance; any remaining instances stay
idle until the pool returns to polyphonic mode. Configure it after pool
construction:

```text
/pp 1,0,20,1,0
/pm 1,1,0,1
```

The arguments are:

```text
/pm pool,mode[,priority[,articulation]]
```

Modes:

| Value | Meaning |
| ---: | --- |
| `0` | Polyphonic allocation |
| `1` | Monophonic held-note selection |
| `2` | Reserved for a future arpeggiator; currently rejected |

Monophonic priorities:

| Value | Meaning |
| ---: | --- |
| `0` | Last pressed note |
| `1` | Highest note |
| `2` | Lowest note |
| `3` | First pressed note |

Articulation:

| Value | Meaning |
| ---: | --- |
| `0` | Retrigger the envelope whenever the active note changes |
| `1` | Legato: trigger only the first held note and release only after the final key is released |

Example last-note legato behavior:

```text
pn 1,2001,60,.8
pn 1,2002,67,.6
pr 1,2002
pr 1,2001
```

The pool plays 60, changes to 67, returns to the still-held 60, then releases.
The held-note ledger retains every key's pitch, cents, velocity, bend, and
press order. That fixed ledger is also the intended input set for future
arpeggiation mode `2`.

Changing `/pm` releases current allocations and clears the held-note ledger.
This makes it safe to switch an existing multi-instance pool between poly and
mono without rebuilding it.

## MIDI Input Recipes

The control-plane MIDI router can drive a configured pool directly. `/mp`
accepts `channel,pool[,bend-range]`; channels are zero-based and `.`/`-` means
all channels.

For the four-instance, two-voice polyphonic pool `0` constructed above:

```text
/mp 0 0 2
```

MIDI channel 1 now supplies note-on, note-off, and ±2-semitone pitch bend.
Every note lifetime is keyed by channel plus MIDI note, so delayed note-off and
equal notes on different accepted channels do not release the wrong instance.

For monophonic pool `1` from the previous section:

```text
/mp 0 1 12
```

This retains the pool's configured priority and articulation while allowing a
±12-semitone bend range. The router does not replace `/pm`; it only translates
MIDI performance messages into the pool's existing note, release, and bend
operations.

The simpler `/mv channel,voice[,bend-range]` route drives one physical voice
without a pool. It is useful for a basic one-note-at-a-time instrument, but it
tracks only the latest active key and cannot return to an earlier held note.
Use monophonic pool mode for held-note priority, legato, and fallback behavior.

See the “MIDI mono and poly synths” and “MIDI drum maps” recipes in
[SKODE_USER_COMMAND_REFERENCE.md](SKODE_USER_COMMAND_REFERENCE.md) for complete
voice setup examples and channel-routing variations.

## Refreshing a Group or Pool

After editing prototype voices, refresh every free instance in pools using the
group:

```text
/pg! 0
```

Refresh only one pool:

```text
/pp! 0
```

Refresh never rewrites held or releasing instances. Those instances retain
their current settings until they become free and a later refresh occurs.

## What Group Copying Includes

Group cloning uses the existing synthesis-copy behavior, resets transient
playback state, and remaps voice dependencies. It includes oscillator, wave,
range/loop, amplitude, pan, envelopes, tuning, filters, smoothing, distortion,
and enabled modulation settings.

It deliberately preserves the destination voice's:

- record/scope track assignment (`r`)
- delay send (`ds`)
- control-event publication (`vc`)

Track delay parameters are global track state and are never part of a group.

The transient state reset includes one-shot/loop progress, output sample,
ping-pong direction leg, finished flags, and sample-hold progress. Oscillator
and envelope performance state begins when the instance receives `pn`.

## Dependency Graphs

Use `/vg` to inspect all dependencies reachable from one voice:

```text
/vg 0
/vg 0,0,4
/vg 0,1,4
```

The arguments are:

```text
/vg voice[,format[,depth]]
```

Formats:

- `0`: human-readable ASCII tree (default)
- `1`: stable machine-readable graph text

Depth `0` means the configured voice count. Cycles are shown once and marked
`(seen)` in ASCII output.

Example ASCII:

```text
voice v0
|- pitch -> v1
`- gate -> v1 (seen)
```

The machine format is a line protocol designed for ro-totem and browser apps:

```text
skred-voice-graph 1
root 0
node 0
edge 0 1 0 pitch
edge 0 1 1 gate
end
```

Grammar:

```text
header = "skred-voice-graph" SP version LF
root   = "root" SP voice LF
node   = "node" SP voice LF
edge   = "edge" SP from SP to SP type SP label LF
end    = "end" LF
```

Edge types are numeric and stable:

| Type | Label | Source setting |
| ---: | --- | --- |
| `0` | pitch | `G` MIDI-note link |
| `1` | gate | `H` velocity/gate link |
| `2` | amp-mod | `A` source voice |
| `3` | freq-mod | `F` source voice |
| `4` | pan-mod | `P` source voice |
| `5` | phase-mod | `C` source voice |
| `6` | ring-mod | `XM` source voice |

Feature-gated edge kinds appear only when included in the build. Multiple edge
types between the same nodes are retained.

A browser-side parser can stay deliberately small:

```javascript
function parseVoiceGraph(text) {
  const graph = { version: 0, root: -1, nodes: [], edges: [] };
  for (const line of text.trim().split("\n")) {
    const field = line.split(/\s+/);
    if (field[0] === "skred-voice-graph") graph.version = Number(field[1]);
    else if (field[0] === "root") graph.root = Number(field[1]);
    else if (field[0] === "node") graph.nodes.push(Number(field[1]));
    else if (field[0] === "edge") graph.edges.push({
      from: Number(field[1]), to: Number(field[2]),
      type: Number(field[3]), label: field[4]
    });
  }
  if (graph.version !== 1) throw new Error("unsupported voice graph version");
  return graph;
}
```

Consumers should key edges by `(from,to,type)`, not just `(from,to)`, because
pitch and gate commonly connect the same pair of voices.

Hosts can bypass the bounded Skode log and obtain the full graph directly:

```c
const char *text = skred_voice_graph(0, 1, 0);
```

In WebAssembly:

```javascript
const ptr = Module.ccall(
  "skred_voice_graph", "number",
  ["number", "number", "number"],
  [0, 1, 0]
);
const graph = Module.UTF8ToString(ptr);
```

The returned pointer uses an internal buffer and remains valid until the next
graph request. Consumers should copy it before issuing another request.

## Complete Polyphonic Example

```text
# Prototype: root plus octave layer.
v0 w0 a0 t.01,.15,.7,.35 N0 G1 H1
v1 w0 a0 t.005,.1,.5,.25 N12

# Four instances occupy voices 8..15.
/pg 0,0,2,0
/pp 0,0,8,4,0

# Inspect the remapped first instance.
/vg 8

# Play and release a chord.
pn 0,1,60,.8
pn 0,2,64,.75
pn 0,3,67,.7
pr 0,1
pr 0,2
pr 0,3
```

See `examples/poly-layer.sk` and `examples/poly-mono.sk` for copyable examples.
