# CMakeList.txt : CMake project for sk-transcoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("sk-transcoder")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

find_package(PkgConfig REQUIRED) 

find_package(GStreamer)
if(GStreamer_FOUND)
  message(STATUS "GStreamer FOUND!!")
  include_directories(${GSTREAMER_INCLUDE_DIRS})
endif()

include_directories("/usr/include/gstreamer-1.0/" "/usr/include/glib-2.0/")

# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0)


# Add source to this project's executable.
add_executable (sk-transcoder "sk-transcoder.cpp" "sk-transcoder.h")

target_link_libraries(sk-transcoder PUBLIC PkgConfig::GLIB_PKG ${GSTREAMER_LIBRARIES})
