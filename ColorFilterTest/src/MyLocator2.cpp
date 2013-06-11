#include "MyLocator2.h"

// Value 0 skipped and used by FSM for initial state
#define COLORCODE_NONE	1
#define COLORCODE_BLK	2
#define COLORCODE_WHT	3
#define COLORCODE_RED	4
#define COLORCODE_GRN	5
#define COLORCODE_BLU	6

// First 6 values are used by COLORCODE-s in the same LUT!
#define STATE_INIT					10
#define STATE_RED_UNSURE			11	// 5 state pure red requirement
#define STATE_RED_UNSURE_NOISE		12	// 10 state noise tolerance
#define STATE_RED_SURE				13	// 10 state noise tolerance
#define STATE_REDBLU				14	// 10 state transient tolerance
#define STATE_BLU_UNSURE			15	// 5 state pure blue required
#define STATE_BLU_UNSURE_NOISE		16	// 10 state noise tolerance
#define STATE_BLU_SURE				17	// 10 state noise tolerance

using namespace smeyel;

MyLocator2::MyLocator2()
{
	init();
}

void MyLocator2::processBoundingBoxes(std::vector<cv::Rect> &boundingBoxes)
{
}

void MyLocator2::processSingleBoundingBox(cv::Rect &boundingBox, cv::Mat &lutImage, cv::Mat &originalImage)
{
}

void MyLocator2::init()
{
	// Setup LUT
	uchar r,g,b;
	for(unsigned int i=0; i<512; i++)
	{
		RgbLut[i]=COLORCODE_NONE;

		// Get RGB, scale back to 0-255 to simplify the conditions
		r = (i >> 6) << 5;
		g = ((i >> 3) & 0x07) << 5;
		b = (i & 0x07) << 5;

		if (r <= 64 &&  g <= 64 && b <= 64)
		{
			RgbLut[i]=COLORCODE_BLK;
		}
		else if (r >= 192 &&  g >= 192 && b >= 192)
		{
			RgbLut[i]=COLORCODE_WHT;
		}
		else if (r >= g+32 &&  r >= b+32)
		{
			RgbLut[i]=COLORCODE_RED;
		}
	}

	// Setup FSM
	FsmBuilder builder;
	// Init states
	builder.init(100,100);
	builder.newState(STATE_INIT,1);
	builder.setCurrentColor(0,0,0);
	builder.newState(STATE_RED_UNSURE,5);
	builder.setCurrentColor(128,0,0);
	builder.newState(STATE_RED_UNSURE_NOISE,5);
	builder.setCurrentColor(128,64,64);
	builder.newState(STATE_RED_SURE,5);
	builder.setCurrentColor(255,0,0);
	// Set transitions
	builder.selectState(STATE_INIT);
	builder.setDefault();
	builder.setTransition(COLORCODE_RED,STATE_RED_UNSURE);
	builder.showCurrent();

	builder.selectState(STATE_RED_UNSURE);
	builder.setCountingTransition(COLORCODE_RED,STATE_RED_SURE);
	builder.setTransition(COLORCODE_NONE,STATE_RED_UNSURE_NOISE);
	builder.showCurrent();

	builder.selectState(STATE_RED_UNSURE_NOISE);
	builder.setCountingTransition(COLORCODE_NONE,STATE_INIT);
	builder.setTransition(COLORCODE_RED,STATE_RED_UNSURE);
	builder.showCurrent();

	builder.selectState(STATE_RED_SURE);
	builder.setTransition(COLORCODE_RED,STATE_RED_SURE);
	builder.setCountingTransition(COLORCODE_NONE,STATE_INIT);
	builder.showCurrent();

	// ---------- Build FSM
	this->useFsmBuilder(builder,FSM_STATE_INIT,STATE_RED_UNSURE,STATE_RED_SURE);

	// ---------- Setup inverse LUT
	InitInverseLut(128,128,128);
	// Color codes
	SetInverseLut(COLORCODE_NONE, 128,128,128);
	SetInverseLut(COLORCODE_BLK, 0,0,0);
	SetInverseLut(COLORCODE_WHT, 255,255,255);
	SetInverseLut(COLORCODE_RED, 255,0,0);
	// States
	builder.setupRgbInverseLut(this->inverseLut);
}
