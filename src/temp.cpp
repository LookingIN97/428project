//
// Created by Wenzhang Qian on 2018-04-26.
//

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
#include <sstream>

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{
  cout << argc << endl;
  string file_path = argv[1];
  string depth_path = file_path + "/depth";
  string color_path = file_path + "/rgb";
  cout << color_path << endl;

  cv::String path(color_path); //select only jpg
  vector<cv::String> fn;
  vector<cv::Mat> data;
  cv::glob(path + "/*.png",fn,true); // recurse
    namedWindow( "Display window", WINDOW_AUTOSIZE );
    for (size_t k=0; k<fn.size(); ++k)
    {
        Mat image;
        ostringstream convert;
        convert << k+1;
        string color_image_path = color_path + "/" + convert.str() + ".png";
        image = imread(color_image_path, CV_LOAD_IMAGE_COLOR);
        cout<<color_image_path<<endl;
        if (!image.data) {
            cout << "Could not open or find the image" << endl;
            return -1;
        }
        imshow( "Display window", image );
    }

//  Mat image;
//  cout << color_path << endl;
//  image = imread(color_path, CV_LOAD_IMAGE_COLOR);   // Read the file
//
//  if(! image.data )                              // Check for invalid input
//  {
//    cout <<  "Could not open or find the image" << std::endl ;
//    return -1;
//  }
//
//  namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
//  imshow( "Display window", image );                   // Show our image inside it.

  waitKey(0);                                          // Wait for a keystroke in the window
  return 0;
}