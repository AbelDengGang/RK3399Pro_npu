#include <opencv2/opencv.hpp>
#include <iostream>
using namespace std;
using namespace cv;
String window_name = "Capture - face detection";
int main() {
// 实例化
    VideoCapture camera;
    camera.open(0); // 打开摄像头, 默认摄像头cameraIndex=0
    if (!camera.isOpened())
    {
        cerr << "Couldn't open camera." << endl;
    }

    // 设置参数
    camera.set(CAP_PROP_FRAME_WIDTH, 1000);     // 宽度
    camera.set(CAP_PROP_FRAME_HEIGHT, 1000);    // 高度
    camera.set(CAP_PROP_FPS, 30);               // 帧率

    // 查询参数
    double frameWidth = camera.get(CAP_PROP_FRAME_WIDTH);
    double frameHeight = camera.get(CAP_PROP_FRAME_HEIGHT);
    double fps = camera.get(CAP_PROP_FPS);
    cout << "width:" << frameWidth << "," << "  height:"<< frameHeight << " fps:" << fps << endl;

    // 循环读取视频帧
    while (true)
    {
        Mat frame;
        camera >> frame;
        imshow(window_name, frame);
        if (waitKey(33) == 27) break; // ESC 键退出
    }

    // 释放
    camera.release();
    destroyWindow("camera");
    return 0;
}