cmake_minimum_required(VERSION 3.10)
project(mini-skred)

# 1. Build the tool
add_executable(kit_tool kit.c)

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
    # Logic for .c.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit")
        set(OUT_C "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        add_custom_command(
            OUTPUT "${OUT_C}"
            COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:kit_tool> < "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" > "${OUT_C}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.c.kit" kit_tool
            VERBATIM
        )
        if(MOD STREQUAL "skode")
          set_source_files_properties("${OUT_C}" PROPERTIES COMPILE_FLAGS "-Wno-multichar")
        endif()
        list(APPEND GEN_SOURCES "${OUT_C}")
    endif()

    # Logic for .h.kit
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit")
        set(OUT_H "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.h")
        add_custom_command(
            OUTPUT "${OUT_H}"
            COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:kit_tool> < "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit" > "${OUT_H}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.h.kit" kit_tool
            VERBATIM
        )
    endif()
endforeach()

# 4. Final App: Combine Plain and Generated sources
add_executable(mini-skred ${PLAIN_SOURCES} ${GEN_SOURCES})

# 5. Include Directories
# Look in the source folder for plain .h files
target_include_directories(mini-skred PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# Look in the build folder for generated .h files
target_include_directories(mini-skred PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# 6. Find the library portably
find_library(MATH_LIB m)

if(MATH_LIB)
    # Link to the main app (covers all modules)
    target_link_libraries(mini-skred PRIVATE ${MATH_LIB})
    
    # Link to the tool (only if kit.c needs it)
    target_link_libraries(kit_tool PRIVATE ${MATH_LIB})
endif()
