#ifndef __DR_DECTOR_H__
#define __DR_DECTOR_H__
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <semaphore.h>
#include <pthread.h>
#include <thread>
#include <sys/time.h>

using namespace std;
class Frame{
    int frameID;
    cv::Mat frame;
};

class DrDetector{
private:
    pthread_mutex_t mutexFrame;     // 缓冲区互斥锁
    sem_t full_sem;                 // 缓冲区计数信号量, 为0则处理线程阻塞
    int queueSize;
    Frame * frameQueue;
    std::string detectorName;
    std::thread * pWorkThread;

    bool stopThread;

public:

    void* workThd()
    {
        // 没有考虑该如何退出
        while (1)
        {
            int k=1;
            int result;
            // 用sem_timedwait，当改变系统时间的时候，会有永远无法超时的风险， 不过问题不大，也许过一段时间新的图片来了，会唤醒他
            struct timespec ts;
            struct timeval tt;
            gettimeofday(&tt,NULL);
            ts.tv_sec = tt.tv_sec + k;
            ts.tv_nsec = tt.tv_usec*1000 ;

            result=sem_timedwait(&full_sem,&ts);
            if (stopThread){
                std::cout << detectorName << " quit work thread!" << endl;
                break;
            }
            if(result == 0){
                std::cout << detectorName << " process a new frame!" << endl;
            }else if (result == -1){
                std::cout << detectorName << " wait frame timeout!" << endl;    
            }
        }
        printf("Leave thread1!\n");

        return NULL;
    }


    DrDetector(int frameQueueSize,const char * name):detectorName(name){
        queueSize = frameQueueSize;
        frameQueue = new Frame[frameQueueSize];
        sem_init(&full_sem, 0, 0);    //初始化满的信号量
        pthread_mutex_init(&mutexFrame, nullptr);

        // 创建接收线程
        stopThread = false;
        pWorkThread = new std::thread (&DrDetector::workThd, this);
    }

    int getQueueSize(void){
        return queueSize;
    }

    ~DrDetector(){
        stopThread = true;
        std::cout << detectorName << " waiting for work thread quit" << endl;
        sem_post(&full_sem); // 唤醒工作线程，让他退出

        pWorkThread->join(); // 等待工作线程退出
        std::cout << detectorName << " work thread quited" << endl;
        if(frameQueue){
            delete []frameQueue;
        }

        
        sem_destroy(&full_sem);
        pthread_mutex_destroy(&mutexFrame);


    }
    bool tryAddFrame(Frame &frame);
};

#endif/*__DR_DECTOR_H__*/
