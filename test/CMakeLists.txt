list(APPEND CUSTOM_INCLUDE_DIRECTORIES 
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "../src/"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

file(GLOB TEST_SOURCES *.cpp)

enable_testing()

foreach(filename ${TEST_SOURCES})
	get_filename_component(testname ${filename} NAME_WE)

	add_executable(${testname} ${filename})

	message("include dirs for ${filename} : ${CUSTOM_INCLUDE_DIRECTORIES}")

	target_include_directories(${testname}
	    PRIVATE
	    ${CUSTOM_INCLUDE_DIRECTORIES}
	)

    set_target_properties(${testname}
        PROPERTIES
        ${DEFAULT_PROJECT_OPTIONS}
    )
    
    target_compile_options(${testname}
        PRIVATE
        ${DEFAULT_COMPILE_OPTIONS}
    )
    
    target_compile_definitions(${testname}
    	PRIVATE
    	${DEFAULT_COMPILE_DEFINITIONS}
	)
	
	target_link_libraries(${testname} 
		LINK_PUBLIC 
		${ringbufferLibaryName}
		Threads::Threads
	)
	
	add_test(${testname} ${testname})

	message("found test ${filename} : ${testname}")
endforeach(filename)