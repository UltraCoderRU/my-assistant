add_subdirectory(proto)

set(SOURCES
	audio_input_alsa.cc
	audio_output_alsa.cc
	base64_encode.cc
	run_assistant_audio.cc
)

add_executable(run_assistant_audio ${SOURCES})
target_include_directories(run_assistant_audio PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/grpc" "${CMAKE_SOURCE_DIR}/grpc/third-party/google")
target_link_libraries(run_assistant_audio
	${ALSA_LIBRARIES}
	assistant_proto
	grpc++
	grpc++_reflection
)

set(SOURCES
	base64_encode.cc
	run_assistant_text.cc
)

add_executable(run_assistant_text ${SOURCES})
target_include_directories(run_assistant_text PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/grpc" "${CMAKE_SOURCE_DIR}/grpc/third-party/google")
target_link_libraries(run_assistant_text
	assistant_proto
	grpc++
	grpc++_reflection
)
