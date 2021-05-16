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

include_directories("/usr/include/gstreamer-1.0/" "/usr/include/glib-2.0/")

# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0)

add_subdirectory("sk-transcoder")

#add_custom_target(amazon-s3-gst-plugin
#  WORKING_DIRECTORY
#    ${CMAKE_SOURCE_DIR}/amazon-s3-gst-plugin
#  COMMENT
#    "Amazon S3 Gst Plugin"
#  VERBATIM
#  )

add_custom_command(
        OUTPUT  ${CMAKE_SOURCE_DIR}/amazon-s3-gst-plugin/build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/amazon-s3-gst-plugin
        COMMAND  meson   ${CMAKE_SOURCE_DIR}/amazon-s3-gst-plugin/build
        VERBATIM
)

