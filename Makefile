HOSTCC ?= cc
CMAKE ?= cmake

DEF_FILES := $(shell find doc/opcodes -type f -name '*.def' 2>/dev/null)
GEN_OUTPUTS := src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

SYNTH_DEF_FILES := $(shell find schema -type f -name '*.def' 2>/dev/null)
SYNTH_GEN_OUTPUTS := src/gen/synth_state_gen.h src/gen/synth_alloc_gen.c src/gen/synth_voice_ops_gen.c src/gen/test_schema_bounds_gen.c src/gen/test_synth_equivalence_gen.c

.PHONY: default help all gen test test-schema clean wasm wasm-test sync-skode

default: help

help:
	@echo "Pulp Audio Engine Build Targets (v0.85.0):"
	@echo "  make help        - Show this list of targets (default)"
	@echo "  make all         - Build native C engine, libraries, and mini-skred CLI"
	@echo "  make gen         - Run declarative compilers (mksynth and mkopcodes)"
	@echo "  make test        - Build native and run all 12/12 C engine tests"
	@echo "  make test-schema - Run automated boundary invariance tests"
	@echo "  make wasm        - Compile C DSP engine to WebAssembly (skred_api.js/wasm)"
	@echo "  make wasm-test   - Run headless Node.js tests against WASM engine"
	@echo "  make sync-skode  - Sync Gleam schemas & WASM into ../skode, format & test"
	@echo "  make clean       - Remove generated files and build directories"

all:
	$(MAKE) -C parts all

tools/mkopcodes: tools/mkopcodes.c
	$(HOSTCC) -O2 -Wall -Wextra tools/mkopcodes.c -o tools/mkopcodes

$(GEN_OUTPUTS): tools/mkopcodes $(DEF_FILES)
	./tools/mkopcodes doc/opcodes src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

tools/mksynth: tools/mksynth.c
	$(HOSTCC) -O2 -Wall -Wextra tools/mksynth.c -o tools/mksynth

$(SYNTH_GEN_OUTPUTS): tools/mksynth $(SYNTH_DEF_FILES)
	./tools/mksynth schema src/gen $(CURDIR)/../skode/src/skode/gen $(CURDIR)/VERSION
	@if command -v gleam >/dev/null 2>&1 && [ -d "$(CURDIR)/../skode" ]; then \
		(cd $(CURDIR)/../skode && gleam format >/dev/null 2>&1); \
	fi

sync-skode: $(SYNTH_GEN_OUTPUTS)
	@echo "==> Synchronizing Gleam code in skode with Pulp..."
	./tools/mksynth schema src/gen $(CURDIR)/../skode/src/skode/gen $(CURDIR)/VERSION
	@if command -v gleam >/dev/null 2>&1 && [ -d "$(CURDIR)/../skode" ]; then \
		(cd $(CURDIR)/../skode && gleam format >/dev/null 2>&1); \
	fi
	@echo "==> Copying compiled WASM binary to skode editor..."
	mkdir -p $(CURDIR)/../skode/app/editor/wasm
	cp -f parts/wasm-assets/skred_api.js $(CURDIR)/../skode/app/editor/wasm/
	cp -f parts/wasm-assets/skred_api.wasm $(CURDIR)/../skode/app/editor/wasm/
	@echo "==> Running Skode tests..."
	@if command -v gleam >/dev/null 2>&1 && [ -d "$(CURDIR)/../skode" ]; then \
		(cd $(CURDIR)/../skode && gleam test && gleam test --target javascript); \
	fi
	@echo "==> Pulp and Skode are synchronized!"

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
