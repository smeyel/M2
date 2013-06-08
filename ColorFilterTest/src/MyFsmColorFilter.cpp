#include "MyFsmColorFilter.h"
#include "FsmBuilder.h"

#define FSM_STATE_INIT	0
#define FSM_STATE_WHT	2
#define FSM_STATE_RED	3
#define FSM_STATE_REDWHT	4
#define FSM_STATE_REDBLU	5

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

	// Setup inverse LUT
/*	InitInverseLut(200,0,200);
	SetInverseLut(COLORCODE_NONE, 100,0,100);
	SetInverseLut(COLORCODE_BLK, 0,0,0);
	SetInverseLut(COLORCODE_WHT, 255,255,255);
	SetInverseLut(COLORCODE_RED, 255,0,0);
	SetInverseLut(COLORCODE_GRN, 0,255,0);
	SetInverseLut(COLORCODE_BLU, 0,0,255);*/
	InitInverseLut(100,0,100);
	SetInverseLut(COLORCODE_NONE, 0,0,0);
	SetInverseLut(COLORCODE_BLK, 0,0,0);
	SetInverseLut(COLORCODE_WHT, 64,64,64);
	SetInverseLut(COLORCODE_RED, 64,0,0);
	SetInverseLut(COLORCODE_GRN, 0,64,0);
	SetInverseLut(COLORCODE_BLU, 0,0,255);

	SetInverseLut(10, 255,0,0);	// debug color

	// TODO: REDWHT csak pár pixel lehet! Köztes NONE is csak pár lehet... és GRN se legyen túl sok...

	// Setup FSM
	FsmBuilder builder;
	builder.init(100,100,FSM_STATE_INIT);
	// From INIT
	builder.setNextState(FSM_STATE_INIT, COLORCODE_WHT, FSM_STATE_WHT);	// ->WHT
	builder.setNextState(FSM_STATE_INIT, COLORCODE_RED, FSM_STATE_RED);	// ->WHT
	// From WHT
	builder.setNextState(FSM_STATE_WHT, COLORCODE_WHT, FSM_STATE_WHT);	// stay
	builder.setNextState(FSM_STATE_WHT, COLORCODE_NONE, FSM_STATE_WHT);	// stay
	builder.setNextState(FSM_STATE_WHT, COLORCODE_RED, FSM_STATE_RED);	// ->RED
	// From RED
	builder.setNextState(FSM_STATE_RED, COLORCODE_RED, FSM_STATE_RED);	// stay
	builder.setNextState(FSM_STATE_RED, COLORCODE_NONE, FSM_STATE_RED);	// stay
	builder.setNextState(FSM_STATE_RED, COLORCODE_BLK, FSM_STATE_RED);	// stay (may be BLU)
	builder.setNextState(FSM_STATE_RED, COLORCODE_WHT, FSM_STATE_REDWHT);	// -> REDWHT
	builder.setNextState(FSM_STATE_RED, COLORCODE_BLU, FSM_STATE_REDBLU);	// -> REDBLU
	// From REDWHT
	builder.setNextState(FSM_STATE_REDWHT, COLORCODE_WHT, FSM_STATE_REDWHT);	// stay
	builder.setNextState(FSM_STATE_REDWHT, COLORCODE_NONE, FSM_STATE_REDWHT);	// stay
	builder.setNextState(FSM_STATE_REDWHT, COLORCODE_BLU, FSM_STATE_REDBLU);	// -> REDBLU
	builder.setNextState(FSM_STATE_REDWHT, COLORCODE_RED, FSM_STATE_RED);	// -> RED
	// From REDBLU
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_NONE, FSM_STATE_REDBLU);	// stay
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_BLU, FSM_STATE_REDBLU);	// stay
	builder.setNextState(FSM_STATE_REDBLU, COLORCODE_GRN, FSM_STATE_REDBLU);	// stay

	// TODO: vertical brakes due to (single line for example) can be avoided by a 2D FSM...
	int inputNumber;
	int stateNumber;
	this->transitions = builder.createFsmTransitionMatrix(stateNumber, inputNumber);

	this->stateNumber = stateNumber;
	this->minStateIdToSave = 5;

}
