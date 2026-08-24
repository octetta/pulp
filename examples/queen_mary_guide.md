# Skode Arrangement Guide: Music for the Funeral of Queen Mary

This guide demonstrates how to use Skode's structural sequencing capabilities to arrange a full piece of music. We use Henry Purcell's *Music for the Funeral of Queen Mary* (specifically the March, famous for its Wendy Carlos synthesized arrangement in *A Clockwork Orange*) as our template.

The accompanying Skode script (`queen_mary.sk`) shows how to implement the structure of the piece using a **Master Timeline** paradigm with modular sub-patterns, pacing out a full 2-minute synthesizer dirge.

## The Structure of the March
The classical piece features distinct, block-like sections that are repeated to build tension. Our electronic arrangement follows this structure:
1. **Intro**: A slow, rhythmic timpani solo (2 Bars).
2. **Theme A**: A solemn, moving brass/vocoder theme (Repeated twice: 8 Bars).
3. **Theme B**: A descending, slightly faster-paced theme (Repeated twice: 8 Bars).
4. **Interlude**: A return to the timpani cadence (2 Bars).
5. **Theme A Climax**: A grand return of Theme A (4 Bars).
6. **Theme B Climax**: A grand return of Theme B (4 Bars).
7. **Outro**: The final timpani solo (2 Bars).

## The Master Timeline Paradigm

Instead of cramming everything into one pattern, we separate the music into multiple patterns (Pattern 1 for Timpani, Pattern 2 for Theme A, Pattern 3 for Theme B). 

We then use a "Master Pattern" (Pattern 0) to conduct them using the `/z` (Pattern State) opcode. 

The secret to this paradigm is **Clock Division Alignment**. 
* We know our base tick is a 16th note. 
* By setting a global tempo of 60 BPM (`M 60`), 1 bar takes exactly 4 seconds.
* Timpani is 2 bars long (32 ticks). Theme A and B are 4 bars long (64 ticks).
* If we set our Master Pattern to run at `32 %` (1 step = 32 ticks = 2 bars = 8 seconds), it perfectly acts as a high-level block arranger!

```skred
y0
32 %
[ /z 1 1 ] x 0                 # Step 0 (0:00): Timpani Intro
[ /z 1 0   /z 2 1 ] x 1        # Step 1 (0:08): Theme A (First Pass) starts
# Step 2 (0:16): Theme A naturally continues playing its second half
[ /z 2 0   /z 2 1 ] x 3        # Step 3 (0:24): Theme A (Second Pass) restarts
# ...
```

**Pros:**
*   Extremely modular; you can develop, loop, and tweak Theme A completely independently of the rest of the song.
*   The master sequence reads exactly like a high-level DAW arranger view.
*   Timestamps and transitions map cleanly to steps.

## Note-Offs and Envelopes (`l 0`)
In Skode, the `l` command triggers the envelope (velocity). 
* The **Timpani** has an ADSR envelope with `0` sustain (`t 0.01 0.6 0 0`). Because it doesn't sustain, the notes naturally die out without you having to explicitly stop them.
* The **Brass** has an envelope with `0.8` sustain (`t 0.1 0.1 0.8 0.4`). These notes will sustain *forever* until they receive a note-off command! 
To prevent the notes from turning into a muddy, overlapping mess, Pattern 2 and 3 explicitly send `l 0` to release the previous notes just before triggering the next ones.
