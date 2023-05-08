#ifndef __DR_DECTOR_H__
#define __DR_DECTOR_H__
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <semaphore.h>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <sys/time.h>
#define LOG_ERR std::cerr
#define LOG_INFO std::cout
using namespace std;
class Frame{
public:
    int frameID;
    cv::Mat picture;
    void copyFrom(Frame &frame);
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

    std::string detectorName;
    std::thread * pWorkThread;
    int timeout;

    sem_t full_sem;                         // 缓冲区计数信号量, 为0则处理线程阻塞
    bool stopThread;
    bool getHeadFrame(Frame & outFrame);    // 获取队列中的第一个Frame用作处理 
public:

    void* workThd();


    DrDetector(int frameQueueSize,const char * name);

    int getQueueSize(void){
        return queueSize;
    }

    ~DrDetector(){
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
};

#endif/*__DR_DECTOR_H__*/
