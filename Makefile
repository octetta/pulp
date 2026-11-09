HOSTCC ?= cc
CMAKE ?= cmake

DEF_FILES := $(shell find doc/opcodes -type f -name '*.def' 2>/dev/null)
GEN_OUTPUTS := src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

.PHONY: all gen test clean wasm wasm-test

all:
	$(MAKE) -C parts all

tools/mkopcodes: tools/mkopcodes.c
	$(HOSTCC) -O2 -Wall -Wextra tools/mkopcodes.c -o tools/mkopcodes

$(GEN_OUTPUTS): tools/mkopcodes $(DEF_FILES)
	./tools/mkopcodes doc/opcodes src/gen/skode_dict_gen.h src/gen/skred_opcodes_gen.c doc/REFERENCE.md src/gen/skode-doc.inc

gen: $(GEN_OUTPUTS)

test: gen
	$(CMAKE) -B parts/build_wasm -S parts
	$(CMAKE) --build parts/build_wasm --target test_dict_scheduler
	./parts/build_wasm/test_dict_scheduler

clean:
	$(MAKE) -C parts clean
	rm -f tools/mkopcodes

wasm: gen
	$(MAKE) -C parts wasm

wasm-test:
	$(MAKE) -C parts wasm-test
