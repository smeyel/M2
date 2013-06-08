#include "MyFsmColorFilter.h"
#include "FsmBuilder.h"

#define STATE_INIT					0
#define STATE_RED_UNSURE			1	// 5 state pure red requirement
#define STATE_RED_UNSURE_NOISE		6	// 10 state noise tolerance
#define STATE_RED_SURE				16	// 10 state noise tolerance
#define STATE_REDBLU				26	// 10 state transient tolerance
#define STATE_BLU_UNSURE			36	// 5 state pure blue required
#define STATE_BLU_UNSURE_NOISE		41	// 10 state noise tolerance
#define STATE_BLU_SURE				51	// 10 state noise tolerance

MyFsmColorFilter::MyFsmColorFilter()
{
	init();
}

void MyFsmColorFilter::init()
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

/*
#define STATE_INIT					0
#define STATE_RED_UNSURE			1	// 5 state pure red requirement
#define STATE_RED_UNSURE_NOISE		6	// 10 state noise tolerance
#define STATE_RED_SURE				16	// 10 state noise tolerance
#define STATE_REDBLU				26	// 10 state transient tolerance
#define STATE_BLU_UNSURE			36	// 5 state pure blue required
#define STATE_BLU_UNSURE_NOISE		41	// 10 state noise tolerance
#define STATE_BLU_SURE				51	// 10 state noise tolerance
*/

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
	int inputNumber;
	int stateNumber;
	this->transitions = builder.createFsmTransitionMatrix(stateNumber, inputNumber);
	assert(inputNumber<256);	// We use a CV_8UC1 image for storing the states
	assert(stateNumber<256);	// We use a CV_8UC1 image for storing the color codes

	this->stateNumber = stateNumber;
	this->minStateToSave = builder.getIdxOfStateID(STATE_BLU_UNSURE);
	this->minStateToCommit = builder.getIdxOfStateID(STATE_BLU_SURE);

	// Setup inverse LUT
	InitInverseLut(64,64,0);
	SetInverseLut(FSM_STATE_INIT, 0,0,0);
	for(int i=0; i<5; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_UNSURE+i),			128,0,0);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_UNSURE_NOISE+i),	128,64,64);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_RED_SURE+i),			255,0,0);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_REDBLU+i),				255,0,255);
	for(int i=0; i<5; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_UNSURE+i),			0,0,128);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_UNSURE_NOISE+i),	64,64,128);
	for(int i=0; i<10; i++) SetInverseLut(builder.getIdxOfStateID(STATE_BLU_SURE+i),			0,0,255);

}
