/*
This program is used to detect motorcycles on a designated area of the screen.
Drag the mouse to designate the region of interest to detect motorcycles.
Program designed to have region of interest be area closer to camera, to count
vehicles/motorcycles coming closer.

*/

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/utility.hpp>

#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

const int KEY_SPACE = 32;
const int KEY_ESC = 27;

//factors by which image is scaled down to improve speed.
const double X_FACTOR =.4L;
const double Y_FACTOR =.4L;

// parameters for motorcycle detection. MAX_WIDTH prevents larger objects being detected.
// MIN_HEIGHT specifies minimum height for motorcycle to be registered. 
const int MAX_WIDTH = 20;
const int MAX_HEIGHT = 40;
const int MIN_HEIGHT = 14;
const int MIN_WIDTH = 9;

//Detects cars every __ frames. On other frames, tracks.
const int DETECT_RATE = 6;

RNG rng(12345);

//Objects and variables for designating ROI
Rect cropRect(0,0,0,0);
Point P1(0,0);
Point P2(0,0);
bool clicked = false;

//tracking
const string trackingalg = "MEDIANFLOW";
Ptr<MultiTracker> trackers = makePtr<MultiTracker>(trackingalg);
vector <Rect2d> objects;
//for counting
vector <bool> passedVehicles;
vector <bool> previousVehicles;

//Method prototypes
void Erosion(Mat &img);
void Contours(Mat &img, Mat &original);
void Dilation(Mat &img);
void onMouse(int event, int x, int y, int f, void*);

int main(int argc, char **arv)
{
  cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;

  namedWindow("Frame", 1);
  namedWindow("Mask", 1);
  namedWindow("Region of Interest",1);
    
  Mat frame, fgMask, frame_gray, original;

  //VideoCapture cap("https://d1h84if288zv9w.cloudfront.net/7dias/0be3_408.stream/playlist.m3u8");
  VideoCapture cap("motorcycle.avi");
  
  Ptr<BackgroundSubtractor> subtractor;
  subtractor = createBackgroundSubtractorMOG2();
  
  if(!cap.isOpened())
    {
      cout << "video error " << endl;
      return -1;
    }

  //only for test sequences. Gets rid of initial watermarks.
  for(int i =0; i <500; i++)
    {
      cap >> frame;
    }

  imshow("Region of Interest", frame);
  setMouseCallback("Region of Interest",onMouse,NULL);

  //wait for user to input region of interest by mouse
  for(;;)
    {
      char c = waitKey();
      if (c == 27) break;
    }

  Mat ROI = frame(cropRect);
  
  //show just the area selected.
  imshow("Region of Interest",ROI);
  
  char key = '\0';

  //number of vehicles/motorcycles that have passed.
  int count =0;
  for(unsigned long i = 0;;i++)
    {
      cap >> original;

      if (original.empty() || original.rows == 0 || original.cols == 0)
	break;
      
      ROI = original(cropRect);
      cv::resize(ROI,frame,Size(),X_FACTOR,Y_FACTOR);

      if (i% DETECT_RATE ==0)
	{
	  //detection using background subtraction

	  //reinitialize tracked objects
	  passedVehicles.clear();
	  previousVehicles.clear();
	  objects.clear();
	  trackers.release();
	  trackers = makePtr<MultiTracker>(trackingalg);
	  
	  subtractor->apply(frame,fgMask,.002);
	  blur(fgMask,fgMask,Size(3,3));
	  
	  //preprocessing to reduce noise
	  threshold(fgMask,fgMask,128,255,THRESH_BINARY);
	  Erosion(fgMask);
	  Dilation(fgMask);
      
	  Contours(fgMask, original);

	  //MAY WANT TO USE ORIGINAL NOT FRAME? FRAME IS SMALLER SO BETTER PERFORMANCE...
	  trackers->add(frame,objects);

	  cout << count << endl;

	  imshow("Mask",fgMask);
      
	  imshow("frame", original); 
	}
      else
	{
	  //tracking using KCF on previously detected motorcycles
	  cap >> original;
	  if (original.empty() || original.rows == 0 || original.cols == 0)
	    break;
	  
	  ROI = original(cropRect);
	  cv::resize(ROI,frame,Size(),X_FACTOR,Y_FACTOR);
	  
	  subtractor->apply(frame,fgMask,.005);

	  //before updating trackers, check if each before or after line. Store in bool array...
	  //if changes after update, increase car/motorcycle count.
	  previousVehicles = passedVehicles;

	  trackers->update(frame);

	  passedVehicles.clear();
	  
	  //drawing tracking locations, also cropping tracking locations to ensure validity
	  Rect location;
	  for (unsigned int j =0; j < trackers->objects.size(); j++)
	    {
	      trackers->objects[j] = trackers->objects[j] & cv::Rect2d(0,0,frame.cols, frame.rows);

	      if(trackers->objects[j].y > frame.rows/2)
		{
		  passedVehicles.push_back(true);

		  if( !previousVehicles.at(j) )
		    {
		      count++;
		    }
		}
	      else
		{
		  passedVehicles.push_back(false);
		}


	      //resizign and relocating for displaying
	      location.width = (int) trackers->objects[j].width / X_FACTOR;
	      location.height = (int) trackers->objects[j].height / Y_FACTOR;
	      location.x = trackers->objects[j].x / X_FACTOR;
	      location.y = trackers->objects[j].y / Y_FACTOR;
	      location.x += cropRect.x;
	      location.y += cropRect.y;
	      location = location & cv::Rect(0,0,original.cols, original.rows);

	      //drawing tracked rectangle on original image
	      rectangle(original, location, Scalar(255,0,0),2,1);
	    }
	  
	  imshow("frame",original); 
	  
	}

      //space for pause, escape or q to quit
      key = waitKey(1);
      if(key == KEY_SPACE)
	key = waitKey(0);
      if(key == KEY_ESC || key == 'q')
	break;
      
    }

  trackers.release();
  cap.release();
  cvDestroyAllWindows();
  return 0;
}

void Erosion(Mat &img)
{
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(3,3),Point(1,1));
  erode(img,img,element);
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

  for(int i =0; i < contours.size(); i++ )
    {
      Scalar color = Scalar( rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255) );
      if (arcLength(contours.at(i),true) > 50)
	{
	  //draw rectangle onto x
	  
	  ROI = boundingRect(contours.at(i));

	  //only choosing boxes of desired size. Can fine tune for motorcycles
	  if (ROI.width < MAX_WIDTH && ROI.height > MIN_HEIGHT)
	    {
	      //trim ROI so it fits in original, then run detector.
	      ROI = ROI & cv::Rect(0,0,img.cols, img.rows);

	      //for tracking
	      objects.push_back(ROI);

	      //checking if there is a vehicle above or below the zone to pass.
	      if (ROI.y > img.rows/2)
		{
		  passedVehicles.push_back(true);
		}
	      else
		{
		  passedVehicles.push_back(false);
		}

	      //scale ROI back up to original size to draw rectangle
	      ROI.width = (int) ROI.width / X_FACTOR;
	      ROI.height = (int) ROI.height / Y_FACTOR;
	      ROI.x /= X_FACTOR;
	      ROI.y /= Y_FACTOR;
	      ROI.x += cropRect.x;
	      ROI.y += cropRect.y;

	      //Draw rectangle onto original image
	      ROI = ROI & cv::Rect(0,0,original.cols, original.rows);
	      rectangle(original, ROI, color);	      
	    }
	}
    }
}

//for drawing the desired area on the image.
void onMouse(int event, int x, int y, int f,  void*)
{
  switch(event)
    {
    case CV_EVENT_LBUTTONDOWN :
      clicked = true;
      P1.x = x;
      P1.y = y;
      P2.x = x;
      P2.y = y;
      break;
    case CV_EVENT_LBUTTONUP :
      clicked = false;
      P2.x = x;
      P2.y = y;
      break;
    case CV_EVENT_MOUSEMOVE :
      if (clicked)
	{
	  P2.x = x;
	  P2.y = y;
	}
      break;
    default : break;
    }
  if (clicked)
    {
      if (P1.x > P2.x)
	{
	  cropRect.x = P2.x;
	  cropRect.width = P1.x - P2.x;
	}
      else
	{
	  cropRect.x = P1.x;
	  cropRect.width = P2.x - P1.x;
	}
      if (P1.y > P2.y)
	{
	  cropRect.y = P2.y;
	  cropRect.height = P1.y - P2.y;
	}
      else
	{
	  cropRect.y = P1.y;
	  cropRect.height = P2.y - P1.y;
	}
      
    }
}

