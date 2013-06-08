#include "LutFsmLocator.h"

using namespace smeyel;
using namespace cv;

LutFsmLocator::LutFsmLocator()
{
	verboseFsmState = NULL;
	fsmStateImage = NULL;
	lutImage = NULL;
	cleanVerboseImage = false;
	showBoundingBoxesOnSrc = false;
	overrideFullLut = false;
}

LutFsmLocator::~LutFsmLocator()
{
	if (verboseFsmState)
		delete verboseFsmState;
	if (fsmStateImage)
		delete fsmStateImage;
	if (lutImage)
		delete lutImage;
	verboseFsmState = NULL;
	fsmStateImage = NULL;
	lutImage = NULL;
}

void LutFsmLocator::useFsmBuilder(FsmBuilder &builder, unsigned int initialState, unsigned int minStateIdToSave, unsigned int minStateIdToCommit)
{
	int inputNumber;
	int stateNumber;
	this->transitions = builder.createFsmTransitionMatrix(stateNumber, inputNumber);
	assert(inputNumber<256);	// We use a CV_8UC1 image for storing the states
	assert(stateNumber<256);	// We use a CV_8UC1 image for storing the color codes

	this->stateNumber = stateNumber;
	this->initialState = builder.getIdxOfStateID(initialState);
	this->minStateToSave = builder.getIdxOfStateID(minStateIdToSave);
	this->minStateToCommit = builder.getIdxOfStateID(minStateIdToCommit);
}

void LutFsmLocator::useBoundingBoxCheckParams(ColorFilter::BoundingBoxCheckParams params)
{
	this->boundingBoxCheckParams = params;
}

void LutFsmLocator::processImage(cv::Mat &src)
{
	bbVector.clear();

	if (!lutImage)
	{
		lutImage = new Mat(src.rows,src.cols,CV_8UC1);
	}

	if (verboseFsmState)
	{
		if (fsmStateImage==NULL)
		{
			fsmStateImage = new Mat(src.rows,src.cols,CV_8UC1);
		}
	}

	if (verboseFsmState)
	{
		FsmColorFilter::Filter(&src,fsmStateImage,&bbVector);	// Creates debug output
		InverseLut(*fsmStateImage, *verboseFsmState);
	}
	else
	{
		FsmColorFilter::Filter(&src,NULL,&bbVector);	// Does not create debug output, only bounding boxes
	}

	// Insert point: access to all bounding boxes at once
	this->processBoundingBoxes(bbVector);

	if (cleanVerboseImage)
		lutImage->setTo(0);	// Only for nicer visualization...

	if (!overrideFullLut)
	{
		for(int i=0; i<bbVector.size(); i++)
		{
			Rect &rect = bbVector[i];
			LutColorFilter::FilterRoI(src,rect,*lutImage);

			// Insert point: access to single bounding box and LUT image of its area
			this->processSingleBoundingBox(rect, *lutImage, src);
		}
	}
	else
	{
		// Override and perform full LUT
		LutColorFilter::Filter(&src,lutImage,NULL);
	}

	if (verboseLutImage)
	{
		InverseLut(*lutImage,*verboseLutImage);
	}

	if (showBoundingBoxesOnSrc)
	{
		int size = bbVector.size();
		char txt[100];
		sprintf(txt,"FsmBBNum=%d",size);
		putText( src, string(txt), cvPoint(25,20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,0) );
		ShowBoundingBoxes(src,Scalar(0,255,0));
	}
}