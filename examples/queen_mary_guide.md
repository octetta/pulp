# Skode Arrangement Guide: Music for the Funeral of Queen Mary

This guide demonstrates how to use Skode's structural sequencing capabilities to arrange a full piece of music. We use Henry Purcell's *Music for the Funeral of Queen Mary* (specifically the March, famous for its Wendy Carlos synthesized arrangement in *A Clockwork Orange*) as our template.

The accompanying Skode script (`queen_mary.sk`) shows two different ways to implement the structure of the piece:

## The Structure of the March
The piece features distinct, block-like sections:
1. **Intro**: A slow, rhythmic timpani solo.
2. **Theme A**: A solemn, moving brass/vocoder theme (repeated twice).
3. **Theme B**: A descending, slightly faster-paced theme (repeated twice).
4. **Outro**: Return to the timpani solo.

## Paradigm A: The Master Orchestrator

In this approach, we separate the music into multiple patterns (e.g., Pattern 1 for Timpani, Pattern 2 for Theme A). We then use a "Master Pattern" (Pattern 0) to conduct them.

This relies heavily on two commands:
*   `[-N] x step`: **Cross-Pattern Wait**. This pauses the execution of the current pattern at the specified step until pattern `N` reaches step 0. 
*   `/z pattern state`: **Pattern State**. This real-time opcode starts (`1`) or stops (`0`) a target pattern.

By combining them, Pattern 0 can trigger the Timpani (Pattern 1), wait for it to finish a cycle, stop it, and trigger Theme A (Pattern 2).

```skred
y0 
# Wait for Pattern 1 to finish, stop it, and start Pattern 2
[-1] x1 [ /z 1 0  /z 2 1 ]
```

**Pros:**
*   Extremely modular; you can develop and tweak sections independently.
*   The master sequence reads like a high-level song arrangement.

## Paradigm B: The Song-in-a-Box

If you prefer to keep everything inside a single, continuous timeline, you can use the `/zl` (Pattern Loop) opcode to loop sections of the pattern before falling through to the next section.

*   `/zl var_id limit dest_step`: **Pattern Loop**. This opcode uses a Skode variable (e.g., variable `1`) to count how many times a section has played. 
    *   If the variable is less than `limit - 1`, it increments the variable and jumps the sequencer back to `dest_step`.
    *   Once the limit is reached, it resets the variable to `0` and falls through to the next step.

```skred
y5
# Step 0-3: Intro
x0 [ ]
x3 [ /zl 1 2 0 ]  # Loops back to step 0 twice, then continues to step 4
```

**Pros:**
*   Keeps all sequence data in one unified space.
*   Doesn't require managing multiple patterns or coordinating clock divisions.

## Next Steps
Open `queen_mary.sk` and fill in the bracketed `[ ]` steps with actual note triggers and synthesizer parameters to bring the Wendy Carlos arrangement to life!
