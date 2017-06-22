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
const int MAX_WIDTH = 23;
const int MAX_HEIGHT = 35;
const int MIN_HEIGHT = 10;
const int MIN_WIDTH = 8;

//Detects cars every __ frames. On other frames, tracks.
const int DETECT_RATE = 5;

RNG rng(12345);

//Objects and variables for initial pespective transform
Point2f source_points[] = {Point2f(0,0),Point2f(0,0),Point2f(0,0),Point2f(0,0)};
Point2f dst_points[] = {Point2f(0,0), Point2f(800,0),Point2f(800,800),Point2f(0,800)};

//
vector <bool> passedVehicles;
vector <bool> previousVehicles;

int pointnumber = 0;

//tracking
const string trackingalg = "MEDIANFLOW";
Ptr<MultiTracker> trackers = makePtr<MultiTracker>(trackingalg);
vector <Rect2d> objects;

//Method prototypes
void Erosion(Mat &img,int radius);
void Contours(Mat &img, Mat &original);
void Dilation(Mat &img,int radius);
void onMouse(int event, int x, int y, int f, void*);

int main(int argc, char **arv)
{
  cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;

  clock_t start;
  double duration;
  
  namedWindow("Frame", 1);
  namedWindow("Mask", 1);
  namedWindow("Perspective transform",1);
    
  Mat frame, fgMask, frame_gray, original, dst;

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

  imshow("Perspective transform", frame);
  setMouseCallback("Perspective transform",onMouse,NULL);
  
  while(pointnumber < 4 )
    {
      char c = waitKey(1);
      if (c == KEY_ESC) break;
    }

  Mat transform_matrix = getPerspectiveTransform(source_points,dst_points);
  warpPerspective(frame,dst,transform_matrix,Size(800,800));
  imshow("Perspective transform",dst);

  
  char key = '\0';

  //number of vehicles/motorcycles that have passed.
  int count = 0;
  int leftcount = 0;
  int rightcount = 0;

  start = clock();
  unsigned long i = 0;

  //allow background to initialize before starting detection
  for (;i<150;i++)
  {
    cap >> original;

    if (original.empty() || original.rows == 0 || original.cols == 0)
      break;
      
    warpPerspective(original,dst,transform_matrix,Size(800,800));
      
    cv::resize(dst,frame,Size(),X_FACTOR,Y_FACTOR);

    subtractor->apply(frame,fgMask,.002);

    	  Dilation(fgMask,1);
	  blur(fgMask,fgMask,Size(3,3));

	  threshold(fgMask,fgMask,130,255,THRESH_BINARY);
	  Erosion(fgMask,1);
	  Dilation(fgMask,2);

  }

  i = 0;
  for(;;i++)
    {
      cap >> original;

      if (original.empty() || original.rows == 0 || original.cols == 0)
	break;

      warpPerspective(original,dst,transform_matrix,Size(800,800));
      
      cv::resize(dst,frame,Size(),X_FACTOR,Y_FACTOR);

      subtractor->apply(frame,fgMask,.002);

      previousVehicles = passedVehicles;

      trackers->update(frame);

      passedVehicles.clear();
      
      Rect location;
      
      for (unsigned int j =0; j < trackers->objects.size(); j++)
	{
	  trackers->objects[j] = trackers->objects[j] & cv::Rect2d(0,0,frame.cols, frame.rows);
	  
	  if(trackers->objects[j].y >= frame.rows/2)
	    {
	      passedVehicles.push_back(true);
	      for (int k = previousVehicles.size(); k<= j; k++)
		previousVehicles.push_back(true);
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
	  location = location & cv::Rect(0,0,original.cols, original.rows);
	  
	  //drawing tracked rectangle on original image
	  rectangle(original, location, Scalar(255,0,0),2,1);
	}

      if (i% DETECT_RATE ==0)
	{
	  //detection using background subtraction

	  //reinitialize tracked objects
	  
	  previousVehicles.clear();
	  objects.clear();
	  trackers.release();
	  trackers = makePtr<MultiTracker>(trackingalg);

	  Dilation(fgMask,1);
	  blur(fgMask,fgMask,Size(3,3));

	  threshold(fgMask,fgMask,130,255,THRESH_BINARY);
	  Erosion(fgMask,1);
	  Dilation(fgMask,2);

	  Contours(fgMask, original);
	  
	  trackers->add(frame,objects);

	  cout << count << endl;


	}

      imshow("Mask",fgMask);

      imshow("frame",frame); 
	  
      //space for pause, escape or q to quit
      key = waitKey(1);
      if(key == KEY_SPACE)
	key = waitKey(0);
      if(key == KEY_ESC || key == 'q')
	break;
      
    }

  //going by clock rate
  //duration = (clock() - start) / (double) CLOCKS_PER_SEC;

  //going by frame rate
  duration = i / (double) cap.get(CV_CAP_PROP_FPS);
  cout << 3600 * count / duration << endl;
  trackers.release();
  cap.release();
  cvDestroyAllWindows();
  return 0;
}

void Erosion(Mat &img, int radius)
{
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(2*radius+1,2*radius+1),Point(radius,radius));
  erode(img,img,element);
}

void Dilation(Mat &img, int radius)
{
  Mat element = getStructuringElement(MORPH_ELLIPSE,Size(2*radius+1,2*radius+1),Point(radius,radius));
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
	  if (ROI.width < MAX_WIDTH && ROI.height > MIN_HEIGHT && ROI.width > MIN_WIDTH && ROI.height < MAX_HEIGHT && ROI.width < ROI.height)
	    {
	      //trim ROI so it fits in original, then run detector.
	      ROI = ROI & cv::Rect(0,0,img.cols, img.rows);

	      //for tracking
	      objects.push_back(ROI);

	      //checking if there is a vehicle above or below the zone to pass.
	      /*if (ROI.y >= img.rows/2)
		{
		  passedVehicles.push_back(true);
		}
	      else
		{
		  passedVehicles.push_back(false);
		}*/
	      //scale ROI back up to original size to draw rectangle
	      ROI.width = (int) ROI.width / X_FACTOR;
	      ROI.height = (int) ROI.height / Y_FACTOR;
	      ROI.x /= X_FACTOR;
	      ROI.y /= Y_FACTOR;
	      //ROI.x += cropRect.x;
	      //ROI.y += cropRect.y;

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
  if (event == CV_EVENT_LBUTTONDOWN)
    {
      source_points[pointnumber].x = x;
      source_points[pointnumber].y = y;
      pointnumber++;
    }
}



