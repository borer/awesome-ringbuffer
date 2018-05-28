message("building ${ringbufferLibaryName} library ")

list(APPEND CUSTOM_INCLUDE_DIRECTORIES 
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(SOURCES
  spsc_queue.cpp
)

add_library(${ringbufferLibaryName} STATIC ${SOURCES})

target_include_directories(${ringbufferLibaryName}
    PUBLIC 
    ${CUSTOM_INCLUDE_DIRECTORIES}
)

set_target_properties(${ringbufferLibaryName}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
)

target_compile_definitions(${ringbufferLibaryName}
    PUBLIC 
    ${DEFAULT_COMPILE_DEFINITIONS}
)

target_compile_options(${ringbufferLibaryName}
    PUBLIC 
    ${DEFAULT_COMPILE_OPTIONS}
)