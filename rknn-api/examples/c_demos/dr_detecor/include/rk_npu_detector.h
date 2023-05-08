#ifndef __RK_NPU_DETECTOR_H__
#define __RK_NPU_DETECTOR_H__
#include "dr_detector.h"
#include "rknn_api.h"
class RKNpuDetector:public DrDetector{
private:
    rknn_context ctx;
    unsigned char *model_data;
    int model_data_size;

public:
    RKNpuDetector(int frameQueueSize,const char * name):DrDetector(frameQueueSize,name){
        rknnn_init();
    }

    virtual void process(Frame & frame);
    int rknnn_init();
    void rknn_deinit();
    virtual ~RKNpuDetector(){rknn_deinit();}

};
#endif/*__RK_NPU_DETECTOR_H__*/
