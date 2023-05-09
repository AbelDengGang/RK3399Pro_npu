#include "dr_detector.h"
void Frame::copyFrom(Frame &frame){
        frameID = frame.frameID;
        frame.picture.copyTo(picture);
}

void* DrDetector::workThd()
{
    while (1)
    {
        int result;
        // 用sem_timedwait，当改变系统时间的时候，会有永远无法超时的风险， 不过问题不大，也许过一段时间新的图片来了，会唤醒他
        struct timespec ts;
        struct timeval tt;
        gettimeofday(&tt,NULL);
        ts.tv_sec = tt.tv_sec + timeout;
        ts.tv_nsec = tt.tv_usec*1000 ;

        result=sem_timedwait(&full_sem,&ts);
        if (stopThread){
            LOG_INFO << detectorName << ": quit work thread!" << endl;
            break;
        }
        if(result == 0){
#ifdef DETAIL_LOG            
            struct timeval tt1;
            gettimeofday(&tt1,NULL);
            LOG_INFO << tt1.tv_sec << "." << tt1.tv_usec << detectorName << ": process a new frame!" <<  endl;
#endif            
            {
                Frame workFrame;
                bool ret = getHeadFrame(workFrame);
                if (ret){
                    // Todo 后续的检测操作
                    process(workFrame);
                }else{
                    LOG_ERR << detectorName << ": get frame failed!" << endl;        
                }
            }
        }else if (result == -1){
            LOG_ERR << detectorName << ": wait frame timeout!" << endl;    
        }
    }
    LOG_INFO << detectorName << ": Leave workThd!" << endl;

    return NULL;
}

DrDetector::DrDetector(int frameQueueSize,const char * name):detectorName(name){
    timeout = 10;
    queueSize = frameQueueSize;
    validFrameCnt = 0;
    queueHead=0;
    queueTail=0;
    frameQueue = new Frame[frameQueueSize];
    sem_init(&full_sem, 0, 0);    //初始化满的信号量
    pthread_mutex_init(&mutexFrame, nullptr);

    // 创建接收线程
    stopThread = false;
    pWorkThread = new std::thread (&DrDetector::workThd, this);
}

bool DrDetector::tryAddFrame(Frame &frame){
    bool ret = false;
    pthread_mutex_lock(&mutexFrame);
    if (validFrameCnt < queueSize){
        Frame *pTailFrame = &frameQueue[queueTail];
        queueTail = (queueTail + 1) % queueSize;
        pTailFrame->copyFrom(frame);
        validFrameCnt ++;
        ret = true;
#ifdef DETAIL_LOG        
        struct timeval tt1;
        gettimeofday(&tt1,NULL);
        LOG_INFO << tt1.tv_sec << "." << tt1.tv_usec << detectorName << ": add a new frame!" <<  endl;
#endif        
        sem_post(&full_sem); 

    }else{
        ret = false;
    }
    pthread_mutex_unlock(&mutexFrame);
    return ret;
}

bool DrDetector::getHeadFrame(Frame & outFrame){
    bool ret = false;
    pthread_mutex_lock(&mutexFrame);
    if( validFrameCnt > 0 ){
        Frame *pHeadFrame = &frameQueue[queueHead];
        queueHead = (queueHead + 1) % queueSize;
        outFrame.copyFrom(*pHeadFrame);
        validFrameCnt --;
        ret = true;
    }else{
        ret = false;
    }
    pthread_mutex_unlock(&mutexFrame);
    return ret;
}