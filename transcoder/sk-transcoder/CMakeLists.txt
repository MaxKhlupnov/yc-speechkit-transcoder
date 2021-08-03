# CMakeList.txt : CMake project for sk-transcoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("sk-transcoder")


# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0 gstreamer-aws-1.0)
# Add source to this project's executable.
add_executable (sk-transcoder "sk-discoverer.cpp" "sk-transcoder.cpp" )

target_link_libraries(sk-transcoder PUBLIC PkgConfig::GLIB_PKG ${GSTREAMER_LIBRARIES} )
