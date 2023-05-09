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
    RKNpuDetector(int frameQueueSize,const char * name,const char * modelName = "model/yolov5s_relu_out_opt.rknn"):DrDetector(frameQueueSize,name){
        LOG_INFO << "RKNpuDetector::RKNpuDetector Called!" <<endl;
        rknnn_init(modelName);
    }

    virtual void process(Frame & frame);
    int rknnn_init(const char * modelName);
    void rknn_deinit();
    virtual ~RKNpuDetector(){rknn_deinit();}

};
#endif/*__RK_NPU_DETECTOR_H__*/
