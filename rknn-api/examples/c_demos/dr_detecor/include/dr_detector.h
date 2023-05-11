#ifndef __DR_DETECTOR_H__
#define __DR_DETECTOR_H__
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <semaphore.h>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <sys/time.h>
#include <execinfo.h>

#define LOG_ERR std::cerr
#define LOG_INFO std::cout
using namespace std;

extern void printStackTrace();

class FrameSlice{
public:
    int col_index; // 切片在原图哪一列
    int rol_index; // 切片在原图的哪一行
    int x_start;   // 在原图的x开始位置
    int y_start;   // 在原图的y开始位置
    int width_org; // 在原图中的宽度
    int height_org; // 在原图中的高度
    cv::Mat picture;// 切片图像
};

class Frame{
    void clear(void);
public:
    Frame():sliceCnt(0),slices(NULL),resizedPicture(NULL){};
    ~Frame(){
        clear();
    }
    int frameID;
    cv::Mat picture;
    void copyFrom(Frame &frame);
    int sliceCnt;
    FrameSlice * slices;
    cv::Mat * resizedPicture;
};

class DrDetector{
private:
    pthread_mutex_t mutexFrame;     // 缓冲区互斥锁
    int queueSize;
    int validFrameCnt;
    int queueHead;
    int queueTail;
    //Todo: 暂时先用Frame数组来维护队列，每次入队和出队的时候使用深拷贝，后续要改成Frame指针数组，用移动指针的方法来减少内存拷贝的动作
    Frame * frameQueue;        

    std::thread * pWorkThread;
    int timeout;

    sem_t full_sem;                         // 缓冲区计数信号量, 为0则处理线程阻塞
    bool stopThread;
    bool getHeadFrame(Frame & outFrame);    // 获取队列中的第一个Frame用作处理 
public:
    std::string detectorName;
    int model_input_height;                 // 模型的输入高度
    int model_input_width;                  // 模型的输入宽度
    int model_input_channel;

    bool resize_to_fit_model;               // 使用缩放的方法还是切割的方法。
    void* workThd();

    void virtual resize_to_model(Frame &frame);  // 缩放到model的输入尺寸，默认使用opencv的resize

    DrDetector(int frameQueueSize,const char * name);

    int getQueueSize(void){
        return queueSize;
    }

    virtual ~DrDetector(){
        printStackTrace();
        stopThread = true;
        LOG_INFO << detectorName << ": waiting for work thread quit" << endl;
        sem_post(&full_sem); // 唤醒工作线程，让他退出

        pWorkThread->join(); // 等待工作线程退出
        LOG_INFO << detectorName << ": work thread quited" << endl;
        if(frameQueue){
            delete []frameQueue;
        }
        sem_destroy(&full_sem);
        pthread_mutex_destroy(&mutexFrame);

    }
    // if queue has free slot, copy the frame to queue, and return true
    // otherwise return false if queue is full.
    bool tryAddFrame(Frame &frame);

    virtual void process(Frame & frame){
        
    }
};

#endif/*__DR_DETECTOR_H__*/
