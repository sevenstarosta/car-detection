#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <vector>
#include <string>

using namespace cv;
using namespace std;

int main(int argc, char **argv)
{
  string xmlname="myxml.xml"; 
  if (argc >= 2)
    {
      xmlname = argv[2];
    }
  FileStorage fs(xmlname, FileStorage::READ);
  FileNode root = fs.getFirstTopLevelNode();
  Point pt(-1,-1);
  vector <Point> points = {};
  int size = (int) root.size();
  cout << root.name() << endl;
  for(int x = 0; x < size; x++)
    {
      //gives polygons.
      FileNode polygon = root["polygon"+to_string(x)];
      cout << polygon.name() << endl;
      for (int y = 0; y <4; y++)
	{
	  pt.x = double(polygon["pointx"+to_string(y)]);
	  pt.y = double(polygon["pointy"+to_string(y)]);
	  points.push_back(pt);
	}
    }

  
  size = points.size();
  for(int x = 0; x < size; x++)
    {
      cout << points.at(x).x << "  " << points.at(x).y << endl; 
    }
  return 0;
}
