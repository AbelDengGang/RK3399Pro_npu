#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#include <opencv2/opencv.hpp>

#include "dr_detector.h"
#ifdef USE_RK_NPU
#include "rk_npu_detector.h"
#endif 
// 检测器列表，越前面的检测器速度越快，优先级越高。每当有新的图片进来，要按照优先级来把图片分配给检测器。
// 当速度最快的检测器忙的时候，才尝试使用低速检测器。
// 最低速的检测器不做任何处理，只是记录图片，用于利用前面检测器的结果进行tracking
// 如果所有的检测器都满了，那就丢弃

#ifdef USE_RK_NPU
RKNpuDetector rknnNpuDetector(2,"rknpu_detector");
#endif    

DrDetector drDetector(3,"detector0");
DrDetector *detectors[]={
#ifdef USE_RK_NPU
    &rknnNpuDetector,
#endif    
    &drDetector,
};
bool showOrgPicture = true;
static void localCamThreadFun(){
    cv::VideoCapture camera;
    camera.open(CAM_ID); // 打开摄像头, 默认摄像头cameraIndex=0
    if (!camera.isOpened())
    {
        LOG_ERR << "Couldn't open camera." << endl;
        return;
    }
    std::string window_name = "Local camera";
    // 设置参数
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1000);     // 宽度
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1000);    // 高度
    camera.set(cv::CAP_PROP_FPS, 30);               // 帧率

    // 查询参数
    double frameWidth = camera.get(cv::CAP_PROP_FRAME_WIDTH);
    double frameHeight = camera.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = camera.get(cv::CAP_PROP_FPS);
    LOG_INFO << "Input picture: width:" << frameWidth << "," << "  height:"<< frameHeight << " fps:" << fps << endl;
    int frameID = 0;

#ifdef TEST_PIC
    LOG_INFO << "Test pic is " << TEST_PIC << endl;
    cv::Mat imageFromFile = cv::imread(TEST_PIC, cv::IMREAD_COLOR);
    if (imageFromFile.empty()) {
        LOG_ERR << "Failed to load image" << std::endl;
        return ;
    }
#endif
    while(1){
        Frame orgFrame;
#ifdef TEST_PIC
        orgFrame.picture = imageFromFile.clone();
#else        
        camera >> orgFrame.picture;
#endif 
#ifdef UINT8_PIC
        if ( orgFrame.picture.depth() != CV_8U){
            LOG_INFO << "picture data format is " << orgFrame.picture.depth() << endl;
            orgFrame.picture.convertTo(orgFrame.picture,CV_8U,255.0);
        }
#endif        
#ifdef BGR2RGB
        {
#ifdef DETAIL_LOG
            struct timeval _start,_end;
            gettimeofday(&_start,NULL);

#endif
        cv::cvtColor(orgFrame.picture, orgFrame.picture, cv::COLOR_BGR2RGB);
#ifdef DETAIL_LOG
            gettimeofday(&_end,NULL);
            LOG_INFO << "BGA->RGB consumes:" << (_end.tv_sec*1000*1000+_end.tv_usec) - (_start.tv_sec*1000*1000+_start.tv_usec) << "us" << endl;
#endif
        }
#endif 


        orgFrame.frameID = frameID;
        frameID ++;
        if (showOrgPicture){
            cv::imshow(window_name, orgFrame.picture);
            if (cv::waitKey(33) == 27) break; // ESC 键退出
        }
        // TODO 
        // 图片预处理，把图片进行切割缩放以后，发送给检测器
        // 检测器就不再进行缩放处理了，而是全力进行检测，并把检测后的结果拼接回原图
        // 检测器线程本身就是耗时比较大的，预处理耗时一定比检测器耗时短，所以在读取线程中进行预处理，
        // 这样可以避免读取线程白白浪费时间等待检测器完成检测
        
        for(int i = 0;i<sizeof(detectors)/sizeof(detectors[0]);++i){
            if(detectors[i]->tryAddFrame(orgFrame)){
                break;
            }else{
            }
        }
    }

    // 释放
    camera.release();
    if (showOrgPicture){
        cv::destroyWindow(window_name);
    }
}
int main(int argc, char **argv)
{
    std::thread threadLocalCam(localCamThreadFun); //调用摄像头

    threadLocalCam.join();
   return 0;
}