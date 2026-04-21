set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

cmake_minimum_required(VERSION 3.20)
project(cmpe275_mini2_basecamp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_REFLECTION gRPC::grpc++_reflection)

if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# ---------------------------------------------------------
# Proto generation
# ---------------------------------------------------------

get_filename_component(BASECAMP_PROTO "proto/basecamp.proto" ABSOLUTE)
get_filename_component(BASECAMP_PROTO_PATH "${BASECAMP_PROTO}" PATH)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(BASECAMP_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/generated/basecamp.pb.cc")
set(BASECAMP_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/generated/basecamp.pb.h")
set(BASECAMP_GRPC_SRCS  "${CMAKE_CURRENT_BINARY_DIR}/generated/basecamp.grpc.pb.cc")
set(BASECAMP_GRPC_HDRS  "${CMAKE_CURRENT_BINARY_DIR}/generated/basecamp.grpc.pb.h")

add_custom_command(
    OUTPUT "${BASECAMP_PROTO_SRCS}" "${BASECAMP_PROTO_HDRS}"
           "${BASECAMP_GRPC_SRCS}"  "${BASECAMP_GRPC_HDRS}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS
        --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
        -I "${BASECAMP_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${BASECAMP_PROTO}"
    DEPENDS "${BASECAMP_PROTO}"
)

add_library(basecamp_grpc_proto
    ${BASECAMP_PROTO_SRCS}
    ${BASECAMP_PROTO_HDRS}
    ${BASECAMP_GRPC_SRCS}
    ${BASECAMP_GRPC_HDRS}
)

target_link_libraries(basecamp_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_include_directories(basecamp_grpc_proto PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# ---------------------------------------------------------
# Includes
# ---------------------------------------------------------

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ---------------------------------------------------------
# basecamp_server
# ---------------------------------------------------------

add_executable(basecamp_server
    src/common/config.cc
    src/client/node_stub.cc
    src/server/node_service.cc
    src/server/main.cc
)

target_link_libraries(basecamp_server
    basecamp_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    Threads::Threads
)

# ---------------------------------------------------------
# basecamp_client
# ---------------------------------------------------------

add_executable(basecamp_client
    src/client/node_stub.cc
    src/client/basecamp_client.cc
)

target_link_libraries(basecamp_client
    basecamp_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    Threads::Threads
)