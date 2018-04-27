#include <iostream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//#include <iomanip>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>

//#include <OpenNI.h>

#include "kcftracker.hpp"


//boost::asio::io_service iosev;
//boost::asio::serial_port sp(iosev);

//unsigned char startByte = 0xff;
//unsigned char endByte = 0xfe;
//std::string Base_Port = "/dev/ttyUSB0";

using namespace std;
//using namespace openni;
using namespace cv;

static const std::string RGB_WINDOW = "RGB Image window";
static const std::string DEPTH_WINDOW = "DEPTH Image window";

#define Max_linear_speed 0.6
#define Min_linear_speed 0.4
#define Min_distance 1.5
#define Max_distance 5.0
#define Max_rotation_speed 0.75

float linear_speed = 0;
float rotation_speed = 0;

float k_linear_speed = (Max_linear_speed - Min_linear_speed) / (Max_distance - Min_distance);
float h_linear_speed = Min_linear_speed - k_linear_speed * Min_distance;

float k_rotation_speed = 0.004;
float h_rotation_speed_left = 1.2;
float h_rotation_speed_right = 1.36;
 
int ERROR_OFFSET_X_left1 = 100;
int ERROR_OFFSET_X_left2 = 300;
int ERROR_OFFSET_X_right1 = 340;
int ERROR_OFFSET_X_right2 = 540;

cv::Mat rgbimage;
cv::Mat depthimage;
cv::Rect selectRect;
cv::Point origin;
cv::Rect result;

bool select_flag = false;
bool bRenewROI = false;  // the flag to enable the implementation of KCF algorithm for the new chosen ROI
bool bBeginKCF = false;
bool enable_get_depth = false;

bool HOG = true;
bool FIXEDWINDOW = false;
bool MULTISCALE = true;
bool SILENT = true;
bool LAB = false;

// Create KCFTracker object
KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);

float dist_val[5] ;

union Max_Value {
    unsigned char buf[8];
    struct _FLOAT_ {
        float _double_vT;
        float _double_vR;
    } Double_RAM;
} Send_Data;


//void onMouse(int event, int x, int y, int, void*)
//{
//    if (select_flag)
//    {
//        selectRect.x = MIN(origin.x, x);
//        selectRect.y = MIN(origin.y, y);
//        selectRect.width = abs(x - origin.x);
//        selectRect.height = abs(y - origin.y);
//        selectRect &= cv::Rect(0, 0, rgbimage.cols, rgbimage.rows);
//    }
//    if (event == CV_EVENT_LBUTTONDOWN)
//    {
//        bBeginKCF = false;
//        select_flag = true;
//        origin = cv::Point(x, y);
//        selectRect = cv::Rect(x, y, 0, 0);
//    }
//    else if (event == CV_EVENT_LBUTTONUP)
//    {
//        select_flag = false;
//        bRenewROI = true;
//    }
//}

//void onMouse(int evt, int x, int y, int flags, void* param) {
//    cout<<"~"<<endl;
//    if(evt == CV_EVENT_LBUTTONDOWN) {
//        std::vector<cv::Point>* ptPtr = (std::vector<cv::Point>*)param;
//        ptPtr->push_back(cv::Point(x,y));
//    }
//}

void hMirrorTrans(const Mat &src, Mat &dst)
{
    dst.create(src.rows, src.cols, src.type());

    int rows = src.rows;
    int cols = src.cols;

    switch (src.channels())
    {
    case 1:   //1通道比如深度图像
        const uchar *origal;
        uchar *p;
        for (int i = 0; i < rows; i++){
            origal = src.ptr<uchar>(i);
            p = dst.ptr<uchar>(i);
            for (int j = 0; j < cols; j++){
                p[j] = origal[cols - 1 - j];
            }
        }
        break;
    case 3:   //3通道比如彩色图像
        const Vec3b *origal3;
        Vec3b *p3;
        for (int i = 0; i < rows; i++) {
            origal3 = src.ptr<Vec3b>(i);
            p3 = dst.ptr<Vec3b>(i);
            for (int j = 0; j < cols; j++){
                p3[j] = origal3[cols - 1 - j];
            }
        }
        break;
    default:
        break;
    }

}

int main(int argc, char** argv)
{
    string file_path = argv[1];
    string depth_path = file_path + "/depth";
    string color_path = file_path + "/rgb";
    cv::String path(color_path);
    vector<cv::String> fn;
    vector<cv::Mat> data;
    cv::glob(path + "/*.png",fn,true); // recurse
    // 创建OpenCV图像窗口
    namedWindow(RGB_WINDOW, CV_WINDOW_AUTOSIZE);
    namedWindow(DEPTH_WINDOW, CV_WINDOW_AUTOSIZE);

    // select ROI
    // load bounding box
    string bbox_path = file_path + "/init.txt";
    ifstream file(bbox_path);
    string str;
    getline(file, str);
//    cout << str << endl;

    std::vector<int> vect;
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        vect.push_back(i);

        if (ss.peek() == ',')
            ss.ignore();
    }

//    for (i=0; i< vect.size(); i++)
//        std::cout << vect.at(i)<<std::endl;

    // setting bounding box
    selectRect.x = vect.at(0);
    selectRect.y = vect.at(1);
    selectRect.width = vect.at(2);
    selectRect.height = vect.at(3);

    // initialize the tracker
    // read color image
    string color_image_path = color_path + "/1.png";
    rgbimage = imread(color_image_path, CV_LOAD_IMAGE_COLOR);   // Read the file
    // read depth image
    string depth_image_path = depth_path + "/1.png";
    depthimage = imread(depth_image_path, CV_LOAD_IMAGE_UNCHANGED);
    resize(rgbimage, rgbimage, Size(640, 480));
//    double min, max;
//    cv::Point min_loc, max_loc;
//    cv::minMaxLoc(depthimage, &min, &max, &min_loc, &max_loc);
//    cout << min_loc.x << "\t" << min_loc.y << endl;
//    cout << max_loc.x << "\t" << max_loc.y << endl;
//    cout << min << '\t' << max << endl;
//    float depth = depthimage.at<ushort>(597, 148);
//    cout << depth << endl;
//    depth = depthimage.at<ushort>(148, 597);
//    cout << depth << endl;
//    depth = depthimage.at<ushort>(max_loc);
//    cout << depth << endl;
//    cout << depthimage.at<float>(148, 597) << endl;
//    float depth;
//    for(int row = selectRect.x; row < selectRect.x + selectRect.width; row++)
//    {
//        row = 146;
//        for(int col = selectRect.y; col < selectRect.y + selectRect.height; col++)
//        {
//            if(depthimage.at<ushort>(row, col) > 0) {
//                depth = depthimage.at<ushort>(row, col) / 1000.0;
//                cout << depth << "\t";
//            }
//        }
//        break;
//        cout << endl;
//    }
//    imshow(RGB_WINDOW, rgbimage);
//    imshow(DEPTH_WINDOW, depthimage);
//    waitKey(0);
    tracker.init(selectRect, rgbimage, depthimage);
//    return -1;

//    string color_image_path = color_path + "/1.png";
//    rgbimage = imread(color_image_path, CV_LOAD_IMAGE_COLOR);
//    imshow(RGB_WINDOW, rgbimage);
//    vector<Point> points;
//    cv::setMouseCallback("Output Window", onMouse, (void*)&points);
//    cout << points.size() << endl;
//
//    while(1) {
//        cv::imshow("Output Window", frame);
//
//        if (points.size() > 2) //we have 2 points
//        {
//            for (auto it = points.begin(); it != points.end(); ++it) {
//
//                cout << "X and Y coordinates are given below" << endl;
//                cout << (*it).x << '\t' << (*it).y << endl;
//            }
//            break;
//            //draw points
//        }
//    }

    Mat image;
    for (size_t k=0; k<fn.size(); ++k)
    {
        ostringstream convert;
        convert << k+1;
        // read color image
        string color_image_path = color_path + "/" + convert.str() + ".png";
        rgbimage = imread(color_image_path, CV_LOAD_IMAGE_COLOR);   // Read the file
        // read depth image
        string depth_image_path = depth_path + "/" + convert.str() + ".png";
        depthimage = imread(depth_image_path, CV_LOAD_IMAGE_GRAYSCALE);
        resize(rgbimage, rgbimage, Size(640, 480));

        cout << convert.str() << endl;

//        // 读取数据流
//        rc = streamDepth.readFrame(&frameDepth);
//        if (rc == STATUS_OK)
//        {
//            // 将深度数据转换成OpenCV格式
//            const Mat mImageDepth(frameDepth.getHeight(), frameDepth.getWidth(), CV_16UC1, (void*)frameDepth.getData());
//            // 为了让深度图像显示的更加明显一些，将CV_16UC1 ==> CV_8U格式
//            //Mat mScaledDepth;//, hScaledDepth;
//            //mImageDepth.convertTo(mScaledDepth, CV_8U, 255.0 / iMaxDepth);
//            // cout << " 1 " << mImageDepth.ptr<ushort>(320)[240] << endl;
//            // cout << " 2 " << mImageDepth.at<float>(320,240) << endl;
//            //水平镜像深度图
//            //hMirrorTrans(mScaledDepth, depthimage);
//            //hMirrorTrans(mImageDepth, depthimage);
//            mImageDepth.copyTo(depthimage);
//        }
//
//        rc = streamColor.readFrame(&frameColor);
//        if (rc == STATUS_OK)
//        {
//            // 同样的将彩色图像数据转化成OpenCV格式
//            const Mat mImageRGB(frameColor.getHeight(), frameColor.getWidth(), CV_8UC3, (void*)frameColor.getData());
//            // 首先将RGB格式转换为BGR格式
//            // Mat cImageBGR,bImageBGR;//,hImageBGR;
//            cvtColor(mImageRGB, rgbimage, CV_RGB2BGR);
//
//            //水平镜像深度图
//            //hMirrorTrans(cImageBGR, rgbimage);
//            resize(rgbimage, rgbimage, Size(640, 480));
//        }

//        setMouseCallback(RGB_WINDOW, onMouse, 0);
//        selectRect = selectROI("Image", im, fromCenter);


//        if(bRenewROI)
//        {
//            if (selectRect.width <= 0 || selectRect.height <= 0)
//            {
//                bRenewROI = false;
//                continue;
//            }
//            tracker.init(selectRect, rgbimage, depthimage);
//            bBeginKCF = true;
//            bRenewROI = false;
//            enable_get_depth = false;
//        }

        if(1)
        {
            
//            cout << "1111111111111"<<endl;
            result = tracker.update(rgbimage, depthimage);
            cv::rectangle(rgbimage, result, cv::Scalar( 0, 255, 255 ), 2, 8 );
            enable_get_depth = true;
//            cout << "22222222222222" << endl;
        }
        else
            cv::rectangle(rgbimage, selectRect, cv::Scalar(255, 0, 0), 2, 8, 0);

        // 然后显示彩色图像
        imshow(RGB_WINDOW, rgbimage);

        if(1)
        {
            // dist_val[0] = depthimage.ptr<ushort>(result.y+result.height/3)[result.x+result.width/3];
            // dist_val[1] = depthimage.ptr<ushort>(result.y+result.height/3)[result.x+2*result.width/3];
            // dist_val[2] = depthimage.ptr<ushort>(result.y+2*result.height/3)[result.x+result.width/3] ;
            // dist_val[3] = depthimage.ptr<ushort>(result.y+2*result.height/3)[result.x+2*result.width/3] ;
            // dist_val[4] = depthimage.ptr<ushort>(result.y+result.height/2)[result.x+result.width/2] ;

            // for(int i = 0; i < 5; i++)
            //     dist_val[i] = dist_val[i] / 1000.0;

            // float distance = 0;
            // int num_depth_points = 5;
            // for(int i = 0; i < 5; i++)
            // {
            //     if(dist_val[i] > 0.4)
            //         distance += dist_val[i];
            //     else
            //         num_depth_points--;
            // }
            // distance /= num_depth_points;
            float distance = tracker.getDepth(result, depthimage);

            //calculate linear speed
            if(distance > Min_distance)
                linear_speed = distance * k_linear_speed + h_linear_speed;
            else
                linear_speed = 0;

            if(linear_speed > Max_linear_speed)
                linear_speed = Max_linear_speed;

            //calculate rotation speed
            int center_x = result.x + result.width/2;
            if(center_x < ERROR_OFFSET_X_left1) 
                rotation_speed =  Max_rotation_speed;
            else if(center_x > ERROR_OFFSET_X_left1 && center_x < ERROR_OFFSET_X_left2)
                rotation_speed = -k_rotation_speed * center_x + h_rotation_speed_left;
            else if(center_x > ERROR_OFFSET_X_right1 && center_x < ERROR_OFFSET_X_right2)
                rotation_speed = -k_rotation_speed * center_x + h_rotation_speed_right;
            else if(center_x > ERROR_OFFSET_X_right2)
                rotation_speed = -Max_rotation_speed;
            else 
                rotation_speed = 0;

            std::cout <<  "-----------------------------------------------------------------------"  << std::endl;
            std::cout <<  "linear_speed = " << linear_speed << "  rotation_speed = " << -rotation_speed << std::endl;
            std::cout <<  dist_val[0]  << " / " <<  dist_val[1] << " / " << dist_val[2] << " / " << dist_val[3] <<  " / " << dist_val[4] << std::endl;
            std::cout <<  "distance = " << distance << std::endl;
        }
//        break;
        // 显示出深度图像
        imshow(DEPTH_WINDOW, depthimage);

        //sendVelocity(linear_speed, -rotation_speed);

        // 终止快捷键
        if (waitKey(30) >= 0)
        {
            //sendVelocity(0, 0);
            break;
        }
    }
    destroyWindow(RGB_WINDOW);
//    // 关闭数据流
//    streamDepth.destroy();
//    streamColor.destroy();
//    // 关闭设备
//    xtion.close();
//    // 最后关闭OpenNI
//    OpenNI::shutdown();

    return 0;
}
