# Skode Arrangement Guide: Music for the Funeral of Queen Mary

This guide demonstrates how to use Skode's structural sequencing capabilities to arrange a full piece of music. We use Henry Purcell's *Music for the Funeral of Queen Mary* (specifically the March, famous for its Wendy Carlos synthesized arrangement in *A Clockwork Orange*) as our template.

The accompanying Skode script (`queen_mary.sk`) shows how to implement the structure of the piece using a "Song-in-a-Box" paradigm.

## The Structure of the March
The piece features distinct, block-like sections:
1. **Intro**: A slow, rhythmic timpani solo.
2. **Theme A**: A solemn, moving brass/vocoder theme (repeated twice).
3. **Theme B**: A descending, slightly faster-paced theme (repeated twice).
4. **Outro**: Return to the timpani solo.

## The Song-in-a-Box Paradigm

To keep everything inside a single, continuous timeline, we use the `/zl` (Pattern Loop) opcode to loop sections of the pattern before naturally falling through to the next section.

*   `/zl var_id limit dest_step`: **Pattern Loop**. This opcode uses a Skode variable (e.g., variable `1`) to count how many times a section has played. 
    *   If the variable is less than `limit - 1`, it increments the variable and jumps the sequencer back to `dest_step`.
    *   Once the limit is reached, it resets the variable to `0` and falls through to the next step.

```skred
y1
# Steps 0-14: Intro Timpani Hits
[ v 0 n 36 l 1 ] x 0
# ...
[ /zl 1 2 0 ] x 15  # Loops back to step 0 twice, then continues to step 16
```

**Pros:**
*   Keeps all sequence data in one unified space.
*   Doesn't require managing multiple patterns or coordinating clock divisions.

## Note on Cross-Pattern Waits
Skode also supports a Cross-Pattern Wait marker (`[-N] x step`). When a pattern hits this marker, it pauses until target pattern `N` reaches step 0. However, once the wait is fulfilled, the pattern *always loops back to its own step 0*. Because of this loop-to-start behavior, `[-N]` is perfect for lockstep polymetric drum fills and synchronized loops, but it is not suitable for linearly advancing a master sequencer through a song structure. Thus, `/zl` is the preferred tool for song arrangement!

## Next Steps
Open `queen_mary.sk` and fill in the bracketed `[ ]` steps with actual note triggers and synthesizer parameters to bring the Wendy Carlos arrangement to life!
