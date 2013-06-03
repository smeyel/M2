#include <iostream>	// for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "CameraRemoteProxy.h"
#include "CameraLocalProxy.h"
#include "MyLutColorFilter.h"
#include "MyFsmColorFilter.h"
#include "StdoutLogger.h"

using namespace std;
using namespace cv;
using namespace smeyel;

void help()
{
	cout
		<< "\n--------------------------------------------------------------------------" << endl
		<< "This program captures video from local camera to file."
		<< "--------------------------------------------------------------------------"   << endl
		<< endl;
}

// Meant to record video from multiple cameras. Later, should be able to use CameraProxy and save images with timestamp for proper re-playing.
// Will support even multiple local and/or multiple remote cameras.
// Later todo: make a component like logging where the measurement host can push the just retrieved images and their timestamps.
// Record to multiple files by first capturing into memory and then saving into AVI at once. (Pre-allocate many buffer Mat-s)
int main(int argc, char *argv[], char *window_name)
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	Size S = Size(640,480);

	namedWindow("Input", CV_WINDOW_AUTOSIZE);
	namedWindow("Output LUT", CV_WINDOW_AUTOSIZE);
	namedWindow("Output FSM", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat dst0(480,640,CV_8UC1);
	Mat vis0(480,640,CV_8UC3);
	Mat dst1(480,640,CV_8UC1);
	Mat vis1(480,640,CV_8UC3);
	Mat mask(480,640,CV_8UC1);

	MyLutColorFilter *filter0 = new MyLutColorFilter();
	filter0->ColorCodeToFind=COLORCODE_BLK;
	filter0->DetectionMask = &mask;
	vector<Rect> bbVector0;

	MyFsmColorFilter *filter1 = new MyFsmColorFilter();
	filter1->DetectionMask = &mask;
	vector<Rect> bbVector1;

	while(true) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);

		bbVector0.clear();
		bbVector1.clear();
		filter0->Filter(&src,&dst0,&bbVector0);
		filter1->Filter(&src,&dst1,&bbVector1);

		filter0->InverseLut(dst0,vis0);
		filter1->InverseLut(dst1,vis1);

		imshow("Input",src);
		imshow("Output LUT",vis0);
		imshow("Output FSM",vis1);

		// Show bounding box number
		cout << "BB_Num: " << bbVector0.size() << ", " << bbVector1.size() << endl;

		char ch = waitKey(25);
		if (ch==27)
		{
			break;
		}
	}
	return 0;
}
