cmake_minimum_required (VERSION 3.8)

project ("speechkit-tts-gst-plugin")

# Build shared libraries by default.
if(NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(GLIB_PKG REQUIRED IMPORTED_TARGET glib-2.0)
# Add source to this project's executable.
add_library (gstskttselements SHARED  "gstskttselements.c"  )
#"gstawscredentials.cpp" "gsts3multipartuploader.cpp" "gsts3sink.c" "gsts3uploader.c"

target_link_libraries(gstskttselements PUBLIC PkgConfig::GLIB_PKG ${GSTREAMER_LIBRARIES})