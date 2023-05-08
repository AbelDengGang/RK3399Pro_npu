cmake_minimum_required(VERSION 3.4.1)

project(dr_detector)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# include
include_directories(${CMAKE_SOURCE_DIR}/include)

# opencv
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(SRC_LISTS src/dr_main.cpp
			  src/dr_detector.cpp
	)
set(LINK_LIST 
	pthread
	${OpenCV_LIBS}
	dl
)
# 为不同的平台定义不同的配置
# 本地摄像头ID: PC:0 ; toybrock 3399pro 5
if(TARGET_PLATFORM STREQUAL PC)
	message(STATUS "当前目标机器为PC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCAM_ID=0")
elseif(TARGET_PLATFORM STREQUAL TB_3399PRO)
	message(STATUS "当前目标机器为Toybrick 3300pro")

	# rga
	set(RGA_DIR ${CMAKE_SOURCE_DIR}/../3rdparty/rga)
	include_directories(${RGA_DIR}/include)

	# drm
	set(DRM_DIR ${CMAKE_SOURCE_DIR}/../3rdparty/drm)
	include_directories(${DRM_DIR}/include)
	include_directories(${DRM_DIR}/include/libdrm)

	include_directories(${CMAKE_SOURCE_DIR}/include)

	# rknn api
	set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../../../librknn_api)
	include_directories(${RKNN_API_PATH}/include)
	set(RKNN_API_LIB ${RKNN_API_PATH}/Linux/lib64/librknn_api.so)

	#stb
	include_directories(${CMAKE_SOURCE_DIR}/../3rdparty/)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCAM_ID=5 -DUSE_RK_NPU")
	set(SRC_LISTS ${SRC_LISTS} 
		src/rk_npu_detector.cpp
		src/drm_func.c
		src/rga_func.c
		src/postprocess.cc)

	set(LINK_LIST ${LINK_LIST} 
		${RKNN_API_LIB}
		)
else()
	message(STATUS "当前目标机器未知")
endif()
add_executable(dr_detector
		${SRC_LISTS}
        )

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(dr_detector
		${LINK_LIST}
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
install(DIRECTORY model DESTINATION ./)
#install(PROGRAMS run_demo.sh DESTINATION ./)
install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
