mkdir -p build
cd build
export VERBOSE=1
cmake 	-DCMAKE_TOOLCHAIN_FILE=../TB3399Pro.cmake \
	-DCMAKE_INSTALL_PREFIX=~/workspace/RK3399Pro_npu/rknn-api/examples/c_demos/3rdparty/opencv \
	-DBUILD_opencv_world=OFF ..
make
make install

#	-DWITH_WEBP=OFF \
