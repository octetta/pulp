HOSTCC ?= cc
CMAKE ?= cmake

DEF_FILES := $(shell find doc/opcodes -type f -name '*.def' 2>/dev/null)
GEN_OUTPUTS := src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

SYNTH_DEF_FILES := $(shell find schema -type f -name '*.def' 2>/dev/null)
SYNTH_GEN_OUTPUTS := src/gen/synth_state_gen.h src/gen/synth_alloc_gen.c src/gen/synth_voice_ops_gen.c src/gen/test_schema_bounds_gen.c src/gen/test_synth_equivalence_gen.c

.PHONY: all gen test test-schema clean wasm wasm-test

all:
	$(MAKE) -C parts all

tools/mkopcodes: tools/mkopcodes.c
	$(HOSTCC) -O2 -Wall -Wextra tools/mkopcodes.c -o tools/mkopcodes

$(GEN_OUTPUTS): tools/mkopcodes $(DEF_FILES)
	./tools/mkopcodes doc/opcodes src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

tools/mksynth: tools/mksynth.c
	$(HOSTCC) -O2 -Wall -Wextra tools/mksynth.c -o tools/mksynth

$(SYNTH_GEN_OUTPUTS): tools/mksynth $(SYNTH_DEF_FILES)
	./tools/mksynth schema src/gen $(CURDIR)/../skode/src/skode/gen
	@if command -v gleam >/dev/null 2>&1 && [ -d "$(CURDIR)/../skode" ]; then \
		(cd $(CURDIR)/../skode && gleam format >/dev/null 2>&1); \
	fi

gen: $(GEN_OUTPUTS) $(SYNTH_GEN_OUTPUTS)

test-schema: $(SYNTH_GEN_OUTPUTS)
	$(HOSTCC) -O2 -Wall -Wextra -Werror src/gen/test_schema_bounds_gen.c -o src/gen/test_schema_bounds
	./src/gen/test_schema_bounds

test: gen test-schema
	$(MAKE) -C parts test

clean:
	$(MAKE) -C parts clean
	rm -f tools/mkopcodes tools/mksynth src/gen/test_schema_bounds

wasm: gen
	$(MAKE) -C parts wasm

wasm-test:
	$(MAKE) -C parts wasm-test
