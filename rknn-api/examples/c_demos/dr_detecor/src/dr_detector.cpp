#include "dr_detector.h"
void Frame::copyFrom(Frame &frame){
        frameID = frame.frameID;
        frame.picture.copyTo(picture);

        // 每个Frame只保留一份 缩小后的图或者切片列表
        // 怎么管理还没有想好
        // 这里会内存泄露

        // TODO resizedPicture,只有一份, 这里临时hack一下，以后要改成引用计数
        if(frame.resizedPicture){
            if(!resizedPicture){
                assert(resizedPicture = new cv::Mat());
            }
            *resizedPicture = *frame.resizedPicture;
        }else
        {
            // 不删除自己的，留着内存给后面的resize函数使用
        }
        

        // TODO slices,只有一份, 这里临时hack一下，以后要改成引用计数
        if( frame.sliceCnt){
            if (sliceCnt != frame.sliceCnt){
                if(slices){
                    delete [] slices;
                }
                assert(slices = new FrameSlice[frame.sliceCnt]);
                sliceCnt = frame.sliceCnt;
                for (int i=0;i<sliceCnt;++i){
                    slices[i] = frame.slices[i];
                }
            }
        }else{
            // 不删除自己的，留着内存给后面的slice函数使用
        }
}

void Frame::clear(void){
    if(slices)
    {
        delete []slices;
    }
    if(resizedPicture)
    {
        delete resizedPicture;
    }

}

void printStackTrace() {
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    printf("Stack trace:\n");
    for (int i = 0; i < frames; ++i) {
        std::cout << strs[i] << std::endl;
    }
    free(strs);
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
    LOG_INFO << "DrDetector::DrDetector Called!" <<endl;
    timeout = 10;
    queueSize = frameQueueSize;
    validFrameCnt = 0;
    queueHead=0;
    queueTail=0;
    model_input_height = 320;
    model_input_width = 320;
    model_input_channel = 3;
    resize_to_fit_model = true;    

    frameQueue = new Frame[frameQueueSize];
    sem_init(&full_sem, 0, 0);    //初始化满的信号量
    pthread_mutex_init(&mutexFrame, nullptr);

    // 创建接收线程
    stopThread = false;
    pWorkThread = new std::thread (&DrDetector::workThd, this);
}

bool showResizePicture = true;
void DrDetector::resize_to_model(Frame &frame){
    if (!frame.resizedPicture )
        frame.resizedPicture = new cv::Mat(model_input_height,model_input_width,CV_8UC3,cv::Scalar(0,0,0));

    int org_height = frame.picture.rows;
    int org_width = frame.picture.cols;
    //LOG_INFO << "Picture size (w x h) :" << org_width << "x" << org_height <<  endl;
    if (model_input_height == org_height  && model_input_width == org_width){
        // 无需缩放，直接拷贝即可
//#ifdef DETAIL_LOG
        LOG_INFO << "Picture size same as model, just copy" << endl;
//#endif        
        *frame.resizedPicture = frame.picture.clone();
        //cv::imshow("resized", (*frame.resizedPicture));
        return;
    }
    // 确定是x方向还是y方向填充
    // 这里默认模型是正方形
    float scale_rate;
    if (org_height >= org_width){
        // x 方向填充
        scale_rate = ((float)model_input_height-1) / (float)org_height  ; // 留点框，以免溢出边界
        
    }else{
        // y 方向填充
        scale_rate = ((float)model_input_width-1) / (float)org_width  ; // 留点框，以免溢出边界
    }
    int scaled_width = (int)(org_width * scale_rate + 0.5);
    int scaled_height = (int)(org_height * scale_rate + 0.5);

    int scaled_start_x =  (model_input_width - scaled_width) / 2;
    int scaled_start_y =  (model_input_height - scaled_height) / 2;
    //LOG_INFO << "Reize to (w x h)" << scaled_width << " x " <<  scaled_height << endl;
    cv::Mat subImg = (*frame.resizedPicture)(cv::Rect(scaled_start_x, scaled_start_y, scaled_width, scaled_height));
    cv::resize(frame.picture, subImg, cv::Size(scaled_width, scaled_height), 0, 0, cv::INTER_LINEAR);
    if (showResizePicture){
        cv::imshow("resized", (*frame.resizedPicture));
        //if (cv::waitKey(33) == 27) {}
    }

}   

bool DrDetector::tryAddFrame(Frame &frame){
    bool ret = false;
    pthread_mutex_lock(&mutexFrame);
    if (validFrameCnt < queueSize){

        Frame *pTailFrame = &frameQueue[queueTail];
        queueTail = (queueTail + 1) % queueSize;
        pTailFrame->copyFrom(frame);
        // 对队列中的图片进行切片或者缩放操作，而不是对原始图进行切片操作，
        // 这样可以节省一次内存拷贝的动作
        if (resize_to_fit_model){
            resize_to_model(*pTailFrame);
        }else{
            // TODO 
            // 切片
        }
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