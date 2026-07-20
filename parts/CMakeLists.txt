cmake_minimum_required(VERSION 3.10)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" PULP_VERSION)
string(STRIP "${PULP_VERSION}" PULP_VERSION)
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" PULP_VERSION_MATCH "${PULP_VERSION}")
if(NOT PULP_VERSION_MATCH)
    message(FATAL_ERROR "VERSION must be MAJOR.MINOR.PATCH, got '${PULP_VERSION}'")
endif()
set(PULP_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(PULP_VERSION_MINOR "${CMAKE_MATCH_2}")
set(PULP_VERSION_PATCH "${CMAKE_MATCH_3}")

project(mini-skred VERSION "${PULP_VERSION}" LANGUAGES C)
include(GNUInstallDirs)

# 1. Build the tool
set(KIT_OPTS "" CACHE STRING "Extra flags for the kit tool")
separate_arguments(KIT_OPTS_LIST NATIVE_COMMAND "${KIT_OPTS}")
list(FIND KIT_OPTS_LIST "RECORD=1" RECORD_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "SCOPE=1" SCOPE_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "TRACKS=1" TRACKS_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "KSYNTH=1" KSYNTH_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "MIDI=1" MIDI_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "PD=1" PD_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "FM=1" FM_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "FILT=1" FILT_FEATURE_INDEX)
list(FIND KIT_OPTS_LIST "FADSR=1" FADSR_FEATURE_INDEX)
if((NOT RECORD_FEATURE_INDEX EQUAL -1 OR NOT SCOPE_FEATURE_INDEX EQUAL -1) AND TRACKS_FEATURE_INDEX EQUAL -1)
    list(APPEND KIT_OPTS_LIST "TRACKS=1")
    set(TRACKS_FEATURE_INDEX 0)
endif()
list(APPEND KIT_OPTS_LIST "ADSR=1")
list(REMOVE_DUPLICATES KIT_OPTS_LIST)
if(NOT SCOPE_FEATURE_INDEX EQUAL -1 AND WIN32)
    message(FATAL_ERROR "SCOPE=1 currently requires POSIX shared memory")
endif()
option(USE_EXTERNAL_KIT "Use an existing native kit_tool binary" OFF)
set(KIT_TOOL_PATH "" CACHE FILEPATH "Path to a prebuilt native kit_tool")

if(USE_EXTERNAL_KIT)
    if(NOT KIT_TOOL_PATH)
        message(FATAL_ERROR "USE_EXTERNAL_KIT=ON requires KIT_TOOL_PATH")
    endif()
    set(KIT_TOOL_EXE "${KIT_TOOL_PATH}")
else()
    add_executable(kit_tool kit.c)
    set(KIT_TOOL_EXE $<TARGET_FILE:kit_tool>)
endif()

function(add_kit_generated_file output input)
    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${KIT_TOOL_EXE}" ${KIT_OPTS_LIST} --input "${input}" --output "${output}"
        DEPENDS "${input}" "${ARGN}"
        VERBATIM
    )
endfunction()

# 1.1 Define the absolute paths for the generator
set(SYNTH_STATE_KIT "${CMAKE_CURRENT_SOURCE_DIR}/synth-state.h.kit")
set(SYNTH_STATE_H   "${CMAKE_CURRENT_BINARY_DIR}/synth-state.h")
set(SKRED_VERSION_H "${CMAKE_CURRENT_BINARY_DIR}/skred-version.h")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/skred-version.h.in"
    "${SKRED_VERSION_H}"
    @ONLY
)

# 1.2 The Command: This is the ONLY place that runs the kit_tool for this header
add_kit_generated_file("${SYNTH_STATE_H}" "${SYNTH_STATE_KIT}" ${KIT_TOOL_EXE})
set_property(SOURCE "${SYNTH_STATE_H}" PROPERTY GENERATED TRUE)

# 2. Hand-written (plain) source files for the API library
set(API_PLAIN_SOURCES
    ands.c
    miniaudio.c
    midi.c
    util.c
    miniwav.c
    exp-vfs/miniz.c
    exp-vfs/miniz_tdef.c
    exp-vfs/miniz_tinfl.c
    exp-vfs/miniz_zip.c
    exp-vfs/skred_vfs.c
    skode-event.c
    skode-dict.c
    skqueue.c
    udp.c
)

# 3. List your .kit modules intended ONLY for the library
set(API_KIT_MODULES 
    api
    synth
    skode
    polyphony
    synth-alloc
    seq
)

if(NOT RECORD_FEATURE_INDEX EQUAL -1)
    list(APPEND API_KIT_MODULES recorder)
endif()

if(NOT SCOPE_FEATURE_INDEX EQUAL -1)
    list(APPEND API_PLAIN_SOURCES scope-ipc.c)
endif()

if(NOT KSYNTH_FEATURE_INDEX EQUAL -1)
    list(APPEND API_PLAIN_SOURCES vendor/ksynth/ksynth.c)
endif()

set(API_GEN_SOURCES "")

foreach(MOD ${API_KIT_MODULES})
    # Logic for .c.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit")
        message(STATUS "DEBUG: Module (C) ${MOD} using KIT_OPTS: ${KIT_OPTS}")
        set(OUT_C "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        if(MOD STREQUAL "skode")
          set(SKODE_DOC_INC "${CMAKE_CURRENT_BINARY_DIR}/skode-doc.inc")
          add_custom_command(
              OUTPUT "${OUT_C}" "${SKODE_DOC_INC}"
              COMMAND "${KIT_TOOL_EXE}" ${KIT_OPTS_LIST}
                --input "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit"
                --output "${OUT_C}"
                --doc "${SKODE_DOC_INC}"
              DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" ${KIT_TOOL_EXE} "${SYNTH_STATE_H}"
              VERBATIM
          )
          set_property(SOURCE "${SKODE_DOC_INC}" PROPERTY GENERATED TRUE)
        else()
          add_kit_generated_file("${OUT_C}" "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" ${KIT_TOOL_EXE} "${SYNTH_STATE_H}")
        endif()
        set_property(SOURCE "${OUT_C}" PROPERTY GENERATED TRUE)
        if(MOD STREQUAL "skode")
          set_source_files_properties("${OUT_C}" PROPERTIES COMPILE_FLAGS "-Wno-multichar")
        endif()
        list(APPEND API_GEN_SOURCES "${OUT_C}")
    endif()

    # Logic for .h.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit")
        message(STATUS "DEBUG: Module (H) ${MOD} using KIT_OPTS: ${KIT_OPTS}")
        set(OUT_H "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.h")
        add_kit_generated_file("${OUT_H}" "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit" ${KIT_TOOL_EXE} "${SYNTH_STATE_H}")
        set_property(SOURCE "${OUT_H}" PROPERTY GENERATED TRUE)
        list(APPEND API_GEN_SOURCES "${OUT_H}")
    endif()
endforeach()

# 4. Generate the API Static Library
add_library(api STATIC
  ${API_PLAIN_SOURCES}
  ${API_GEN_SOURCES}
  "${SYNTH_STATE_H}"
)

set_target_properties(api PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(api_shared SHARED
  ${API_PLAIN_SOURCES}
  ${API_GEN_SOURCES}
  "${SYNTH_STATE_H}"
)

set_target_properties(api_shared PROPERTIES
  OUTPUT_NAME api
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# 5. Include Directories for the library
target_include_directories(api PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/exp-vfs")
target_include_directories(api_shared PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(api_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(api_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/exp-vfs")

if(NOT MIDI_FEATURE_INDEX EQUAL -1)
    target_compile_definitions(api PUBLIC SKRED_MIDI_ENABLED=1)
    target_compile_definitions(api_shared PUBLIC SKRED_MIDI_ENABLED=1)
    if(EMSCRIPTEN)
        target_link_libraries(api PUBLIC "-sASYNCIFY")
        target_link_libraries(api_shared PUBLIC "-sASYNCIFY")
    elseif(APPLE)
		    find_library(COREMIDI_FRAMEWORK CoreMIDI REQUIRED)
		    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
		    target_link_libraries(api PUBLIC ${COREMIDI_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
		    target_link_libraries(api_shared PUBLIC ${COREMIDI_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
		elseif(UNIX)
        find_package(ALSA REQUIRED)
        target_include_directories(api PRIVATE ${ALSA_INCLUDE_DIRS})
        target_include_directories(api_shared PRIVATE ${ALSA_INCLUDE_DIRS})
        target_link_libraries(api PUBLIC ${ALSA_LIBRARIES})
        target_link_libraries(api_shared PUBLIC ${ALSA_LIBRARIES})
    endif()
endif()

# 6. Setup the main mini-skred executable module
set(EXE_MOD "mini-skred")
set(EXE_GEN_SOURCES "")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${EXE_MOD}.c.kit")
    message(STATUS "DEBUG: Executable Module (C) ${EXE_MOD} using KIT_OPTS: ${KIT_OPTS}")
    set(OUT_C "${CMAKE_CURRENT_BINARY_DIR}/${EXE_MOD}.c")
    add_kit_generated_file("${OUT_C}" "${CMAKE_CURRENT_SOURCE_DIR}/${EXE_MOD}.c.kit" ${KIT_TOOL_EXE} "${SYNTH_STATE_H}")
    set_property(SOURCE "${OUT_C}" PROPERTY GENERATED TRUE)
    list(APPEND EXE_GEN_SOURCES "${OUT_C}")
endif()

set(ALL_KIT_GENERATED_SOURCES
  ${API_GEN_SOURCES}
  ${EXE_GEN_SOURCES}
  "${SYNTH_STATE_H}"
)

add_custom_target(generated-sources
  DEPENDS ${ALL_KIT_GENERATED_SOURCES}
)

add_custom_target(clean-generated
  COMMAND ${CMAKE_COMMAND} -E remove -f ${ALL_KIT_GENERATED_SOURCES}
  VERBATIM
)

# Final App: Build executable and link to the library
add_executable(mini-skred ${EXE_GEN_SOURCES})
target_link_libraries(mini-skred PRIVATE api)

# 7. Find the library portably
find_library(MATH_LIB m)

if(MATH_LIB)
    # Link to the API library (PUBLIC so mini-skred inherits the math dependency)
    target_link_libraries(api PUBLIC ${MATH_LIB})
    target_link_libraries(api_shared PUBLIC ${MATH_LIB})
    
    # Link to the tool (only if kit.c needs it)
    if(TARGET kit_tool)
        target_link_libraries(kit_tool PRIVATE ${MATH_LIB})
    endif()
endif()

if(WIN32)
    target_link_libraries(api PUBLIC ws2_32 ole32 uuid user32 advapi32 winmm)
    target_link_libraries(api_shared PUBLIC ws2_32 ole32 uuid user32 advapi32 winmm)
endif()

add_custom_target(check_opts
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT FEATURES"
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT_OPTS (Raw): ${KIT_OPTS}"
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT_OPTS_LIST:  ${KIT_OPTS_LIST}"
  VERBATIM
)

# Hook the check_opts to the library so it always runs
add_dependencies(api check_opts)
add_dependencies(api_shared check_opts)

install(TARGETS api api_shared mini-skred
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/api.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/polyphony.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/exp-vfs/skred_vfs.h"
  "${SKRED_VERSION_H}"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/skred"
)

enable_testing()

add_test(
  NAME smoke.build-artifacts
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DPROJECT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -DKIT_TOOL_EXE=${KIT_TOOL_EXE}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SmokeTest.cmake
)

add_executable(ands_parser_tests tests/ands_parser_tests.c)
target_link_libraries(ands_parser_tests PRIVATE api)

add_test(
  NAME ands.parser-contract
  COMMAND ands_parser_tests
)

add_executable(skode_state_tests tests/skode_state_tests.c)
target_link_libraries(skode_state_tests PRIVATE api)
target_compile_definitions(skode_state_tests PRIVATE
  SKRED_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT TRACKS_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_TRACKS=1)
endif()
if(NOT RECORD_FEATURE_INDEX EQUAL -1 OR NOT SCOPE_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_RECORD_SCOPE=1)
endif()
if(NOT PD_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_PD=1)
endif()
if(NOT FM_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_FM=1)
endif()
if(NOT KSYNTH_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_KSYNTH=1)
endif()
if(NOT PD_FEATURE_INDEX EQUAL -1 AND
   NOT FILT_FEATURE_INDEX EQUAL -1 AND
   NOT FADSR_FEATURE_INDEX EQUAL -1 AND
   NOT KSYNTH_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE
    SKRED_TEST_INSPIRED_BANKS=1)
endif()
add_test(
  NAME skode.command-state
  COMMAND skode_state_tests
)
add_test(
  NAME skode.session-archive
  COMMAND skode_state_tests --session-only
)

add_executable(skqueue_tests tests/skqueue_tests.c)
target_link_libraries(skqueue_tests PRIVATE api)

add_test(
  NAME skqueue.concurrent-publication
  COMMAND skqueue_tests
)

add_executable(audio_command_tests tests/audio_command_tests.c)
target_link_libraries(audio_command_tests PRIVATE api)

add_test(
  NAME audio.command-routing
  COMMAND audio_command_tests
)

add_executable(midi_tests tests/midi_tests.c)
target_link_libraries(midi_tests PRIVATE api)

add_test(
  NAME midi.control-event-bridge
  COMMAND midi_tests
)

add_executable(polyphony_tests tests/polyphony_tests.c)
target_link_libraries(polyphony_tests PRIVATE api)
if(NOT TRACKS_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(polyphony_tests PRIVATE SKRED_TEST_TRACKS=1)
endif()

add_test(
  NAME polyphony.groups-pools-and-graphs
  COMMAND polyphony_tests
)

add_executable(ksynth_bridge_tests tests/ksynth_bridge_tests.c)
target_link_libraries(ksynth_bridge_tests PRIVATE api)
if(NOT KSYNTH_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(ksynth_bridge_tests PRIVATE SKRED_TEST_KSYNTH=1)
else()
  target_sources(ksynth_bridge_tests PRIVATE vendor/ksynth/ksynth.c)
endif()

add_test(
  NAME ksynth.array-bridge
  COMMAND ksynth_bridge_tests
)

add_executable(render_equivalence_probe EXCLUDE_FROM_ALL
  tests/render_equivalence_probe.c)
target_link_libraries(render_equivalence_probe PRIVATE api)

add_executable(synth_callback_bench EXCLUDE_FROM_ALL
  tests/synth_callback_bench.c)
target_link_libraries(synth_callback_bench PRIVATE api)
if(NOT PD_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(synth_callback_bench PRIVATE SKRED_BENCH_PD=1)
endif()
if(NOT FM_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(synth_callback_bench PRIVATE SKRED_BENCH_FM=1)
endif()

if(NOT RECORD_FEATURE_INDEX EQUAL -1)
  add_executable(recording_tests tests/recording_tests.c)
  target_link_libraries(recording_tests PRIVATE api)

  add_test(
    NAME recording.track-routing
    COMMAND recording_tests
  )
endif()

if(NOT SCOPE_FEATURE_INDEX EQUAL -1)
  add_executable(scope_reader scope-reader.c)
  target_link_libraries(scope_reader PRIVATE api)

  add_executable(scope_ipc_tests tests/scope_ipc_tests.c)
  target_link_libraries(scope_ipc_tests PRIVATE api)

  add_test(
    NAME scope.shared-memory-publication
    COMMAND scope_ipc_tests
  )
endif()
