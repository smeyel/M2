#include <iostream>	// for standard I/O
#include <fstream>
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

#include "MyLutColorFilter.h"

#include "TimeMeasurement.h"

#include "FsmLearner.h"

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
int test_FSM()
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
	//MyLutFsmLocator *locator = new MyLutFsmLocator();
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

void feedImageIntoTransitionStat(TransitionStat *stat, Mat &image, bool isOn)
{
	// Assert for only 8UC1 output images
	OPENCV_ASSERT(image.type() == CV_8UC1,"feedImageIntoTransitionStat","Image type is not CV_8UC1");

	// Go along every pixel and do the following:
	for (int row=0; row<image.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(image.data + row*image.step);

		// Go along every BGR colorspace pixel
		for (int col=0; col<image.cols; col++)
		{
			unsigned char value = *ptr++;
			stat->addValue(value,isOn);
		}	// end for col
	}	// end for row
}


void test_mkStatFromImage()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	Size S = Size(640,480);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);
	namedWindow("LUT", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat lut(480,640,CV_8UC1);
	Mat visLut(480,640,CV_8UC3);

	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();

	TransitionStat *stat = new TransitionStat(7,3,COLORCODE_NONE);

	int filenameCounter=0;
	bool running=true;
	while(running) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);

		lutColorFilter->Filter(&src,&lut,NULL);
		lutColorFilter->InverseLut(lut,visLut);

		feedImageIntoTransitionStat(stat,lut,false);

		imshow("SRC",src);
		imshow("LUT",visLut);
		//imshow("FSM",visFsm);

		char ch = waitKey(25);
		switch(ch)
		{
		case 27:
			running = false;
			break;
		case 's':	// Save current frame
			char filename[200];
			sprintf(filename,"frame%d.jpg",filenameCounter);
			imwrite(filename,src);
			filenameCounter++;
			break;
		}
	}

	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_OFF);
	stat->counterTreeRoot->showCompactRecursive(0,0);

	char ch = waitKey(-1);
}

void test_FsmLearner()
{
	TransitionStat *stat = new TransitionStat(6,3,0);

	unsigned int inputValues[10] = {1, 2, 3, 1, 2, 3};

	stat->startNewSequence();
	cout << "--- Initial tree:" << endl;
	stat->counterTreeRoot->showRecursive(0,1,false);
	cout << "Adding values..." << endl;
	for(int i=0; i<6; i++)
	{
		stat->addValue(inputValues[i],false);
	}
	cout << "--- Result tree:" << endl;
	stat->counterTreeRoot->showRecursive(0,0,false);

	cout << "Summing up child counters..." << endl;
	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_OFF);

	cout << "--- Result tree:" << endl;
	stat->counterTreeRoot->showRecursive(0,0,false);
}

void test_frameSaver()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);

	int filenameCounter=0;
	bool running=true;
	while(running) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);

		imshow("SRC",src);

		char ch = waitKey(25);
		switch(ch)
		{
		case 27:
			running = false;
			break;
		case 's':	// Save current frame
			char filename[200];
			sprintf(filename,"frame%d.jpg",filenameCounter);
			imwrite(filename,src);
			filenameCounter++;
			break;
		}
	}
}

void processImages(istream *filenameListFile, bool isOn, LutColorFilter *filter, TransitionStat *stat)
{
	Mat src(480,640,CV_8UC3);

	char filename[256];
	while (filenameListFile->getline(filename,256)) //Show the image captured in the window and repeat
	{
		cout << "Processing file: " << filename << endl;
		VideoInput *input = VideoInputFactory::CreateVideoInput(VIDEOINPUTTYPE_GENERIC);
		input->init(filename);
		input->captureFrame(src);

		Mat lut(src.rows,src.cols,CV_8UC1);
		Mat visLut(src.rows,src.cols,CV_8UC3);

		filter->Filter(&src,&lut,NULL);
		filter->InverseLut(lut,visLut);

		feedImageIntoTransitionStat(stat,lut,isOn);

/*		imshow("SRC",src);
		imshow("LUT",visLut);*/

/*		cout << "press a key" << endl;
		waitKey(0);*/
	}
}

void test_mkStatFromImageList(const char *offImageFilenameList, const char *onImageFilenameList)
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);
	namedWindow("LUT", CV_WINDOW_AUTOSIZE);

	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();

	TransitionStat *stat = new TransitionStat(7,3,COLORCODE_NONE);

	std::filebuf fileBuff;
	if (fileBuff.open(offImageFilenameList,std::ios::in))
	{
		std::istream istrm(&fileBuff);
		processImages(&istrm,false,lutColorFilter,stat);
		fileBuff.close();
	}
	if (fileBuff.open(onImageFilenameList,std::ios::in))
	{
		std::istream istrm(&fileBuff);
		processImages(&istrm,true,lutColorFilter,stat);
		fileBuff.close();
	}

	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_OFF);
	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_ON);

	float offSum = stat->counterTreeRoot->getCounter(COUNTERIDX_OFF);
	float onSum = stat->counterTreeRoot->getCounter(COUNTERIDX_ON);

	stat->counterTreeRoot->divAllCounters(COUNTERIDX_OFF, offSum);
	stat->counterTreeRoot->divAllCounters(COUNTERIDX_ON, onSum);

	stat->counterTreeRoot->showCompactRecursive(0,1);

	char ch = waitKey(-1);
}


int main(int argc, char *argv[], char *window_name)
{
	//test_FSM();
	//test_FsmLearner();
	//test_mkStatFromImage();
	//test_frameSaver();
	test_mkStatFromImageList("off_list.txt","on_list.txt");
}
