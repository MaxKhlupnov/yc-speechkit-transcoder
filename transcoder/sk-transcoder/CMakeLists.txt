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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB protobuf_files 
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/yandex/cloud/ai/stt/v2/*.proto
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/yandex/cloud/operation/*.proto
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/yandex/cloud/api/*.proto
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/google/api/*.proto
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/google/rpc/*.proto
			${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/google/type/*.proto
			)

# compile each file in turn, and then generate the pb.cc And. pb.h move back to the original directory
FOREACH(proto ${protobuf_files})
		
		FILE(TO_NATIVE_PATH ${proto} proto_native)
		
		EXECUTE_PROCESS(
				COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} -I "${CMAKE_SOURCE_DIR}/yandex-cloud-api/third_party/googleapis/" -I "${CMAKE_SOURCE_DIR}/yandex-cloud-api/" --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} "${proto_native}"
				RESULT_VARIABLE rv)

		 IF(${rv})
			MESSAGE("Generation of data model returned ${rv} for proto ${proto_native}")
		 ENDIF()

ENDFOREACH()

# Add source to this project's executable.
add_executable (sk-transcoder "sk-discoverer.cpp" "sk-transcoder.cpp")

target_link_libraries(sk-transcoder PUBLIC PkgConfig::GLIB_PKG ${GSTREAMER_LIBRARIES})

# DEBUG PRINT
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()