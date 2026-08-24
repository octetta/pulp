# Skode Arrangement Guide: Music for the Funeral of Queen Mary

This guide demonstrates how to use Skode's structural sequencing capabilities to arrange a full piece of music. We use Henry Purcell's *Music for the Funeral of Queen Mary* (specifically the March, famous for its Wendy Carlos synthesized arrangement in *A Clockwork Orange*) as our template.

The accompanying Skode script (`queen_mary.sk`) shows how to implement the structure of the piece using a **Master Timeline** paradigm with modular sub-patterns.

## The Structure of the March
The piece features distinct, block-like sections:
1. **Intro**: A slow, rhythmic timpani solo (2 Bars).
2. **Theme A**: A solemn, moving brass/vocoder theme (4 Bars).
3. **Theme B**: A descending, slightly faster-paced theme (4 Bars).
4. **Outro**: Return to the timpani solo (2 Bars).

## The Master Timeline Paradigm

Instead of cramming everything into one pattern, we separate the music into multiple patterns (Pattern 1 for Timpani, Pattern 2 for Theme A, Pattern 3 for Theme B). 

We then use a "Master Pattern" (Pattern 0) to conduct them using the `/z` (Pattern State) opcode. 

The secret to this paradigm is **Clock Division Alignment**. 
* We know our base tick is a 16th note. 
* Timpani is 2 bars long (32 ticks). Theme A and B are 4 bars long (64 ticks).
* If we set our Master Pattern to run at `z%32` (1 step = 32 ticks = 2 bars), it perfectly acts as a high-level block arranger!

```skred
y0
z%32
[ /z 1 1 ] x 0                 # Step 0 (Bars 1-2): Timpani starts
[ /z 1 0   /z 2 1 ] x 1        # Step 1 (Bars 3-4): Timpani stops, Theme A starts
# Step 2 (Bars 5-6): Theme A naturally continues playing its second half...
[ /z 2 0   /z 3 1 ] x 3        # Step 3 (Bars 7-8): Theme A stops, Theme B starts
```

**Pros:**
*   Extremely modular; you can develop, loop, and tweak Theme A completely independently of the rest of the song.
*   The master sequence reads exactly like a high-level DAW arranger view.

## Note-Offs and Envelopes (`l 0`)
In Skode, the `l` command triggers the envelope (velocity). 
* The **Timpani** has an ADSR envelope with `0` sustain (`t 0.01 0.4 0 0`). Because it doesn't sustain, the notes naturally die out without you having to explicitly stop them.
* The **Brass** has an envelope with `0.8` sustain (`t 0.1 0.1 0.8 0.4`). These notes will sustain *forever* until they receive a note-off command! 
To prevent the notes from turning into a muddy, overlapping mess, Pattern 2 explicitly sends `l 0` to release the previous notes just before triggering the next ones.
