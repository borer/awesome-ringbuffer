set(BENCHMARK_INCLUDE_DIRECTORIES 
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${RINGBUFFER_INCLUDE_PATH}
)

file(GLOB BENCHMARK_SOURCES *.cpp)

function(ringbuffer_benchmark name file)
	add_executable(${name} ${filename})
	target_include_directories(${name} PRIVATE ${BENCHMARK_INCLUDE_DIRECTORIES})
	set_target_properties(${name} PROPERTIES ${DEFAULT_PROJECT_OPTIONS})
	target_compile_options(${name} PRIVATE ${DEFAULT_COMPILE_OPTIONS})
	target_compile_definitions(${name} PRIVATE ${DEFAULT_COMPILE_DEFINITIONS})
	target_link_libraries(${name} LINK_PUBLIC ${RINGBUFFER_LIBRARY_NAME} Threads::Threads)
endfunction()

message("Creating benchmarks targets...")

foreach(filename ${BENCHMARK_SOURCES})
	get_filename_component(name ${filename} NAME_WE)

	ringbuffer_benchmark(${name} ${filename})

	message("${name} : ${filename}")
endforeach(filename)