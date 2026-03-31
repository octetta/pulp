# A simple "Top-Level" Makefile to manage the CMake complexity
.PHONY: all clean native pi

all: native # pi

native:
	cmake -B build_native -S .
	cmake --build build_native --target kit_tool
	cmake --build build_native

pi: native
	cmake -B build_pi -S . \
		-DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" \
		-DUSE_EXTERNAL_KIT=ON \
		-DKIT_TOOL_PATH=$(shell pwd)/build_native/kit_tool
	cmake --build build_pi

clean:
	rm -rf build_native build_pi
