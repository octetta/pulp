set(required_files
  "${PROJECT_BINARY_DIR}/synth-state.h"
  "${PROJECT_BINARY_DIR}/mini-skred"
)

foreach(path IN LISTS required_files)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Missing expected build artifact: ${path}")
  endif()
endforeach()

file(READ "${PROJECT_BINARY_DIR}/synth-state.h" synth_state_h)
string(FIND "${synth_state_h}" "typedef struct" synth_state_struct_pos)
if(synth_state_struct_pos EQUAL -1)
  message(FATAL_ERROR "Generated synth-state.h does not look valid")
endif()

set(smoke_out "${PROJECT_BINARY_DIR}/smoke_features.txt")
execute_process(
  COMMAND "${KIT_TOOL_EXE}" --input "${PROJECT_SOURCE_DIR}/features.kit" --output "${smoke_out}"
  RESULT_VARIABLE smoke_result
)

if(NOT smoke_result EQUAL 0)
  message(FATAL_ERROR "kit_tool smoke run failed with exit code ${smoke_result}")
endif()

file(READ "${smoke_out}" smoke_text)
string(FIND "${smoke_text}" "PD=1" smoke_pd_pos)
string(FIND "${smoke_text}" "FILT=1" smoke_filt_pos)
if(smoke_pd_pos EQUAL -1 OR smoke_filt_pos EQUAL -1)
  message(FATAL_ERROR "kit_tool smoke output was not generated as expected")
endif()
