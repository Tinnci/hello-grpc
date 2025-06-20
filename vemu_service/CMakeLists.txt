cmake_minimum_required(VERSION 3.15)
project(vemu_service CXX C)

# 使用共享版 protobuf
set(Protobuf_USE_STATIC_LIBS OFF)

# --------------------------- 全局设置 ---------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------- Protobuf ---------------------------
find_package(Protobuf REQUIRED)        # 会导出 protobuf::{protobuf|protoc}

# 额外：通过 pkg-config 获取完整链接参数（包含 Abseil、utf8_range 等依赖）
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_PC REQUIRED protobuf)
pkg_check_modules(GRPC_PC REQUIRED grpc++)

# 查找 PThread
find_package(Threads REQUIRED)

# --------------------------- 先尝试系统 gRPC --------------------
find_package(gRPC CONFIG QUIET)

if(NOT gRPC_FOUND)
    message(STATUS "system gRPC not found, fallback to FetchContent")
    include(FetchContent)

    # 禁用 install() 规则，减少导出相关报错
    set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
    set(absl_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
      grpc
      GIT_REPOSITORY https://github.com/grpc/grpc.git
      GIT_TAG        v1.63.0          # 固定版本，CI 不漂移
    )
    set(gRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(grpc)

    # 引入 gRPC 提供的 proto 生成宏
    include(${grpc_SOURCE_DIR}/cmake/protobuf.cmake)
else()
    message(STATUS "Using system gRPC")
endif()

# --------------------------- 生成 proto --------------------------
set(PROTO_FILE ${CMAKE_SOURCE_DIR}/../api/v1/vemu_service.proto)
set(PROTO_IMPORT_DIRS
    ${Protobuf_INCLUDE_DIRS}           # google/protobuf/...
    ${CMAKE_SOURCE_DIR}/../api/       # 指向 api/ 目录
)

# 生成 Protobuf C++ 代码
set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

# 若当前环境未找到 grpc_generate_cpp\(\)，提供一个极简 fallback
if(NOT COMMAND grpc_generate_cpp)
    message(WARNING "grpc_generate_cpp() NOT found; using local fallback implementation")
    function(grpc_generate_cpp OUT_SRCS OUT_HDRS)
        set(_out_srcs "")
        set(_out_hdrs "")
        list(REMOVE_AT ARGV 0 1)
        set(_protos ${ARGV})

        # 从全局变量 PROTO_IMPORT_DIRS 构造 include 路径
        set(_import_args "")
        foreach(dir IN LISTS PROTO_IMPORT_DIRS)
            list(APPEND _import_args "-I" "${dir}")
        endforeach()

        # 找到 grpc_cpp_plugin
        find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin)
        if(NOT _GRPC_CPP_PLUGIN)
            message(FATAL_ERROR "grpc_cpp_plugin not found; please install protobuf-compiler-grpc or grpc-tools")
        endif()

        foreach(proto IN LISTS _protos)
            get_filename_component(_abs "${proto}" ABSOLUTE)
            get_filename_component(_name "${proto}" NAME_WE)
            # 相对路径，如 v1/vemu_service.proto
            file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}/../api" "${_abs}")

            # protoc 会在 --grpc_out 目录下保留相对目录结构
            # 先去掉 .proto 后缀，再拼接生成文件名
            string(REPLACE ".proto" "" _rel_noext "${_rel}")
            set(_src "${CMAKE_CURRENT_BINARY_DIR}/${_rel_noext}.grpc.pb.cc")
            set(_hdr "${CMAKE_CURRENT_BINARY_DIR}/${_rel_noext}.grpc.pb.h")
            add_custom_command(
                OUTPUT ${_src} ${_hdr}
                COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                        --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN}
                        ${_import_args}
                        "${_rel}"
                DEPENDS ${_abs}
                COMMENT "[grpc] Generating stubs for ${proto}"
                VERBATIM)
            list(APPEND _out_srcs ${_src})
            list(APPEND _out_hdrs ${_hdr})
        endforeach()
        set(${OUT_SRCS} ${_out_srcs} PARENT_SCOPE)
        set(${OUT_HDRS} ${_out_hdrs} PARENT_SCOPE)
    endfunction()
endif()

# 生成 gRPC stub 代码
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILE})

# --------------------------- 打包为静态库 --------------------------
add_library(vemu_proto STATIC
    ${PROTO_SRCS} ${GRPC_SRCS}
)

target_include_directories(vemu_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(vemu_proto
  PUBLIC
    ${GRPC_PC_LIBRARIES}
    ${PROTOBUF_PC_LIBRARIES}
)

# ----------- 处理 include 路径不一致的问题（临时方案） -------------
# gRPC 插件生成的 .grpc.pb.* 会包含 "v1/vemu_service.pb.h" 路径，而
# protobuf_generate_cpp() 生成的 .pb.* 默认放在顶层构建目录。
# 这里简单地把头文件复制一份到 v1/ 目录，避免编译报错。
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/v1/vemu_service.pb.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/v1
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/vemu_service.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/v1/vemu_service.pb.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vemu_service.pb.h
    COMMENT "Copying vemu_service.pb.h into v1/ subdir for gRPC stubs"
)

add_custom_target(proto_header_fix ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/v1/vemu_service.pb.h)

add_dependencies(vemu_proto proto_header_fix)

# --------------------------- 服务可执行 --------------------------
file(GLOB SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main_original.cpp")

add_executable(vemu_service ${SRC_FILES})
target_include_directories(vemu_service PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR}        # 让生成的 *.pb.h 被找到
    ${CMAKE_CURRENT_BINARY_DIR}/v1     # 让 gRPC stub 头文件被找到
)
target_link_libraries(vemu_service PRIVATE
    vemu_proto
    Threads::Threads
    ${GRPC_PC_LIBRARIES}
    ${PROTOBUF_PC_LIBRARIES}
)

# --------------------------- 编译选项 ----------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(vemu_service PRIVATE -O3 -DNDEBUG)
else()
    target_compile_options(vemu_service PRIVATE -O0 -g)
endif()
