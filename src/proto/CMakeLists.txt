set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CPP_PATH ${CMAKE_CURRENT_BINARY_DIR})

# Обработка proto-файлов
file(GLOB PROTO_FILES ${PROTO_PATH}/*.proto)
file(MAKE_DIRECTORY ${CPP_PATH})
foreach(PROTO_FILE ${PROTO_FILES})
	set(COMM ${PROTOC_COMMAND} ${PROTO_FILE})
	get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
	add_custom_command(
		OUTPUT ${CPP_PATH}/${PROTO_NAME}.pb.cc ${CPP_PATH}/${PROTO_NAME}.pb.h
		       ${CPP_PATH}/${PROTO_NAME}.grpc.pb.cc ${CPP_PATH}/${PROTO_NAME}.grpc.pb.h
		COMMAND ${_PROTOBUF_PROTOC}
		ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
		     --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
		     --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
		     -I ${PROTO_PATH} -I "${CMAKE_SOURCE_DIR}/grpc/third_party/protobuf/src"
		     ${PROTO_FILE}
		DEPENDS ${PROTO_FILE}
		COMMENT "Generating code from ${PROTO_FILE}"
	)
	list(APPEND SOURCES
		${CPP_PATH}/${PROTO_NAME}.pb.cc
		${CPP_PATH}/${PROTO_NAME}.pb.h
		${CPP_PATH}/${PROTO_NAME}.grpc.pb.cc
		${CPP_PATH}/${PROTO_NAME}.grpc.pb.h
	)
endforeach()

# Добавление цели
add_library(assistant_proto ${SOURCES})
target_include_directories(assistant_proto PUBLIC ${CPP_PATH})
target_link_libraries(assistant_proto PUBLIC
	${_GRPC_GRPCPP_UNSECURE}
	${_PROTOBUF_LIBPROTOBUF}
)
