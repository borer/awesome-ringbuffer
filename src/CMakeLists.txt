message("Building ${RINGBUFFER_LIBRARY_NAME} library")

set(SOURCES
  spsc_queue.cpp
  mpsc_queue.cpp
  spsc_queue_orchestrator.cpp
  queue_wait_strategy.cpp
)

add_library(${RINGBUFFER_LIBRARY_NAME} STATIC ${SOURCES})

target_include_directories(${RINGBUFFER_LIBRARY_NAME}
    PUBLIC 
    ${RINGBUFFER_INCLUDE_PATH}
)

set_target_properties(${RINGBUFFER_LIBRARY_NAME}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
)

target_compile_definitions(${RINGBUFFER_LIBRARY_NAME}
    PUBLIC 
    ${DEFAULT_COMPILE_DEFINITIONS}
)

target_compile_options(${RINGBUFFER_LIBRARY_NAME}
    PUBLIC 
    ${DEFAULT_COMPILE_OPTIONS}
)