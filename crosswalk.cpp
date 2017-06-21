#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <opencv2/videoio.hpp>

using namespace cv;
using namespace std;

Point pt(-1,-1);
bool newclick = false;

void callback(int event, int x, int y, int flag, void * param)
{
  if(event == EVENT_LBUTTONDOWN)
    {
      pt.x = x;
      pt.y = y;
      newclick = true;
    }
}

int main(int argc, char **argv)
{
  string url = "https://d1h84if288zv9w.cloudfront.net/7dias/0be3_408.stream/playlist.m3u8";
    //"http://devimages.apple.com/iphone/samples/bipbop/gear1/prog_index.m3u8?dummy=param.mjpg";
  string xmlname = "myxml.xml";
  Mat img;
  //  Dealing with parameters: if only one, assume to be url of stream. If two, url of stream and name of xml
  if(argc >= 2)
    {
      //cout << "Include url for videostream." << endl;
      url = argv[1];
    }
  VideoCapture cap(url);
  if(!cap.isOpened())
    {
      cout << "Video stream non-functional." << endl;
      return 1;
    }
  if (argc >= 3)
    {
      xmlname = argv[2];
    }
  FileStorage fs(xmlname, FileStorage::WRITE);

  cap >> img;
  vector<vector<Point>> points;
  namedWindow("window",1);
  setMouseCallback("window",callback);
  imshow("window",img);
  int numpts =0;
  cout << "press q when ready to save to xml" << endl;
  
  for(;;)
  {
    
    if (newclick == true && ((numpts%4) != 0))
      {
	numpts++;
	cout << "registered click" << numpts << endl;
	points.back().push_back(pt);
	newclick = false;
	if ((numpts % 4) ==0)
	  {
	    cout << "polygon registered" << endl;
	    int x = points.size()-1;
	    Point pts [] = {points.at(x).at(0),points.at(x).at(1),points.at(x).at(2),points.at(x).at(3)};
	    fillConvexPoly(img,pts,4,Scalar(0,255,0));
	    imshow("window",img);
	  }
      }
    else if (newclick == true)
      {
	numpts++;
	cout << "registered click" << numpts << endl;
	points.push_back((vector<Point>){});
	points.back().push_back(pt);
	newclick = false;
      }
    if ((waitKey(5) & 0xFF) == 'q')
      break;
  }
  
  if ((numpts % 4) == 0 && (numpts >0))
    {

      fs << "crosswalk" << "{";
      for(int y =0;y < points.size(); y++)
	{
	  fs << "polygon" + to_string(y) << "{";
	  for(int x = 0;x < 4; x++)
	    {
	      write(fs,"pointx" + to_string(x),points.at(y).at(x).x);
	      write(fs,"pointy" + to_string(x),points.at(y).at(x).y);
	    }
	  fs << "}";
	}
      fs << "}";
      fs.release();
    }
  return 0;
}
