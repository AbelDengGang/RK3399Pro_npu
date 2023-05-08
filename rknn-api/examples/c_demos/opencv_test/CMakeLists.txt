cmake_minimum_required(VERSION 2.8)
project( display_image )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable( display_image display_image.cpp )
target_link_libraries( display_image ${OpenCV_LIBS} )


add_executable( cam_test cam_test.cpp )
target_link_libraries( cam_test ${OpenCV_LIBS} )

add_executable( data_format data_format.cpp )
target_link_libraries( data_format ${OpenCV_LIBS} )
