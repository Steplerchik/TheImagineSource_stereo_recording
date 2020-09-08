project(tcamstereocapture)
 
cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_STANDARD 11)

set( TISCAMERA_DIR /home/stepan/tiscamera) 

message( ${TISCAMERA_DIR} )

if(NOT IS_DIRECTORY "${TISCAMERA_DIR}/examples/cpp/common" )
    message(FATAL_ERROR "Error : TISCAMERA_DIR not set to tiscamera directory. Please correct CMakeLists.txt." )
    return()
endif()

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)


pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
pkg_check_modules(TCAMLIB tcam)


include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${TISCAMERA_DIR}/examples/cpp/common  ${GSTREAMER_INCLUDE_DIRS} ${TCAM_INCLUDE_DIRS}  ${OpenCV_INCLUDE_DIRS} )
add_definitions(${GSTREAMER_CFLAGS_OTHER})  

add_executable(tcamstereocapture main.cpp ${TISCAMERA_DIR}/examples/cpp/common/tcamcamera.cpp )

target_link_libraries(tcamstereocapture ${TCAMLIB_LIBRARIES} ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} )

install(TARGETS tcamstereocapture RUNTIME DESTINATION bin)
 
