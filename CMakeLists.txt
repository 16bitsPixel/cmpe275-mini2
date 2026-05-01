cmake_minimum_required(VERSION 3.20)
project(cmpe275_mini2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_REFLECTION gRPC::grpc++_reflection)

find_program(_PROTOBUF_PROTOC protoc REQUIRED)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

# ---------------------------------------------------------
# Proto helper
# ---------------------------------------------------------

function(add_grpc_proto_library TARGET_NAME PROTO_FILE)
    get_filename_component(PROTO_ABS "${PROTO_FILE}" ABSOLUTE)
    get_filename_component(PROTO_DIR "${PROTO_ABS}" PATH)
    get_filename_component(PROTO_NAME_WE "${PROTO_ABS}" NAME_WE)

    set(PROTO_SRCS "${GENERATED_DIR}/${PROTO_NAME_WE}.pb.cc")
    set(PROTO_HDRS "${GENERATED_DIR}/${PROTO_NAME_WE}.pb.h")
    set(GRPC_SRCS  "${GENERATED_DIR}/${PROTO_NAME_WE}.grpc.pb.cc")
    set(GRPC_HDRS  "${GENERATED_DIR}/${PROTO_NAME_WE}.grpc.pb.h")

    add_custom_command(
        OUTPUT
            "${PROTO_SRCS}" "${PROTO_HDRS}"
            "${GRPC_SRCS}"  "${GRPC_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS
            --grpc_out "${GENERATED_DIR}"
            --cpp_out "${GENERATED_DIR}"
            -I "${PROTO_DIR}"
            --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
            "${PROTO_ABS}"
        DEPENDS "${PROTO_ABS}"
        VERBATIM
    )

    add_library(${TARGET_NAME}
        "${PROTO_SRCS}"
        "${PROTO_HDRS}"
        "${GRPC_SRCS}"
        "${GRPC_HDRS}"
    )

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
    )

    target_include_directories(${TARGET_NAME}
        PUBLIC
            "${GENERATED_DIR}"
    )
endfunction()

# ---------------------------------------------------------
# Proto generation
# ---------------------------------------------------------

add_grpc_proto_library(basecamp_grpc_proto "proto/basecamp.proto")
add_grpc_proto_library(query_grpc_proto    "proto/query.proto")

# ---------------------------------------------------------
# Common include dirs
# ---------------------------------------------------------

set(PROJECT_INCLUDE_DIRS
    "${GENERATED_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ---------------------------------------------------------
# node_server
# ---------------------------------------------------------

add_executable(node_server
    src/common/config.cc
    src/model/OverlayConfig.cpp
    src/transport/node_stub.cc
    src/transport/QueryProtoConverters.cpp
    src/transport/GrpcRemoteQueryClient.cpp
    src/query/LocalQueryEngine.cpp
    src/query/QueryCoordinator.cpp
    src/server/BasecampServiceImpl.cpp
    src/server/QueryServiceImpl.cpp
    src/server/main.cpp
)

target_include_directories(node_server
    PRIVATE
        ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(node_server
    PRIVATE
        basecamp_grpc_proto
        query_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        Threads::Threads
)

# ---------------------------------------------------------
# basecamp_client
# ---------------------------------------------------------

add_executable(basecamp_client
    src/transport/node_stub.cc
    src/client/basecamp_client.cc
)

target_include_directories(basecamp_client
    PRIVATE
        ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(basecamp_client
    PRIVATE
        basecamp_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        Threads::Threads
)

# ---------------------------------------------------------
# query_client
# ---------------------------------------------------------

add_executable(query_client
    src/transport/QueryProtoConverters.cpp
    src/client/query_client.cc
)

target_include_directories(query_client
    PRIVATE
        ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(query_client
    PRIVATE
        query_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        Threads::Threads
)

message(STATUS "gRPC plugin: ${_GRPC_CPP_PLUGIN_EXECUTABLE}")