message("building ${ringbufferLibaryName} library ")

set(SOURCES
  ringbuffer.cpp
)

foreach(filename ${SOURCES})
	
	if("${filename}" MATCHES "main.c")

	else("${filename}" MATCHES "main.c")
		list(APPEND OBJECTS ${filename})
	endif("${filename}" MATCHES "main.c")

endforeach(filename ${SOURCES})

add_library(${ringbufferLibaryName} STATIC ${SOURCES})

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