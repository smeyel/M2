#include "MyFsmColorFilter.h"
#include "FsmBuilder.h"

#define FSM_STATE_INIT		0
#define FSM_STATE_RED		1	// 10 states: 1-10
#define FSM_STATE_REDBLU	11	// 5 states: 11-15
#define FSM_STATE_BLU		16	// 10 states: 16-25

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
#define FSM_STATE_INIT		0
#define FSM_STATE_RED		1	// 10 states: 1-10
#define FSM_STATE_REDBLU	11	// 5 states: 11-15
#define FSM_STATE_BLU		16	// 10 states: 16-25
*/

	// Setup inverse LUT
	InitInverseLut(64,64,0);
	SetInverseLut(FSM_STATE_INIT, 0,0,0);
	for(int i=0; i<10; i++) SetInverseLut(FSM_STATE_RED+i, 128,0,0);
	for(int i=0; i<5; i++) SetInverseLut(FSM_STATE_REDBLU+i, 128,0,128);
	for(int i=0; i<10; i++) SetInverseLut(FSM_STATE_BLU+i, 128,128,255);

	// Setup FSM
	FsmBuilder builder;
	builder.init(100,100,FSM_STATE_INIT);
	// From INIT
	builder.setNextState(FSM_STATE_INIT, COLORCODE_RED, FSM_STATE_RED);

	// From RED
	builder.setNextState(FSM_STATE_RED, COLORCODE_RED, FSM_STATE_RED);		// stay, reset counter
	builder.setNextState(FSM_STATE_RED, COLORCODE_NONE, FSM_STATE_RED+1);	// stay, count (should come RED or BLU soon)
	builder.setNextState(FSM_STATE_RED, COLORCODE_BLU, FSM_STATE_BLU);
	builder.setNextState(FSM_STATE_RED, COLORCODE_BLK, FSM_STATE_REDBLU);	// WHT allowed between RED and BLU
	builder.setNextState(FSM_STATE_RED, COLORCODE_WHT, FSM_STATE_REDBLU);	// WHT allowed between RED and BLU
	builder.setCounterState(FSM_STATE_RED,10);
	builder.setCounterInput(FSM_STATE_RED,10,COLORCODE_NONE,FSM_STATE_INIT);

	// From REDBLU
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_NONE, FSM_STATE_REDBLU);	// stay, count
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_WHT, FSM_STATE_REDBLU);	// stay, count
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_BLK, FSM_STATE_REDBLU);	// stay, count
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_RED, FSM_STATE_RED);	// fallback to RED area
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_BLU, FSM_STATE_BLU);
	builder.setCounterState(FSM_STATE_REDBLU,5);
	builder.setCounterInput(FSM_STATE_REDBLU,5,COLORCODE_NONE,FSM_STATE_INIT);
	builder.setCounterInput(FSM_STATE_REDBLU,5,COLORCODE_WHT,FSM_STATE_INIT);

	// From BLU
	builder.setNextState(FSM_STATE_BLU, COLORCODE_BLU, FSM_STATE_BLU);		// stay, reset counter
	builder.setNextState(FSM_STATE_BLU, COLORCODE_RED, FSM_STATE_RED);		// End of marker center
	builder.setNextState(FSM_STATE_BLU, COLORCODE_NONE, FSM_STATE_BLU+1);	// stay, count (should come BLU soon)
	builder.setNextState(FSM_STATE_BLU, COLORCODE_GRN, FSM_STATE_BLU+1);	// stay, count (should come BLU soon)
	builder.setNextState(FSM_STATE_BLU, COLORCODE_BLK, FSM_STATE_BLU+1);	// stay, count (should come BLU soon)
	builder.setCounterState(FSM_STATE_BLU,10);
	builder.setCounterInput(FSM_STATE_BLU,10,COLORCODE_NONE,FSM_STATE_INIT);	// End of marker center
	builder.setCounterInput(FSM_STATE_BLU,10,COLORCODE_GRN,FSM_STATE_INIT);		// End of marker center
	builder.setCounterInput(FSM_STATE_BLU,10,COLORCODE_BLK,FSM_STATE_INIT);		// End of marker center

	// TODO: vertical brakes due to (single line for example) can be avoided by a 2D FSM...
	int inputNumber;
	int stateNumber;
	this->transitions = builder.createFsmTransitionMatrix(stateNumber, inputNumber);
	assert(inputNumber<256);	// We use a CV_8UC1 image for storing the states
	assert(stateNumber<256);	// We use a CV_8UC1 image for storing the color codes

	this->stateNumber = stateNumber;
	this->minStateIdToSave = FSM_STATE_BLU;

}
