cmake_minimum_required(VERSION 3.10)
project(mini-skred)

# 1. Build the tool
set(KIT_OPTS "" CACHE STRING "Extra flags for the kit tool")
separate_arguments(KIT_OPTS_LIST NATIVE_COMMAND "${KIT_OPTS}")
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

# 1.2 The Command: This is the ONLY place that runs the kit_tool for this header
add_kit_generated_file("${SYNTH_STATE_H}" "${SYNTH_STATE_KIT}" ${KIT_TOOL_EXE})
set_property(SOURCE "${SYNTH_STATE_H}" PROPERTY GENERATED TRUE)

# 2. Hand-written (plain) source files for the API library
set(API_PLAIN_SOURCES
    ands.c
    miniaudio.c
    util.c
    miniwav.c
    skqueue.c
    seq.c
    udp.c
)

# 3. List your .kit modules intended ONLY for the library
set(API_KIT_MODULES 
    api
    synth
    skode
    synth-alloc
)

set(API_GEN_SOURCES "")

foreach(MOD ${API_KIT_MODULES})
    # Logic for .c.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit")
        message(STATUS "DEBUG: Module (C) ${MOD} using KIT_OPTS: ${KIT_OPTS}")
        set(OUT_C "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        add_kit_generated_file("${OUT_C}" "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" ${KIT_TOOL_EXE} "${SYNTH_STATE_H}")
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

# 5. Include Directories for the library
target_include_directories(api PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

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

# Final App: Build executable and link to the library
add_executable(mini-skred ${EXE_GEN_SOURCES})
target_link_libraries(mini-skred PRIVATE api)

# 7. Find the library portably
find_library(MATH_LIB m)

if(MATH_LIB)
    # Link to the API library (PUBLIC so mini-skred inherits the math dependency)
    target_link_libraries(api PUBLIC ${MATH_LIB})
    
    # Link to the tool (only if kit.c needs it)
    if(TARGET kit_tool)
        target_link_libraries(kit_tool PRIVATE ${MATH_LIB})
    endif()
endif()

add_custom_target(check_opts
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT FEATURES"
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT_OPTS (Raw): ${KIT_OPTS}"
  COMMAND ${CMAKE_COMMAND} -E echo "! KIT_OPTS_LIST:  ${KIT_OPTS_LIST}"
  VERBATIM
)

# Hook the check_opts to the library so it always runs
add_dependencies(api check_opts)

enable_testing()

add_test(
  NAME smoke.build-artifacts
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DPROJECT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -DKIT_TOOL_EXE=${KIT_TOOL_EXE}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SmokeTest.cmake
)
