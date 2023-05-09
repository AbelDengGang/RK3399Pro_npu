#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace cv;
int main(int argc, char** argv )
{
    //cv::Mat image(100, 100, CV_8UC3, Scalar(255, 0, 0)); //BLUE
    //cv::Mat image(100, 100, CV_8UC3, Scalar(0, 255, 0));    //GREEN
    //cv::Mat image(100, 100, CV_8UC3, Scalar(0, 0, 255));    //RED
    cv::Mat image(100, 100, CV_8UC3, Scalar(10, 128, 255));   // HWC格式， BGR888
    int totalbyte = image.elemSize() * image.cols * image.rows;
    std::cout << "100*100*3 = " << 100*100*3 << std::endl; 
    std::cout << "total bytes = " <<  totalbyte << std::endl; 
    for(int i = 0;i<9;i++){
        std::cout << i << ":" << (unsigned int)(image.data[i]) << std::endl;
    }

    // 设置ROI区域，并修改成黑色
    cv::Mat subImg = image(cv::Rect(10, 20, 10, 10)); 
    subImg.setTo(Scalar(0,0,0));

    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", image);
    waitKey(0);
    return 0;
}