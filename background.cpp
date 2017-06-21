#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
//#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

#define KEY_SPACE 32
#define KEY_ESC 27
#define X_FACTOR .2L
#define Y_FACTOR .2L

RNG rng(12345);

//crosswalk points
vector <Point> points = {};
CascadeClassifier cascade;


Rect detect(Mat img);
void Erosion(Mat &img);
void Contours(Mat &img, Mat &original);
void Dilation(Mat &img);

int main(int argc, char **arv)
{
  cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;

  cascade.load("cars.xml");
  namedWindow("Frame", 1);
  namedWindow("Mask", 1);
  
  //reading crosswalk locations from xml -------------------------------

  string xmlname="myxml.xml"; 
  FileStorage fs(xmlname, FileStorage::READ);
  FileNode root = fs.getFirstTopLevelNode();
  Point pt(-1,-1);
  int size = (int) root.size();
  for(int x = 0; x < size; x++)
    {
      //gives polygons.
      FileNode polygon = root["polygon"+to_string(x)];
      for (int y = 0; y <4; y++)
	{
	  pt.x = double(polygon["pointx"+to_string(y)]);
	  pt.y = double(polygon["pointy"+to_string(y)]);
	  points.push_back(pt);
	}
    }
  fs.release();

  //done reading crosswalk locations -------------------------------------

  Mat frame, fgMask, frame_gray;

  //VideoCapture cap("https://d1h84if288zv9w.cloudfront.net/7dias/0be3_408.stream/playlist.m3u8");
  VideoCapture cap("traffic.avi");
  
  Ptr<BackgroundSubtractor> subtractor;
  subtractor = createBackgroundSubtractorMOG2();
  
  if(!cap.isOpened())
    {
      cout << "video error " << endl;
      return -1;
    }

  for(int i =0; i <200; i++)
    {
      cap >> frame;
    }
  const int MIN_AREA = frame.cols*frame.rows / 6700;
  cout << MIN_AREA << endl;
  
  Mat original;
  int key =0;
  for(;;)
    {
      cap >> original;
      cv::resize(original,frame,Size(),X_FACTOR,Y_FACTOR);
      
      if (frame.empty() || frame.rows == 0 || frame.cols == 0)
	break;

      subtractor->apply(frame,fgMask,.005);

      cvtColor(frame, frame_gray, CV_BGR2GRAY);
      blur(frame_gray,frame_gray,Size(3,3));

      Erosion(fgMask);
      threshold(fgMask,fgMask,200,255,THRESH_TOZERO);
      //Dilation(fgMask);
      Contours(fgMask, original);
      imshow("Mask",fgMask);
      
      imshow("frame", original);
      
      key = waitKey(1);
      if(key == KEY_SPACE)
	key = waitKey(0);
      if(key == KEY_ESC || key == (int) 'q')
	break;
      
    }
  cap.release();
  cvDestroyAllWindows();
  return 0;
}

int detect(Mat img, Rect result)
{
  Mat A, B, C;
  C= Mat::zeros(img.size(), 0);
  std::vector<Rect> cars;
  cascade.detectMultiScale(img,cars,1.1,1,0,Size(32,32));
  
  // Old detection code --------------------------------------------------------------
  /*for (int x = 0; x  < points.size(); x+=4)
    {
      A= Mat::zeros(img.size(), 0);
      Point pts [] = {points.at(x),points.at(x+1),points.at(x+2),points.at(x+3)};
      fillConvexPoly(img,pts,4,Scalar(150,0,0));
      fillConvexPoly(A,pts,4,Scalar(255));
      for(int i=0; i < cars.size(); i++)
	{
	  B= Mat::zeros(img.size(), 0);
	  rectangle(B,cars.at(i),Scalar(255),CV_FILLED);
	  bitwise_and(A,B,C);
	  //if (countNonZero(C) / (float)countNonZero(B) > .6)
	    // count++;
	    }
    }
  for(int i=0; i < cars.size(); i++)
    { 
      rectangle(img,cars.at(i),Scalar(255,0,0),1);
    }
  imshow("video",img);
  // End old detection code ---------------------------------------------------------*/
  
    if (cars.size() > 0)
      {
	result = cars.at(0);
	return cars.size();
      }
    return 0;
}

void Erosion(Mat &img)
{
  //Mat dst = Mat::zeros(img.size(),0);
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(3,3),Point(1,1));
  erode(img,img,element);
  //imshow("Mask",dst);
}

void Dilation(Mat &img)
{
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(3,3),Point(1,1));
  dilate(img,img,element);
}

void Contours(Mat &img, Mat &original)
{
  Mat canny_output;
  Rect ROI;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  Canny(img, canny_output, 100, 200, 3);
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(3,3));
  morphologyEx(canny_output, canny_output, MORPH_CLOSE, element);
  findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );

  Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );
  for(int i =0; i < contours.size(); i++ )
    {
      Scalar color = Scalar( rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255) );
      if (arcLength(contours.at(i),true) > 50)
	{
	  //draw rectangle onto x
	  ROI = boundingRect(contours.at(i));

	  //trim ROI so it fits in original, then run detector.
	  ROI = ROI & cv::Rect(0,0,img.cols, img.rows);

	  ROI.width /= X_FACTOR;
	  ROI.height /= Y_FACTOR;
	  ROI.x /= X_FACTOR;
	  ROI.y /= Y_FACTOR;
	  
	  Mat B = original(ROI);
	  
	  int valid = detect(B, ROI);

	  //calculate number of nonzero pixels in ROI area
	  ///int count = countNonZero(B);

	  
	  /*if (count > 100)
	    {
	      rectangle(original, ROI, color);
	    }*/
	  
	  //if (valid)
	  // {
	  //   rectangle(original,ROI,color);
	  // }

	  // drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
	}
    }

  
  
  // namedWindow("Contours", CV_WINDOW_AUTOSIZE );
  //imshow("Contours", original);
}

