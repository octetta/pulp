#include "synth.h"
#include "synth-internal.h"
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "control-events.h"

// Process a single sample through the filter - VERY FAST
// Only multiplication and addition, no transcendental functions
float mmf_process(skred_engine_t *engine, int n, float input) {
    // Calculate output using Direct Form II - only 5 multiplies, 4 adds
    float output = sv.filter[n].b0 * input +
                  sv.filter[n].b1 * sv.filter[n].x1 +
                  sv.filter[n].b2 * sv.filter[n].x2 -
                  sv.filter[n].a1 * sv.filter[n].y1 -
                  sv.filter[n].a2 * sv.filter[n].y2;
    
    // Update delay lines
    sv.filter[n].x2 = sv.filter[n].x1;
    sv.filter[n].x1 = input;
    sv.filter[n].y2 = sv.filter[n].y1;
    sv.filter[n].y1 = output;
    if (sv.filter[n].drive > 0.0f) {
      // Standard cubic soft-clip: transparent near zero, compresses toward
      // the boundary, C1-continuous at the clamp so no audible kink.
      float d = output * sv.filter[n].drive;
      if (d > 1.0f) d = 1.0f;
      else if (d < -1.0f) d = -1.0f;
      output = (d - (d * d * d) * (1.0f / 3.0f)) * 1.5f; // Scale 2/3 peak to unity, don't divide by drive!
    }
    
    return output;
}

