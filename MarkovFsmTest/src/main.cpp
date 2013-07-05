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

using namespace std;
using namespace cv;
using namespace smeyel;
using namespace LogConfigTime;


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

void getScoreMaskForImage(TransitionStat *stat, Mat &src, Mat &dst)
{
	// Assert for only 8UC1 output images
	OPENCV_ASSERT(src.type() == CV_8UC1,"getScoreMaskForImage","src type is not CV_8UC1");
	OPENCV_ASSERT(dst.type() == CV_8UC1,"getScoreMaskForImage","dst type is not CV_8UC1");
	OPENCV_ASSERT(src.rows=dst.rows,"getScoreMaskForImage","src and dst size does not match");
	OPENCV_ASSERT(src.cols=dst.cols,"getScoreMaskForImage","src and dst size does not match");

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *srcPtr = (const uchar *)(src.data + row*src.step);
		uchar *dstPtr = (uchar *)(dst.data + row*dst.step);

		// Go along every BGR colorspace pixel
		for (int col=0; col<src.cols; col++)
		{
			unsigned char value = *srcPtr++;
			*dstPtr = stat->getScoreForValue(value);
			dstPtr++;
		}	// end for col
	}	// end for row
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

	}
}

// Type of callback for promising nodes
typedef void (*notifycallbackPtr)(SequenceCounterTreeNode *node, float rate);

void callback(SequenceCounterTreeNode *node, float rate)
{
	cout << "----- Promising node found: " << endl;
	// set score of node
	node->auxScore = (unsigned char)((rate-0.8F)/0.2F*255.0F);

	cout << "Input sequence (reverse order!!!):" << endl;
	SequenceCounterTreeNode *current = node;
	SequenceCounterTreeNode *parent = node->getParentNode();
	while(parent)
	{
		cout << parent->getInputValueForChild(current) << " ";
		current = parent;
		parent = current->getParentNode();
	}
	cout << "(root)" << endl;

	cout << "Subtree:" << endl;
	node->showCompactRecursive(0,1);
}


void checkNode(SequenceCounterTreeNode *node, float sumOn, float sumOff, int maxInputValue, notifycallbackPtr callback)
{
	float onRate = node->getCounter(COUNTERIDX_ON) / sumOn;
	float offRate = node->getCounter(COUNTERIDX_OFF) / sumOff;
	float rate = onRate / (onRate+offRate);
	if (rate > 0.95)
	{
		(*callback)(node, rate);
	}
	// Continue on children
	for (int i=0; i<=maxInputValue; i++)
	{
		SequenceCounterTreeNode *child = node->getChildNode(i,false);
		if (child)
		{
			checkNode(child,sumOn,sumOff,maxInputValue,callback);
		}
	}
}

void findClassifierSequences(TransitionStat *stat)
{
	// Recursively go along every sequence and compare on/off frequencies
	SequenceCounterTreeNode *root = stat->counterTreeRoot;

	SequenceCounterTreeNode *node = root;

	float sumOn = (float)root->getCounter(COUNTERIDX_ON);
	float sumOff = (float)root->getCounter(COUNTERIDX_OFF);

	checkNode(root,sumOn,sumOff,7,callback);
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

	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_OFF);
	stat->counterTreeRoot->getAndStoreSubtreeSumCounter(COUNTERIDX_ON);

	float offSum = (float)stat->counterTreeRoot->getCounter(COUNTERIDX_OFF);
	float onSum = (float)stat->counterTreeRoot->getCounter(COUNTERIDX_ON);

	stat->counterTreeRoot->showCompactRecursive(0,1);

	findClassifierSequences(stat);

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

		getScoreMaskForImage(stat,lut,score);

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


int main(int argc, char *argv[], char *window_name)
{
	//test_frameSaver();
	test_mkStatFromImageList("off_list.txt","on_list.txt");
}
