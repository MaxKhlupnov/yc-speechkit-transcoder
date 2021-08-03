# CMakeList.txt : CMake project for sk-transcoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("svc-transcoder")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

find_package(PkgConfig REQUIRED) 

find_package(GStreamer)
if(GStreamer_FOUND)
  message(STATUS "GStreamer FOUND!!")
  include_directories(${GSTREAMER_INCLUDE_DIRS})
endif()

include_directories("/usr/include/gstreamer-1.0/" "/usr/include/glib-2.0/" 
"/usr/include/gstreamer-1.0/gst/pbutils/" "/usr/include/gstreamer-1.0/gst/video" "/usr/include/gstreamer-1.0/gst/audio" "/usr/local/include/gstreamer-1.0/gst/aws")

# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0 gstreamer-pbutils-1.0)

add_subdirectory("amazon-s3-gst-plugin/src")

add_subdirectory("sk-transcoder")
