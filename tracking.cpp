#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/utility.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

const int KEY_SPACE = 32;
const int KEY_ESC = 27;
const string trackingalg = "KCF";

CascadeClassifier cascade;
//crosswalk points
vector <Point> points = {};
MultiTracker trackers(trackingalg);
vector <Rect2d> objects;
//vector <Ptr <Tracker> > algorithms;

int detect(Mat img);

int main(int argc, char** argv)
{
  cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;
  
  cascade.load("cas1.xml");

  //READING XML for crosswalk-----------------------------------
  
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

  //DONE READING XML -----------------------------------------

  Mat frame;
  
  VideoCapture cap("traffic.avi");
  //VideoCapture cap("https://d1h84if288zv9w.cloudfront.net/7dias/0be3_408.stream/playlist.m3u8");
  for(int i = 0;i<200;i++)
    cap >> frame;
  
  cvNamedWindow("video", 1);
  int key = 0; //key pressed
  int count = 0; //count of cars blocking crosswalk
  int oldcount= 0; //previous count. Used for printing.
  
  /* for (int x = 0; x  < points.size(); x+=4)
    {
      Point pts [] = {points.at(x),points.at(x+1),points.at(x+2),points.at(x+3)};
      fillConvexPoly(crosswalks,pts,4,Scalar(255));
      }*/
  
  for(unsigned int i = 0;;i++)
  {
    cap >> frame;
    if (frame.empty())
      break;
    if (i%5 == 0)
      {
	objects = {};
	trackers.objects={};
	count = detect(frame);
	if(oldcount != count)
	  {
	    cout << count << endl;
	  }
	oldcount = count;
	trackers.add(frame,objects);
      }
    else
      {
	trackers.update(frame);
	for (unsigned int j =0; j < trackers.objects.size(); j++)
	  {
	    rectangle(frame,trackers.objects[j], Scalar(255,0,0),2,1);
	  }
	imshow("video",frame);
      }
    key = cvWaitKey(33);
    if(key == KEY_SPACE)
      key = cvWaitKey(0);
    if(key == KEY_ESC || key == (int) 'q')
      break;
  }

  return 0;
}

int detect(Mat img)
{
  int count=0;
  Mat A, B, C;
  C= Mat::zeros(img.size(), 0);
  vector<Rect> cars;
  cascade.detectMultiScale(img,cars,1.15,2,0,Size(50,50));

  /*for (int x = 0; x  < points.size(); x+=4)
    {
      A= Mat::zeros(img.size(), 0);
      Point pts [] = {points.at(x),points.at(x+1),points.at(x+2),points.at(x+3)};
      fillConvexPoly(img,pts,4,Scalar(0,0,150));
      fillConvexPoly(A,pts,4,Scalar(255));
      for(int i=0; i < cars.size(); i++)
	{
	  B= Mat::zeros(img.size(), 0);
	  rectangle(B,cars.at(i),Scalar(255),CV_FILLED);
	  bitwise_and(A,B,C);
	  if (countNonZero(C) / (float)countNonZero(B) > .6)
	    count++;
	    }
	    }*/
  
  for(int i=0; i < cars.size(); i++)
    {
      rectangle(img,cars.at(i),Scalar(0,255,0),1);
      objects.push_back(cars.at(i));
    }
  imshow("video",img);
  return count;
}
