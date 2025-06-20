# ---------------------------  File: cmake/protobuf-generate.cmake -------------
# Very small fallback for protobuf_generate(LANGUAGE cpp …)
#
# 仅支持:
#   protobuf_generate(
#       LANGUAGE cpp
#       TARGET   
#       IMPORT_DIRS 
#       PROTOS  
#   )

function(protobuf_generate)
    # ---- 解析参数 ----
    cmake_parse_arguments(PB "" "LANGUAGE;TARGET" "IMPORT_DIRS;PROTOS" ${ARGN})

    if(NOT PB_LANGUAGE STREQUAL "cpp")
        message(FATAL_ERROR "fallback protobuf_generate() only supports LANGUAGE cpp")
    endif()

    if(NOT PB_TARGET)
        message(FATAL_ERROR "protobuf_generate(): TARGET is required")
    endif()

    # ---- 组装 proto-compiler 参数 ----
    set(_import_args "")
    foreach(dir IN LISTS PB_IMPORT_DIRS)
        list(APPEND _import_args "-I" "${dir}")
    endforeach()

    set(_srcs "")
    set(_hdrs "")

    foreach(proto IN LISTS PB_PROTOS)
        get_filename_component(_abs "${proto}" ABSOLUTE)
        get_filename_component(_name "${proto}" NAME_WE)
        set(_src "${CMAKE_CURRENT_BINARY_DIR}/${_name}.pb.cc")
        set(_hdr "${CMAKE_CURRENT_BINARY_DIR}/${_name}.pb.h")

        add_custom_command(
            OUTPUT  ${_src} ${_hdr}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                    ${_import_args}
                    --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                    "${_abs}"
            DEPENDS ${_abs}
            COMMENT "Generating C++ protobuf: ${proto}"
            VERBATIM
        )
        list(APPEND _srcs ${_src})
        list(APPEND _hdrs ${_hdr})
    endforeach()

    # ---- 建目标 ----
    add_library(${PB_TARGET} ${_srcs} ${_hdrs})
    target_include_directories(${PB_TARGET} PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
    )
endfunction() 