set(sources
  ringbuffer.cpp
)

list(APPEND CUSTOM_INCLUDE_DIRECTORIES 
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Build library
message("building library ${ringbufferLibaryName}")
# add_library(${ringbufferLibaryName} STATIC ${sources})
add_executable(${ringbufferLibaryName} ${sources})

target_include_directories(${ringbufferLibaryName}
    PRIVATE
    ${CUSTOM_INCLUDE_DIRECTORIES}
)

set_target_properties(${ringbufferLibaryName}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
)

target_compile_definitions(${ringbufferLibaryName}
    PRIVATE
    ${DEFAULT_COMPILE_DEFINITIONS}
)

target_compile_options(${ringbufferLibaryName}
    PRIVATE
    ${DEFAULT_COMPILE_OPTIONS}
)