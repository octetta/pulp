# A simple "Top-Level" Makefile to manage the CMake complexity
.PHONY: all clean native pi test warn

all: native # pi

maxed:
	rm -rf build_maxed
	mkdir -p build_maxed
	cd build_maxed && cmake -DKIT_OPTS="ADSR=1 AM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SEQ=1 SMOOTHER=1 UDP=1" .. && make

wasm:
	cmake -B build_wasm -S .
	cd build_wasm && cmake -DKIT_OPTS="SEQ=1 ADSR=1 AM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SMOOTHER=1" .. && make
	cp wasm-assets/mkwasm build_wasm
	cp wasm-assets/test-wasm.html build_wasm
	cp wasm-assets/serve-wasm.sh build_wasm
	cd build_wasm && ./mkwasm
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
	cmake -B build_pi -S . \
		-DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" \
		-DUSE_EXTERNAL_KIT=ON \
		-DKIT_TOOL_PATH=$(shell pwd)/build_native/kit_tool
	cmake --build build_pi

clean:
	rm -rf build_native build_pi build
