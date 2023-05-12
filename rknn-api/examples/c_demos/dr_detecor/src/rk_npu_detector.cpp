#include "rk_npu_detector.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#undef cimg_display
#define cimg_display 0
#include "CImg/CImg.h"

#include "drm_func.h"
#include "rga_func.h"
#include "rknn_api.h"
#include "postprocess.h"



/*-------------------------------------------
                  Functions
-------------------------------------------*/

inline const char* get_type_string(rknn_tensor_type type)
{
    switch(type) {
    case RKNN_TENSOR_FLOAT32: return "FP32";
    case RKNN_TENSOR_FLOAT16: return "FP16";
    case RKNN_TENSOR_INT8: return "INT8";
    case RKNN_TENSOR_UINT8: return "UINT8";
    case RKNN_TENSOR_INT16: return "INT16";
    default: return "UNKNOW";
    }
}

inline const char* get_qnt_type_string(rknn_tensor_qnt_type type)
{
    switch(type) {
    case RKNN_TENSOR_QNT_NONE: return "NONE";
    case RKNN_TENSOR_QNT_DFP: return "DFP";
    case RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC: return "AFFINE";
    default: return "UNKNOW";
    }
}

inline const char* get_format_string(rknn_tensor_format fmt)
{
    switch(fmt) {
    case RKNN_TENSOR_NCHW: return "NCHW";
    case RKNN_TENSOR_NHWC: return "NHWC";
    default: return "UNKNOW";
    }
}

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp)
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

static unsigned char *load_model(const char *filename, int *model_size)
{

    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}


void RKNpuDetector::process(Frame & frame){
    if(frame.resizedPicture){
        //cv::imshow("detect", (*frame.resizedPicture));
        LOG_INFO << detectorName << " resizedPicture in !" << endl;
        int ret;
        // 使用单张图片进行检测
        struct timeval start_time, stop_time;
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].size = model_input_width * model_input_height * model_input_channel;
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].pass_through = 0;
        inputs[0].buf = (void *)frame.resizedPicture->data;
        gettimeofday(&start_time, NULL);
        rknn_inputs_set(ctx, io_num.n_input, inputs);
        rknn_output outputs[io_num.n_output];
        memset(outputs, 0, sizeof(outputs));
        for (int i = 0; i < io_num.n_output; i++)
        {
            outputs[i].want_float = 0;
        }

        ret = rknn_run(ctx, NULL);
        ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
        gettimeofday(&stop_time, NULL);
        printf("once run use %f ms\n",
           (__get_us(stop_time) - __get_us(start_time)) / 1000);


        //post process
        float scale_w = 1.0;//(float)width / img_width;
        float scale_h = 1.0;//(float)height / img_height;

        detect_result_group_t detect_result_group;
        std::vector<float> out_scales;
        std::vector<uint32_t> out_zps;
        for (int i = 0; i < io_num.n_output; ++i)
        {
            out_scales.push_back(output_attrs[i].scale);
            out_zps.push_back(output_attrs[i].zp);
        }
        post_process((uint8_t *)outputs[0].buf, (uint8_t *)outputs[1].buf, (uint8_t *)outputs[2].buf, model_input_height, model_input_width,
                    0.5, 0.6, scale_w, scale_h, out_zps, out_scales, &detect_result_group);

        // Draw Objects
        char text[256];
        const unsigned char blue[] = {0, 0, 255};
        const unsigned char white[] = {255, 255, 255};
        for (int i = 0; i < detect_result_group.count; i++)
        {
            detect_result_t *det_result = &(detect_result_group.results[i]);
            sprintf(text, "%s %.2f", det_result->name, det_result->prop);
            printf("%s @ (%d %d %d %d) %f\n",
                det_result->name,
                det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                det_result->prop);
            int x1 = det_result->box.left;
            int y1 = det_result->box.top;
            int x2 = det_result->box.right;
            int y2 = det_result->box.bottom;
#if 0            
            //draw box
            // TODO use OPENCV function to draw box
            img.draw_rectangle(x1, y1, x2, y2, blue, 1, ~0U);
            img.draw_text(x1, y1 - 12, text, white);
#endif            
        }

    }

}


int RKNpuDetector::rknnn_init(const char * model_name){
    int ret;
    //char *model_name = "model/yolov5s_relu_out_opt.rknn";
    /* Create the neural network */
    printf("Loading mode...\n");
    model_data_size = 0;
    model_data = load_model(model_name, &model_data_size);
    ret = rknn_init(&ctx, model_data, model_data_size, RKNN_FLAG_PRIOR_MEDIUM|RKNN_FLAG_ASYNC_MASK);
    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        return -1;
    }

    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version,
                    sizeof(rknn_sdk_version));

    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        return -1;
    }
    printf("sdk version: %s driver version: %s\n", version.api_version,
           version.drv_version);


    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input,
           io_num.n_output);

    //rknn_tensor_attr input_attrs[io_num.n_input];
    input_attrs = new rknn_tensor_attr[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]),
                         sizeof(rknn_tensor_attr));
        if (ret < 0)
        {
            printf("rknn_init error ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    //rknn_tensor_attr output_attrs[io_num.n_output];
    output_attrs = new rknn_tensor_attr[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]),
                         sizeof(rknn_tensor_attr));
        dump_tensor_attr(&(output_attrs[i]));
        if(output_attrs[i].qnt_type != RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC || output_attrs[i].type != RKNN_TENSOR_UINT8)
        {
            fprintf(stderr,"The Demo required for a Affine asymmetric u8 quantized rknn model, but output quant type is %s, output data type is %s\n", 
                    get_qnt_type_string(output_attrs[i].qnt_type),get_type_string(output_attrs[i].type));
            return -1;
        }
    }

    int channel = 3;
    int width = 0;
    int height = 0;
    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        width = input_attrs[0].dims[0];
        height = input_attrs[0].dims[1];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        width = input_attrs[0].dims[1];
        height = input_attrs[0].dims[2];
    }
    model_input_height = height;
    model_input_width = width;
    model_input_channel = channel;
    printf("model input height=%d, width=%d, channel=%d\n", height, width,
           channel);

    return 0;

}

void RKNpuDetector::rknn_deinit(){
    int ret;
    // release
    ret = rknn_destroy(ctx);
    if (model_data)
    {
        free(model_data);
    }

    if (output_attrs){
        delete []output_attrs;
    }

    if (input_attrs){
        delete []input_attrs;
    }
}
