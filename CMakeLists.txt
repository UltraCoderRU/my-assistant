cmake_minimum_required(VERSION 2.8)

project(MyAssistant)

set(CMAKE_CXX_STANDARD 14)

# Configuration options
option(USE_SYSTEM_SSL "Build with system SSL library" OFF)

# Configure gRPC
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "")
if (USE_SYSTEM_SSL)
	set(gRPC_SSL_PROVIDER "package" CACHE STRING "Provider of ssl library" FORCE)
else()
	set(gRPC_SSL_PROVIDER "module" CACHE STRING "Provider of ssl library" FORCE)
endif()
add_subdirectory(grpc EXCLUDE_FROM_ALL)

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

# Find ALSA libraries
find_package(ALSA REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Process application sources
add_subdirectory(src)
