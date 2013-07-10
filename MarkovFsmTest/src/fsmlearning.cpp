#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

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

