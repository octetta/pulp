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
if(NOT SCOPE_FEATURE_INDEX EQUAL -1 AND WIN32)
    message(FATAL_ERROR "SCOPE=1 currently requires POSIX shared memory")
endif()





set(SKRED_VERSION_H "${CMAKE_CURRENT_BINARY_DIR}/skred-version.h")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/skred-version.h.in"
    "${SKRED_VERSION_H}"
    @ONLY
)

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

# 3. List modules
    list(APPEND API_PLAIN_SOURCES
        api.c
        synth.c
        skode.c
        polyphony.c
        synth-alloc.c
        seq.c
        recorder.c
    )
    
if(NOT SCOPE_FEATURE_INDEX EQUAL -1)
    list(APPEND API_PLAIN_SOURCES scope-ipc.c)
endif()

if(NOT KSYNTH_FEATURE_INDEX EQUAL -1)
    list(APPEND API_PLAIN_SOURCES vendor/ksynth/ksynth.c)
endif()




# 4. Generate the API Static Library
add_library(api STATIC
  ${API_PLAIN_SOURCES}
  
  
)

set_target_properties(api PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(api_shared SHARED
  ${API_PLAIN_SOURCES}
  
  
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

    target_compile_definitions(api PUBLIC SKRED_MIDI_ENABLED=1)
    target_compile_definitions(api_shared PUBLIC SKRED_MIDI_ENABLED=1)

if(NOT SCOPE_FEATURE_INDEX EQUAL -1)
    target_compile_definitions(api PUBLIC SCOPE=1)
    target_compile_definitions(api_shared PUBLIC SCOPE=1)
endif()
if(NOT UDP_FEATURE_INDEX EQUAL -1)
    target_compile_definitions(api PUBLIC UDP=1)
    target_compile_definitions(api_shared PUBLIC UDP=1)
endif()

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

# 6. Setup the main mini-skred executable module
add_executable(mini-skred mini-skred.c)
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




install(TARGETS api api_shared mini-skred
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SKODE_DOCS.md" "${CMAKE_CURRENT_BINARY_DIR}/skode-doc.inc"
  COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/gen_docs.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/skode-dict.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/skode.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/skode.h"
          "${CMAKE_CURRENT_BINARY_DIR}/SKODE_DOCS.md"
          "${CMAKE_CURRENT_BINARY_DIR}/skode-doc.inc"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/skode-dict.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/skode.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/skode.h"
          "${CMAKE_CURRENT_SOURCE_DIR}/gen_docs.py"
  COMMENT "Generating SKODE_DOCS.md and skode-doc.inc..."
)
add_custom_target(generate_docs ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/SKODE_DOCS.md")
add_dependencies(api generate_docs)

install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/api.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/polyphony.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/synth-config.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/synth-types.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/portable_atomic.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/skode.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/ands.h"
  "${CMAKE_CURRENT_BINARY_DIR}/seq.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/exp-vfs/skred_vfs.h"
  "${SKRED_VERSION_H}"
  "${CMAKE_CURRENT_BINARY_DIR}/SKODE_DOCS.md"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/skred"
)

enable_testing()



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
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_TRACKS=1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_RECORD_SCOPE=1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_PD=1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_FM=1)
if(NOT KSYNTH_FEATURE_INDEX EQUAL -1)
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_KSYNTH=1)
endif()
  target_compile_definitions(skode_state_tests PRIVATE SKRED_TEST_MIDI=1)
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
  target_compile_definitions(polyphony_tests PRIVATE SKRED_TEST_TRACKS=1)

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
  target_compile_definitions(synth_callback_bench PRIVATE SKRED_BENCH_PD=1)
  target_compile_definitions(synth_callback_bench PRIVATE SKRED_BENCH_FM=1)

  add_executable(recording_tests tests/recording_tests.c)
  target_link_libraries(recording_tests PRIVATE api)

  add_test(
    NAME recording.track-routing
    COMMAND recording_tests
  )

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
set_source_files_properties(skode.c PROPERTIES COMPILE_FLAGS "-Wno-multichar")
