cmake_minimum_required(VERSION 3.4.1)

project(dr_detector)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# include
include_directories(${CMAKE_SOURCE_DIR}/include)

# opencv
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )



add_executable(dr_detector
        src/dr_main.cpp
        )

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(dr_detector
		pthread
		${OpenCV_LIBS}
		dl
	)
else()
	target_link_libraries(dr_detector
		dl
		${OpenCV_LIBS}
	)
endif()

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/dr_detcor${CMAKE_SYSTEM_NAME})
install(TARGETS dr_detector DESTINATION ./)
#install(DIRECTORY model DESTINATION ./)
#install(PROGRAMS run_demo.sh DESTINATION ./)
#install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
