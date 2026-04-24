# A simple "Top-Level" Makefile to manage the CMake complexity
.PHONY: all clean native pi test warn maxed wasm

ZIG_LOCAL_CACHE_DIR ?= /tmp/pulp-zig-local-cache
ZIG_GLOBAL_CACHE_DIR ?= /tmp/pulp-zig-global-cache
EM_CACHE ?= /tmp/pulp-em-cache
ZIG_ENV = cmake -E env ZIG_LOCAL_CACHE_DIR=$(ZIG_LOCAL_CACHE_DIR) ZIG_GLOBAL_CACHE_DIR=$(ZIG_GLOBAL_CACHE_DIR)
EM_ENV = env EM_CACHE=$(EM_CACHE)
MAXED_KIT_OPTS = ADSR=1 AM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SEQ=1 SMOOTHER=1 UDP=1
WASM_KIT_OPTS = SEQ=1 ADSR=1 AM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SMOOTHER=1

all: native # pi

maxed:
	cmake -B build_maxed -S . -DKIT_OPTS="$(MAXED_KIT_OPTS)"
	cmake --build build_maxed

wasm:
	cmake -B build_wasm -S . -DKIT_OPTS="$(WASM_KIT_OPTS)"
	cmake --build build_wasm
	cp wasm-assets/mkwasm build_wasm/mkwasm
	cp docs/index.html build_wasm/index.html
	cp wasm-assets/test-wasm.html build_wasm/test-wasm.html
	cp wasm-assets/serve-wasm.sh build_wasm/serve-wasm.sh
	cd build_wasm && $(EM_ENV) ./mkwasm
	# cd build_wasm && ./serve-wasm.sh

native:
	cmake -B build_native -S .
	cmake --build build_native --target kit_tool
	cmake --build build_native

test: native
	ctest --test-dir build_native --output-on-failure

warn:
	cmake -B build_warn -S . -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
	cmake --build build_warn
	ctest --test-dir build_warn --output-on-failure

pi: native
	$(ZIG_ENV) cmake -B build_pi -S . \
		-DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" \
		-DUSE_EXTERNAL_KIT=ON \
		-DKIT_TOOL_PATH=$(shell pwd)/build_native/kit_tool
	$(ZIG_ENV) cmake --build build_pi

clean:
	rm -rf build_native build_pi build_warn build_maxed build_wasm build_pi_check build
