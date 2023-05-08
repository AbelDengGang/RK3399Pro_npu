#!/bin/bash

set -e


ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

BUILD_DIR=${ROOT_PWD}/build

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
cmake .. \
    -DCMAKE_SYSTEM_NAME="Linux" \
    -DTARGET_PLATFORM="PC"
make -j4
make install
cd -

#cp run_demo.sh install/rknn_yolov5_demo_Linux/
