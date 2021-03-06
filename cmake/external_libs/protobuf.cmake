if (NOT TARGET protobuf::protobuf)
set(protobuf_USE_STATIC_LIBS ON)
set(protobuf_CXXFLAGS "-Wno-maybe-uninitialized -Wno-unused-parameter -fPIC -fstack-protector-all -D_FORTIFY_SOURCE=2 -O2")
set(protobuf_LDFLAGS "-Wl,-z,relro,-z,now,-z,noexecstack")
set(_ge_tmp_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE " -Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE " -Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/protobuf_source/repository/archive/v3.8.0.tar.gz")
    set(MD5 "eba86ae9f07ba5cfbaf8af3bc4e84236")
else()
    set(REQ_URL "https://github.com/protocolbuffers/protobuf/archive/v3.8.0.tar.gz")
    set(MD5 "3d9e32700639618a4d2d342c99d4507a")
endif ()

graphengine_add_pkg(protobuf
        VER 3.8.0
        LIBS protobuf
        EXE protoc
        URL ${REQ_URL}
        MD5 ${MD5}
        CMAKE_PATH ../cmake/
        CMAKE_OPTION -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF)
set(CMAKE_CXX_FLAGS ${_ge_tmp_CMAKE_CXX_FLAGS})
endif()
add_library(graphengine::protobuf ALIAS protobuf::protobuf)
set(PROTOBUF_LIBRARY protobuf::protobuf)
include_directories(${protobuf_INC})
include_directories(${protobuf_DIRPATH}/src)

function(ge_protobuf_generate comp c_var h_var)
    if(NOT ARGN)
        message(SEND_ERROR "Error: ge_protobuf_generate() called without any proto files")
        return()
    endif()

    set(${c_var})
    set(${h_var})

    foreach(file ${ARGN})
        get_filename_component(abs_file ${file} ABSOLUTE)
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_dir ${abs_file} PATH)

        list(APPEND ${c_var} "${CMAKE_BINARY_DIR}/proto/${comp}/proto/${file_name}.pb.cc")
        list(APPEND ${h_var} "${CMAKE_BINARY_DIR}/proto/${comp}/proto/${file_name}.pb.h")

        add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/proto/${comp}/proto/${file_name}.pb.cc"
                "${CMAKE_BINARY_DIR}/proto/${comp}/proto/${file_name}.pb.h"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/proto/${comp}/proto"
                COMMAND protobuf::protoc -I${file_dir} --cpp_out=${CMAKE_BINARY_DIR}/proto/${comp}/proto ${abs_file}
                DEPENDS protobuf::protoc ${abs_file}
                COMMENT "Running C++ protocol buffer compiler on ${file}" VERBATIM )
    endforeach()

    set_source_files_properties(${${c_var}} ${${h_var}} PROPERTIES GENERATED TRUE)
    set(${c_var} ${${c_var}} PARENT_SCOPE)
    set(${h_var} ${${h_var}} PARENT_SCOPE)

endfunction()
