#include <iostream>	// for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

//#include "CameraRemoteProxy.h"
#include "CameraLocalProxy.h"
//#include "MyLutColorFilter.h"
//#include "MyFsmColorFilter.h"
#include "StdoutLogger.h"

#include "MyLutFsmLocator.h"
#include "MyLocator2.h"

#include "TimeMeasurement.h"

const int tm_filter_lut = 1;
const int tm_filter_fsm = 2;

using namespace std;
using namespace cv;
using namespace smeyel;
using namespace LogConfigTime;

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
	TimeMeasurement timeMeasurement;
	timeMeasurement.init();
	timeMeasurement.setMeasurementName("Filter execution times");
	timeMeasurement.setname(tm_filter_lut,"LUT");
	timeMeasurement.setname(tm_filter_fsm,"FSM");

	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	Size S = Size(640,480);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);
	namedWindow("LUT", CV_WINDOW_AUTOSIZE);
	namedWindow("FSM", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat visFsm(480,640,CV_8UC3);
	Mat visLut(480,640,CV_8UC3);

	MyLocator2 *locator = new MyLocator2();
	locator->verboseFsmState = &visFsm;
	locator->verboseLutImage = &visLut;
	locator->showBoundingBoxesOnSrc=true;
	locator->cleanVerboseImage=true;
	locator->overrideFullLut = true;

	while(true) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);

		timeMeasurement.start(tm_filter_fsm);
		locator->processImage(src);
		timeMeasurement.finish(tm_filter_fsm);

		imshow("SRC",src);
		imshow("LUT",visLut);
		imshow("FSM",visFsm);

		char ch = waitKey(25);
		if (ch==27)
		{
			break;
		}
	}

	timeMeasurement.showresults();
	char ch = waitKey(-1);

	return 0;
}
