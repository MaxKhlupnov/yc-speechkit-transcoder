# CMakeList.txt : CMake project for sk-transcoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("sk-transcoder")


# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0 gstreamer-aws-1.0)


########################################################################
#   Protobuf
########################################################################
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)


include_directories(${Protobuf_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(Protobuf_IMPORT_DIRS ${Protobuf_IMPORT_DIRS} ${CMAKE_SOURCE_DIR}/yandex-cloud-api  ${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/)

set(Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR}    CACHE INTERNAL "Path to generated protobuf files.")

protobuf_generate_cpp(stt_service_proto_src stt_service_proto_hdrs ${CMAKE_SOURCE_DIR}/yandex-cloud-api/yandex/cloud/ai/stt/v2/stt_service.proto)
#protobuf_generate_cpp(annotation_src annotation_hdrs ${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/google/api/annotations.proto)


# Add source to this project's executable.
add_executable (sk-transcoder "sk-discoverer.cpp" "sk-transcoder.cpp"  ${stt_service_proto_src} ${stt_service_proto_hdrs})

target_link_libraries(sk-transcoder PUBLIC PkgConfig::GLIB_PKG ${GSTREAMER_LIBRARIES})

message("   --> PROTOBUF LIB: ${PROTOBUF_LIBRARIES}")
message("   --> PROTOBUF INCLUDE: ${PROTOBUF_INCLUDE_DIRS}")
message("   --> PROTOBUF VERSION: ${Protobuf_VERSION}")
message("   --> PROTOBUF Found: ${Protobuf_FOUND}")
# DEBUG PRINT
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()