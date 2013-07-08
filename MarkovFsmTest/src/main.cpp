#include <iostream>	// for standard I/O
#include <fstream>
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "CameraLocalProxy.h"
#include "StdoutLogger.h"

#include "MyLutColorFilter.h"

//#include "TimeMeasurement.h"

#include "TransitionStat.h"

#include "fsmlearning.h"

using namespace std;
using namespace cv;
using namespace smeyel;
using namespace LogConfigTime;



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

		stat->addImage(lut,isOn);
	}
}

bool callbackVerbose=true;
void callback(SequenceCounterTreeNode *node, float precision)
{
	if(callbackVerbose)
		cout << "----- Promising node found: " << endl;
	// set score of node
	node->auxScore = (unsigned char)((precision-0.9F)*2550.0F);
	
	if(callbackVerbose)
		cout << "Precision, Score: " << precision << ", " << (int)node->auxScore << endl;

/*	if(callbackVerbose)
		cout << "Input sequence (reverse order!!!):" << endl;
	SequenceCounterTreeNode *current = node;
	SequenceCounterTreeNode *parent = node->getParentNode();
	while(parent)
	{
		if(callbackVerbose)
			cout << parent->getInputValueForChild(current) << " ";
		current = parent;
		parent = current->getParentNode();
	} */
	if(callbackVerbose)
	{
		cout << "(root)" << endl;
		cout << "Subtree:" << endl;
		node->showCompactRecursive(0,1);
	}
}

void test_mkStatFromImageList(const char *offImageFilenameList, const char *onImageFilenameList)
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();

	TransitionStat *stat = new TransitionStat(8,3,COLORCODE_NONE);

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

	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);
	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);

	stat->counterTreeRoot->showCompactRecursive(0,1);

	stat->findClassifierSequences(callback);

	// -------------- Now start camera and apply statistics (auxScore mask) to the frames
	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);
	namedWindow("LUT", CV_WINDOW_AUTOSIZE);
	namedWindow("Score", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat lut(480,640,CV_8UC1);
	Mat score(480,640,CV_8UC1);
	Mat visLut(480,640,CV_8UC3);

	while(true) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);
		
		lutColorFilter->Filter(&src,&lut,NULL);
		lutColorFilter->InverseLut(lut,visLut);

		stat->getScoreMaskForImage(lut,score);

		imshow("SRC",src);
		imshow("LUT",visLut);
		imshow("Score",score);

		char ch = waitKey(25);
		if (ch==27)
		{
			break;
		}
	}
}

void test_singleFeed()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);
	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();
	TransitionStat *stat = new TransitionStat(8,3,COLORCODE_NONE);

	Mat src(50,50,CV_8UC3);
	Mat lut(50,50,CV_8UC1);
	Mat lutVis(50,50,CV_8UC3);
	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,49),Scalar(255,0,0));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	waitKey(0);
	stat->addImage(lut, true);

	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,49),Scalar(0,0,255));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	waitKey(0);
	stat->addImage(lut, false);

	stat->counterTreeRoot->showCompactRecursive(0,1);
	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);
	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);

	stat->findClassifierSequences(callback);

	cout << "done" << endl;
}

void test_mkStatInteractive()
{
	const int markovChainOrder = 10;
	if (markovChainOrder>4)
		callbackVerbose=false;

	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();

	TransitionStat *stat = new TransitionStat(8,markovChainOrder,COLORCODE_NONE);

	CameraLocalProxy *camProxy0 = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	camProxy0->getVideoInput()->SetNormalizedExposure(-1);
	camProxy0->getVideoInput()->SetNormalizedGain(-1);
	camProxy0->getVideoInput()->SetNormalizedWhiteBalance(-1,-1,-1);

	namedWindow("SRC", CV_WINDOW_AUTOSIZE);
	namedWindow("LUT", CV_WINDOW_AUTOSIZE);
	namedWindow("Score", CV_WINDOW_AUTOSIZE);

	Mat src(480,640,CV_8UC3);
	Mat lut(480,640,CV_8UC1);
	Mat score(480,640,CV_8UC1);
	Mat visLut(480,640,CV_8UC3);

	bool running = true;
	while(running) //Show the image captured in the window and repeat
	{
		camProxy0->CaptureImage(0,&src);
		
		lutColorFilter->Filter(&src,&lut,NULL);
		lutColorFilter->InverseLut(lut,visLut);

		stat->getScoreMaskForImage(lut,score);

		imshow("SRC",src);
		imshow("LUT",visLut);
		imshow("Score",score);

		char ch = waitKey(25);
		switch(ch)
		{
		case 27:
			running=false;
			cout << "Exiting" << endl;
			break;
		case '+':	// on
			stat->addImage(lut,true);
			cout << "Saved ON" << endl;
			break;
		case '-':	// off
			stat->addImage(lut,false);
			cout << "Saved OFF" << endl;
			break;
		case 'l':	// learn
			stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);
			stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);
			if (markovChainOrder<4)
				stat->counterTreeRoot->showCompactRecursive(0,1);
			stat->findClassifierSequences(callback);
			cout << "Trained" << endl;
			break;
		case 'r':	// reset
			delete stat;
			stat = new TransitionStat(8,markovChainOrder,COLORCODE_NONE);
			cout << "TransitionStat reset" << endl;
			break;
		}
	}
}

int main(int argc, char *argv[], char *window_name)
{
	//test_frameSaver();
	//test_mkStatFromImageList("off_list.txt","on_list.txt");
	//test_mkStatInteractive();
	
	//test_singleFeed();
	test_graphOpt();
}
