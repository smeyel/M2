#include "MyLutFsmLocator.h"

// First 6 values are used by COLORCODE-s in the same LUT!
#define STATE_INIT					20
#define STATE_RED_UNSURE			21	// 5 state pure red requirement
#define STATE_RED_UNSURE_NOISE		26	// 10 state noise tolerance
#define STATE_RED_SURE				36	// 10 state noise tolerance
#define STATE_REDBLU				46	// 10 state transient tolerance
#define STATE_BLU_UNSURE			56	// 5 state pure blue required
#define STATE_BLU_UNSURE_NOISE		61	// 10 state noise tolerance
#define STATE_BLU_SURE				71	// 10 state noise tolerance

using namespace smeyel;

MyLutFsmLocator::MyLutFsmLocator()
{
	init();
}

void MyLutFsmLocator::processBoundingBoxes(std::vector<cv::Rect> boundingBoxes)
{
}

void MyLutFsmLocator::processSingleBoundingBox(cv::Rect boundingBox, cv::Mat lutImage, cv::Mat originalImage)
{
}

void MyLutFsmLocator::init()
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

		if (r == g &&  g == b && r <= 64)
		{
			RgbLut[i]=COLORCODE_BLK;
		}
		else if (r == g &&  g == b && r > 64)
		{
			RgbLut[i]=COLORCODE_WHT;
		}
		else if ((r >= 160 &&  r >= g+64 && r >= b+32) || (r < 160 && r >= g+32 && r >= b+32))
		{
			RgbLut[i]=COLORCODE_RED;
		}
		else if ((r <= 64 &&  g <= 64 && b >= 64) || (r <= 96 &&  g <= 96 && b >= 128) || (r == 0 &&  g == 0 && b >= 32) || (r == 0 &&  g == 64 && b == 64))	// 3rd condition overrides some blacks...
		{
			RgbLut[i]=COLORCODE_BLU;
		}
		else if ((r <= 64 &&  g >= 96 && b <= 150) || (r <= 96 &&  g >= 128 && b <= 128) || (g < 96 && g >= 64 && g >= r+64 && g >= r+64))
		{
			RgbLut[i]=COLORCODE_GRN;
		}
		else if (r >= 115 && g >= 115 && b >= 115 && abs(r-g)<=35 && abs(r-b)<=35 && abs(g-b)<=35 )
		{
			RgbLut[i]=COLORCODE_WHT;
		}
		else if (r <= 64 &&  g <= 64 && b <= 64 && abs(r-g)<=32 && abs(r-b)<=32 && abs(g-b)<=32)
		{
			RgbLut[i]=COLORCODE_BLK;
		}
	}

	// Setup FSM
	FsmBuilder builder;
	builder.init(100,100,FSM_STATE_INIT);
	// ---------- Inside INIT area
	builder.setNextState(FSM_STATE_INIT, COLORCODE_RED, STATE_RED_UNSURE);

	// ---------- Inside RED area
	// Unsure red
	builder.setNextState(STATE_RED_UNSURE, COLORCODE_RED, STATE_RED_UNSURE+1);		// stay, counter
	builder.setNextState(STATE_RED_UNSURE, COLORCODE_NONE, STATE_RED_UNSURE_NOISE);	// noise
	builder.setCounterState(STATE_RED_UNSURE,3);
	builder.setCounterInput(STATE_RED_UNSURE,3,COLORCODE_RED,STATE_RED_SURE);		// count ends -> surely in red area

	// Noise in unsure red
	builder.setNextState(STATE_RED_UNSURE_NOISE, COLORCODE_RED, STATE_RED_UNSURE);
	builder.setNextState(STATE_RED_UNSURE_NOISE, COLORCODE_NONE, STATE_RED_UNSURE_NOISE+1);	// count noise
	builder.setCounterState(STATE_RED_UNSURE_NOISE,10);
	builder.setCounterInput(STATE_RED_UNSURE_NOISE,10,COLORCODE_NONE,STATE_INIT);

	// Sure red (may contain some noise)
	builder.setNextState(STATE_RED_SURE, COLORCODE_RED, STATE_RED_SURE);	// stay
	builder.setNextState(STATE_RED_SURE, COLORCODE_BLU, STATE_BLU_UNSURE);	// -> blue area
	builder.setNextState(STATE_RED_SURE, COLORCODE_WHT, STATE_REDBLU);		// -> red-blue transient area
	builder.setNextState(STATE_RED_SURE, COLORCODE_BLK, STATE_REDBLU);		// -> red-blue transient area
	builder.setNextState(STATE_RED_SURE, COLORCODE_NONE, STATE_RED_SURE+1);	// count noise
	builder.setCounterState(STATE_RED_SURE,10);
	builder.setCounterInput(STATE_RED_SURE,10,COLORCODE_NONE,STATE_INIT);

	// ---------- Inside RED-BLU transient area
	builder.setNextState(STATE_REDBLU, COLORCODE_RED, STATE_RED_SURE);		// fallback to red area
	builder.setNextState(STATE_REDBLU, COLORCODE_BLU, STATE_BLU_UNSURE);	// -> blue area
	builder.setNextState(STATE_REDBLU, COLORCODE_WHT, STATE_REDBLU+1);		// stay, count
	builder.setNextState(STATE_REDBLU, COLORCODE_BLK, STATE_REDBLU+1);		// stay, count
	builder.setNextState(STATE_REDBLU, COLORCODE_GRN, STATE_REDBLU+1);		// stay, count
	builder.setNextState(STATE_REDBLU, COLORCODE_NONE, STATE_REDBLU+1);		// stay, count
	builder.setCounterState(STATE_REDBLU,10);
	builder.setCounterInput(STATE_REDBLU,10,COLORCODE_WHT,STATE_INIT);
	builder.setCounterInput(STATE_REDBLU,10,COLORCODE_BLK,STATE_INIT);
	builder.setCounterInput(STATE_REDBLU,10,COLORCODE_GRN,STATE_INIT);
	builder.setCounterInput(STATE_REDBLU,10,COLORCODE_NONE,STATE_INIT);

	// ---------- Inside BLU area
	// Unsure blue
	builder.setNextState(STATE_BLU_UNSURE, COLORCODE_BLU, STATE_BLU_UNSURE+1);		// stay, counter
	builder.setNextState(STATE_BLU_UNSURE, COLORCODE_BLK, STATE_BLU_UNSURE_NOISE);	// noise
	builder.setCounterState(STATE_BLU_UNSURE,3);
	builder.setCounterInput(STATE_BLU_UNSURE,3,COLORCODE_BLU,STATE_BLU_SURE);		// count ends -> surely in blue area

	// Noise in unsure blue
	builder.setNextState(STATE_BLU_UNSURE_NOISE, COLORCODE_BLU, STATE_BLU_UNSURE);
	builder.setNextState(STATE_BLU_UNSURE_NOISE, COLORCODE_RED, STATE_RED_UNSURE);	// fallback to red
	builder.setNextState(STATE_BLU_UNSURE_NOISE, COLORCODE_WHT, STATE_REDBLU);	// fallback to REDBLU
	builder.setNextState(STATE_BLU_UNSURE_NOISE, COLORCODE_BLK, STATE_BLU_UNSURE_NOISE+1);	// count noise
	builder.setNextState(STATE_BLU_UNSURE_NOISE, COLORCODE_GRN, STATE_BLU_UNSURE_NOISE+1);	// count noise
	builder.setCounterState(STATE_BLU_UNSURE_NOISE,10);
	builder.setCounterInput(STATE_BLU_UNSURE_NOISE,10,COLORCODE_BLK,STATE_INIT);
	builder.setCounterInput(STATE_BLU_UNSURE_NOISE,10,COLORCODE_GRN,STATE_INIT);

	// Sure blue (may contain some noise)
	builder.setNextState(STATE_BLU_SURE, COLORCODE_RED, STATE_RED_UNSURE);	// fallback to red
	builder.setNextState(STATE_BLU_SURE, COLORCODE_BLU, STATE_BLU_SURE);	// stay
	builder.setNextState(STATE_BLU_SURE, COLORCODE_BLK, STATE_BLU_SURE+1);	// stay, count noise
	builder.setNextState(STATE_BLU_SURE, COLORCODE_GRN, STATE_BLU_SURE+1);	// stay, count noise
	builder.setCounterState(STATE_BLU_SURE,10);
	builder.setCounterInput(STATE_BLU_SURE,10,COLORCODE_BLK,STATE_INIT);
	builder.setCounterInput(STATE_BLU_SURE,10,COLORCODE_GRN,STATE_INIT);

	// ---------- Build FSM
	this->useFsmBuilder(builder,FSM_STATE_INIT,STATE_BLU_UNSURE,STATE_BLU_SURE);

	// Setup inverse LUT
	InitInverseLut(64,64,0);
	// Color codes
	SetInverseLut(COLORCODE_NONE, 100,0,100);
	SetInverseLut(COLORCODE_BLK, 0,0,0);
	SetInverseLut(COLORCODE_WHT, 255,255,255);
	SetInverseLut(COLORCODE_RED, 255,0,0);
	SetInverseLut(COLORCODE_GRN, 0,255,0);
	SetInverseLut(COLORCODE_BLU, 0,0,255);
	// States
	SetInverseLut(FSM_STATE_INIT, 0,0,0);
	for(int i=0; i<5; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_UNSURE+i),			128,0,0);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_UNSURE_NOISE+i),	128,64,64);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_SURE+i),			255,0,0);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_REDBLU+i),				255,0,255);
	for(int i=0; i<5; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_UNSURE+i),			0,0,128);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_UNSURE_NOISE+i),	64,64,128);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_SURE+i),			0,0,255);
}
