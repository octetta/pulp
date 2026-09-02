# I Feel Love: Skred/Skode Quirks & Lessons Learned

While building the "I Feel Love" live-coding arrangement, we navigated several fascinating quirks and edge cases in the Skred/Skode engine. If you are a livecoder, DJ, or performer using this engine, here is a breakdown of the traps we fell into and how to master them.

## 1. The `-1` Sequencer Math Bug (`z` vs `zq`)
**The Problem:** While the song was playing, triggering `y 1 z 1` (Restart Orchestrator) would sometimes instantly kill the drums instead of starting them.
**The Quirky Reason:** If you trigger a sequence (`z`) while the master clock is running, and the current master tick isn't perfectly aligned to the pattern's modulo, the C-engine's integer division (`seq_offset`) can evaluate to `-1`. Because Skred's sequencer wraps around, `-1` executes the *last step* of the pattern. Since our last step contained `[ /z 2 0 ]` (stop drums), the drums were instantly killed.
**The Fix:** Always use `zq 1` (Queue on Downbeat) instead of `z 1` when dropping elements in mid-performance. This forces the engine to wait for a master downbeat where the math perfectly aligns to `0`.

## 2. The 4-Character Macro Limit
**The Problem:** The `CrdEb` chord macro would not execute, resulting in missing chords.
**The Quirky Reason:** Deep inside `skode-dict.c`, strings are packed into 32-bit atoms. This imposes a strict **4-character limit** on macro names. `CrdEb` (5 characters) was silently truncated, meaning it was never actually called.
**The Fix:** Keep all custom macro definitions to 4 characters or fewer (e.g., we renamed it to `CrdE`).

## 3. The Step 0 Trigger Miss
**The Problem:** When chaining patterns (e.g., Orchestrator starting the Vocal Melody), the very first note on Beat 1 (`x 0`) would sometimes fail to play.
**The Quirky Reason:** Patterns started mid-tick via `/z` are queued for the *next* tick, inherently missing step 0 of the current cycle.
**The Fix:** Manually inject the first note into the caller. E.g., `[ /z 9 1 Voc 67 ]` guarantees the first note (`Voc 67`) plays exactly when the pattern is cued.

## 4. The Infinite Sub-Bass Drone (Missing Envelopes)
**The Problem:** Our audio analysis revealed a continuous, infinite 220Hz drone destroying the mix.
**The Quirky Reason:** We chained the Sub-Kick (`v 11`) to the main Kick (`v 0`) using `G 11 H 11` (Link MIDI/Velocity). However, we forgot to give `v 11` an ADSR envelope (`t`). Without `t`, Skred treats the voice as a raw oscillator. The moment the kick fired, the sub-kick turned on and never turned off.
**The Fix:** Even if a voice is chained, always give it a punchy envelope (e.g., `t 0.001 0.1 0 0`) so it decays properly.

## 5. Delay Bus vs. Multitrack Stem Pollution (`r 4 ds 0.0`)
**The Problem:** The kick drum and the Donna vocals were fighting on the same audio extraction stem.
**The Quirky Reason:** We assigned the kick to `r 4 ds 0.0`. While `ds 0.0` successfully bypasses the delay effect, `r 4` *still dumps the dry signal into recording track 4*. When we later put the vocals on Track 4, they shared the same stem.
**The Fix:** Track routing (`r`) and Delay Sends (`ds`) are linked. If you just want to isolate a dry kick, put it on an unused track. Don't share tracks between highly disparate elements if you plan to extract the stems.

## 6. Staccato vs. Legato (The Donna Summer Melisma)
**The Problem:** The vocals sounded like a choppy, robotic "yoo-who" instead of a soaring, breathy vocal run.
**The Quirky Reason:** Triggering a note with `l 1` resets the ADSR envelope, cutting the "breath". 
**The Fix:** We built two macros:
- `[Voc] : v 14 n $$0 l 1 ;` (Restarts the breath)
- `[Vo_] : v 14 n $$0 ;` (Changes pitch *without* resetting `l 1`)
Using `Vo_` allowed us to create true legato melismas (sliding pitches over a single sustained breath), giving it that legendary disco feel.
