cmake_minimum_required(VERSION 3.10)
project(mini-skred)

# 1. Build the tool
set(KIT_OPTS "" CACHE STRING "Extra flags for the kit tool")
separate_arguments(KIT_OPTS_LIST NATIVE_COMMAND "${KIT_OPTS}")
add_executable(kit_tool kit.c)

# 1.1 Define the absolute paths for the generator
set(SYNTH_STATE_KIT "${CMAKE_CURRENT_SOURCE_DIR}/synth-state.h.kit")
set(SYNTH_STATE_H   "${CMAKE_CURRENT_BINARY_DIR}/synth-state.h")

# 1.2 The Command: This is the ONLY place that runs the kit_tool for this header
add_custom_command(
    OUTPUT "${SYNTH_STATE_H}"
    COMMAND $<TARGET_FILE:kit_tool> ${KIT_OPTS_LIST} < "${SYNTH_STATE_KIT}" > "${SYNTH_STATE_H}"
    DEPENDS "${SYNTH_STATE_KIT}" kit_tool
    COMMENT "Generating global header: synth-state.h"
    VERBATIM
)

# 2. Hand-written (plain) source files
set(PLAIN_SOURCES
    ands.c
    miniaudio.c
    util.c
    miniwav.c
    skqueue.c
    seq.c
    # Add any other .c files you wrote by hand here
)

# 3. List your .kit modules
set(KIT_MODULES 
    mini-skred
    synth
    skode
    synth-alloc
)

set(GEN_SOURCES "")

foreach(MOD ${KIT_MODULES})
    # message(STATUS "DEBUG: Module ${MOD} using KIT_OPTS: ${KIT_OPTS}")
    # Logic for .c.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit")
        message(STATUS "DEBUG: Module (C) ${MOD} using KIT_OPTS: ${KIT_OPTS}")
        set(OUT_C "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        add_custom_command(
            OUTPUT "${OUT_C}"
            COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:kit_tool> ${KIT_OPTS_LIST} < "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" > "${OUT_C}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" kit_tool "${SYNTH_STATE_H}"
            VERBATIM
        )
        if(MOD STREQUAL "skode")
          set_source_files_properties("${OUT_C}" PROPERTIES COMPILE_FLAGS "-Wno-multichar")
        endif()
        list(APPEND GEN_SOURCES "${OUT_C}")
    endif()

    # Logic for .h.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit")
        message(STATUS "DEBUG: Module (H) ${MOD} using KIT_OPTS: ${KIT_OPTS}")
        set(OUT_H "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.h")
        add_custom_command(
            OUTPUT "${OUT_H}"
            COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:kit_tool> ${KIT_OPTS_LIST} < "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit" > "${OUT_H}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit" kit_tool "${SYNTH_STATE_H}"
            VERBATIM
        )
        list(APPEND GEN_SOURCES "${OUT_H}")
    endif()
endforeach()

# 4. Final App: Combine Plain and Generated sources
add_executable(mini-skred
  ${PLAIN_SOURCES}
  ${GEN_SOURCES}
  "${SYNTH_STATE_H}"
)

# 5. Include Directories
target_include_directories(mini-skred PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(mini-skred PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# 6. Find the library portably
find_library(MATH_LIB m)

if(MATH_LIB)
    # Link to the main app (covers all modules)
    target_link_libraries(mini-skred PRIVATE ${MATH_LIB})
    
    # Link to the tool (only if kit.c needs it)
    target_link_libraries(kit_tool PRIVATE ${MATH_LIB})
endif()
