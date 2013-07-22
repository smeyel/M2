#include <iostream>	// for standard I/O
#include <fstream>

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "CameraLocalProxy.h"

#include "MyLutColorFilter.h"
#include "StdoutLogger.h"

using namespace cv;
using namespace LogConfigTime;
using namespace smeyel;

#include "FsmLearner.h"

void test_graphOpt()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);
	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();
	FsmLearner *fsmlearner = new FsmLearner(8,3,COLORCODE_NONE);

	vector<string> inputValueNames(7);
	inputValueNames[COLORCODE_BLK]=string("BLK");
	inputValueNames[COLORCODE_WHT]=string("WHT");
	inputValueNames[COLORCODE_RED]=string("RED");
	inputValueNames[COLORCODE_GRN]=string("GRN");
	inputValueNames[COLORCODE_BLU]=string("BLU");
	inputValueNames[COLORCODE_NONE]=string("NON");

	Mat src(50,50,CV_8UC3);
	Mat lut(50,50,CV_8UC1);
	Mat lutVis(50,50,CV_8UC3);
	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,25),Scalar(255,0,0));
	rectangle(src,Point2d(25,26),Point2d(30,49),Scalar(0,255,0));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	//waitKey(0);
	fsmlearner->addImage(lut, true);

	//cout << "------------- ON train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,49),Scalar(0,0,255));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	//waitKey(0);
	fsmlearner->addImage(lut, false);

	//cout << "------------- OFF train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	// calculateSubtreeCounters
	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);
	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);

	// Set precisions
	fsmlearner->setPrecisionStatus(fsmlearner->counterTreeRoot,0.7F);

	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	// cut
	cout << "------------- cut -------------" << endl;
	fsmlearner->counterTreeRoot->cut(0);

	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	cout << "------------- merge -------------" << endl;
	fsmlearner->mergeNodesForPrecision(&inputValueNames);

	cout << "------------- combining nodes... -------------" << endl;
	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);



	cout << "done" << endl;

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

void test_mkStatFromImageList(const char *offImageFilenameList, const char *onImageFilenameList)
{
	vector<string> inputValueNames(7);
	inputValueNames[COLORCODE_BLK]=string("BLK");
	inputValueNames[COLORCODE_WHT]=string("WHT");
	inputValueNames[COLORCODE_RED]=string("RED");
	inputValueNames[COLORCODE_GRN]=string("GRN");
	inputValueNames[COLORCODE_BLU]=string("BLU");
	inputValueNames[COLORCODE_NONE]=string("NON");

	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);

	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();

	FsmLearner *fsmlearner = new FsmLearner(8,10,COLORCODE_NONE);

	std::filebuf fileBuff;
	if (fileBuff.open(offImageFilenameList,std::ios::in))
	{
		std::istream istrm(&fileBuff);
		processImages(&istrm,false,lutColorFilter,fsmlearner);
		fileBuff.close();
	}
	if (fileBuff.open(onImageFilenameList,std::ios::in))
	{
		std::istream istrm(&fileBuff);
		processImages(&istrm,true,lutColorFilter,fsmlearner);
		fileBuff.close();
	}

	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);
	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);

	// Optimize tree
	// Set precisions
	fsmlearner->setPrecisionStatus(fsmlearner->counterTreeRoot,0.7F);

	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	// cut
	cout << "------------- cut -------------" << endl;
	fsmlearner->counterTreeRoot->cut(0);
	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	cout << "------------- merge -------------" << endl;
	fsmlearner->mergeNodesForPrecision(&inputValueNames);
	fsmlearner->setPrecisionStatus(fsmlearner->counterTreeRoot,0.7F);	// re-set precision status and AUX!
	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);


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

		fsmlearner->getScoreMaskForImage(lut,score);

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

