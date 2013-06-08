#include <iostream>	// for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "CameraRemoteProxy.h"
#include "CameraLocalProxy.h"
#include "MyLutColorFilter.h"
#include "MyFsmColorFilter.h"
#include "StdoutLogger.h"

#include "TimeMeasurement.h"

const int tm_filter_lut = 1;
const int tm_filter_fsm = 2;

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

	namedWindow("Input", CV_WINDOW_AUTOSIZE);
	//namedWindow("Output LUT", CV_WINDOW_AUTOSIZE);
	namedWindow("Output FSM", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat dstFsm(480,640,CV_8UC1);
	Mat visFsm(480,640,CV_8UC3);
	Mat dstLut(480,640,CV_8UC1);
	Mat visLut(480,640,CV_8UC3);

	vector<Rect> bbVector;

	MyLutColorFilter *filterLut = new MyLutColorFilter();
	//filterLut->ColorCodeToFind=COLORCODE_BLK;
	//filter0->DetectionMask = &mask;

	MyFsmColorFilter *filterFsm = new MyFsmColorFilter();
	
	while(true) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);

		bbVector.clear();

/*		timeMeasurement.start(tm_filter_lut);
		filter0->Filter(&src,&dst0,&bbVector0);
		timeMeasurement.finish(tm_filter_lut); */

		timeMeasurement.start(tm_filter_fsm);
		filterFsm->Filter(&src,&dstFsm,&bbVector);	// Creates debug output
		//filterFsm->Filter(&src,NULL,&bbVector);	// Does not create debug output, only bounding boxes
		timeMeasurement.finish(tm_filter_fsm);

		// For all bounding boxes returned by FSM, execute LutColorFilter

		// Remark: this is unnecessary if we only look at the areas defined by the bounding boxes...
		dstLut.setTo(0);	// Only for nicer visualization...
		timeMeasurement.start(tm_filter_lut);
		for(int i=0; i<bbVector.size(); i++)
		{
			Rect &rect = bbVector[i];
			filterLut->FilterRoI(src,rect,dstLut);
		}
		timeMeasurement.finish(tm_filter_lut);

		filterFsm->InverseLut(dstFsm,visFsm);
		filterLut->InverseLut(dstLut,visLut);

		// Show bounding boxes on source image
		int size = bbVector.size();
		char txt[100];
		sprintf(txt,"FsmBBNum=%d",size);
		putText( src, string(txt), cvPoint(25,20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,0) );
		filterFsm->ShowBoundingBoxes(src,Scalar(0,255,0));

		imshow("Input",src);
		imshow("Output LUT",visLut);
		imshow("Output FSM",visFsm);

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
