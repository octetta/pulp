# A simple "Top-Level" Makefile to manage the CMake complexity
.PHONY: all clean native pi test warn warn-maxed maxed wasm analysis-src analysis-check libs native-libs maxed-libs dist-api generated-sources clean-generated cross-windows-kit cross-windows-zig-maxed cross-windows-zig-maxed-refresh mini-skred-windows windows-cross-libs dist-api-windows

ZIG_LOCAL_CACHE_DIR ?= /tmp/pulp-zig-local-cache
ZIG_GLOBAL_CACHE_DIR ?= /tmp/pulp-zig-global-cache
EM_CACHE ?= /tmp/pulp-em-cache
ZIG_ENV = cmake -E env ZIG_LOCAL_CACHE_DIR=$(ZIG_LOCAL_CACHE_DIR) ZIG_GLOBAL_CACHE_DIR=$(ZIG_GLOBAL_CACHE_DIR)
EM_ENV = env EM_CACHE=$(EM_CACHE)
VERSION := $(shell cat ../VERSION)
DIST_ROOT ?= ../dist
HOST_OS ?= $(shell uname -s | tr '[:upper:]' '[:lower:]')
HOST_ARCH ?= $(shell uname -m)
MACOS_UNIVERSAL ?= 0
MACOS_UNIVERSAL_ARCHS ?= x86_64;arm64
DIST_PLATFORM ?= $(if $(filter 1 true yes,$(MACOS_UNIVERSAL)),macos-universal,$(HOST_OS)-$(HOST_ARCH))
DIST_PLATFORM_ROOT ?= $(DIST_ROOT)/$(DIST_PLATFORM)
DIST_NATIVE_PREFIX ?= $(DIST_PLATFORM_ROOT)/skred-$(VERSION)-native
DIST_MAXED_PREFIX ?= $(DIST_PLATFORM_ROOT)/skred-$(VERSION)-maxed
DIST_MAXED_ARCHIVE ?= skred-$(VERSION)-maxed.tar.gz
WINDOWS_DIST_PLATFORM ?= windows-x86_64
DIST_WINDOWS_PLATFORM_ROOT ?= $(DIST_ROOT)/$(WINDOWS_DIST_PLATFORM)
DIST_WINDOWS_PREFIX ?= $(DIST_WINDOWS_PLATFORM_ROOT)/skred-$(VERSION)-maxed
DIST_WINDOWS_ARCHIVE ?= skred-$(VERSION)-maxed.zip
WINDOWS_ARTIFACT_DIR ?= out/$(WINDOWS_DIST_PLATFORM)
CMAKE_PLATFORM_ARGS ?= $(if $(filter 1 true yes,$(MACOS_UNIVERSAL)),-DCMAKE_OSX_ARCHITECTURES="$(MACOS_UNIVERSAL_ARCHS)",)
MAXED_KIT_OPTS = KSYNTH=1 AM=1 XM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SEQ=1 SMOOTHER=1 UDP=1 RECORD=1 SCOPE=1
BUILD_TYPE ?= Release
WASM_KIT_OPTS = KSYNTH=1 SEQ=1 AM=1 XM=1 CRUSH=1 GLISS=1 FILT=1 FADSR=1 FM=1 PANMOD=1 PD=1 SAH=1 SMOOTHER=1 TRACKS=1
ANALYSIS_BUILD_DIR ?= build_analysis
ANALYSIS_SRC_DIR ?= analysis-src
ANALYSIS_GENERATED = \
	api.c \
	mini-skred.c \
	polyphony.c \
	recorder.c \
	recorder.h \
	seq.c \
	seq.h \
	skode.c \
	synth-alloc.c \
	synth-state.h \
	synth.c \
	synth.h

all: native # pi

maxed:
	cmake -B build_maxed -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_PLATFORM_ARGS) \
		-DKIT_OPTS="$(MAXED_KIT_OPTS)"
	cmake --build build_maxed

libs: native-libs

native-libs:
	cmake -B build_native -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_PLATFORM_ARGS)
	cmake --build build_native --target api api_shared mini-skred
	cmake --install build_native --prefix $(DIST_NATIVE_PREFIX)

maxed-libs:
	cmake -B build_maxed -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_PLATFORM_ARGS) \
		-DKIT_OPTS="$(MAXED_KIT_OPTS)"
	cmake --build build_maxed --target api api_shared mini-skred
	cmake --install build_maxed --prefix $(DIST_MAXED_PREFIX)

dist-api: maxed-libs
	cmake -E chdir $(DIST_PLATFORM_ROOT) cmake -E tar czf $(DIST_MAXED_ARCHIVE) \
		--format=gnutar skred-$(VERSION)-maxed

generated-sources: native
	cmake --build build_native --target generated-sources

clean-generated: native
	cmake --build build_native --target clean-generated

cross-windows-kit:
	cmake --preset ninja-release
	cmake --build --preset ninja-release --target kit_tool

cross-windows-zig-maxed: cross-windows-kit
	$(ZIG_ENV) cmake --preset cross-windows-zig-maxed
	$(ZIG_ENV) cmake --build --preset cross-windows-zig-maxed --target mini-skred

cross-windows-zig-maxed-refresh: cross-windows-kit
	$(ZIG_ENV) cmake --preset cross-windows-zig-maxed
	$(ZIG_ENV) cmake --build --preset cross-windows-zig-maxed --target clean-generated
	$(ZIG_ENV) cmake --build --preset cross-windows-zig-maxed --target mini-skred

mini-skred-windows: cross-windows-zig-maxed
	cmake -E make_directory $(WINDOWS_ARTIFACT_DIR)
	cmake -E copy_if_different build_cross_windows_zig_maxed/mini-skred.exe $(WINDOWS_ARTIFACT_DIR)/mini-skred.exe

windows-cross-libs: cross-windows-kit
	$(ZIG_ENV) cmake --preset cross-windows-zig-maxed
	$(ZIG_ENV) cmake --build --preset cross-windows-zig-maxed --target api api_shared mini-skred
	cmake --install build_cross_windows_zig_maxed --prefix $(DIST_WINDOWS_PREFIX)

dist-api-windows: windows-cross-libs
	cmake -E chdir $(DIST_WINDOWS_PLATFORM_ROOT) cmake -E tar cf $(DIST_WINDOWS_ARCHIVE) \
		--format=zip skred-$(VERSION)-maxed

analysis-src:
	cmake -B $(ANALYSIS_BUILD_DIR) -S . -DKIT_OPTS="$(MAXED_KIT_OPTS)"
	cmake --build $(ANALYSIS_BUILD_DIR) --target api mini-skred
	cmake -E make_directory $(ANALYSIS_SRC_DIR)
	cmake -E copy_if_different \
		$(addprefix $(ANALYSIS_BUILD_DIR)/,$(ANALYSIS_GENERATED)) \
		$(ANALYSIS_SRC_DIR)

analysis-check: analysis-src
	@status="$$(git status --porcelain -- $(ANALYSIS_SRC_DIR))"; \
	if [ -n "$$status" ]; then \
		printf '%s\n' "$$status"; \
		printf '%s\n' "analysis sources are stale; run 'make analysis-src' and commit the result"; \
		exit 1; \
	fi

wasm:
	cmake -B build_wasm -S . -DKIT_OPTS="$(WASM_KIT_OPTS)"
	cmake --build build_wasm
	cp wasm-assets/mkwasm build_wasm/mkwasm
	cp ../doc/index.html build_wasm/index.html
	cp wasm-assets/test-wasm.html build_wasm/test-wasm.html
	cp wasm-assets/serve-wasm.sh build_wasm/serve-wasm.sh
	cd build_wasm && $(EM_ENV) ./mkwasm
	cp build_wasm/skred_api.js ../doc
	cp build_wasm/skred_api.wasm ../doc
	cp wasm-assets/serve-wasm.sh ../doc

native:
	cmake -B build_native -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_PLATFORM_ARGS)
	cmake --build build_native --target kit_tool
	cmake --build build_native

test: native
	ctest --test-dir build_native --output-on-failure

warn:
	cmake -B build_warn -S . -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
	cmake --build build_warn
	ctest --test-dir build_warn --output-on-failure

warn-maxed:
	cmake -B build_warn_maxed -S . \
		-DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' \
		-DKIT_OPTS="$(MAXED_KIT_OPTS)"
	cmake --build build_warn_maxed
	ctest --test-dir build_warn_maxed --output-on-failure

pi: native
	$(ZIG_ENV) cmake -B build_pi -S . \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" \
		-DUSE_EXTERNAL_KIT=ON \
		-DKIT_TOOL_PATH=$(shell pwd)/build_native/kit_tool
	$(ZIG_ENV) cmake --build build_pi

clean:
	rm -rf build_native build_pi build_warn build_warn_maxed build_maxed build_wasm build_pi_check build build_cross_windows_zig_maxed

very-all:
	make clean maxed dist-api analysis-src wasm
