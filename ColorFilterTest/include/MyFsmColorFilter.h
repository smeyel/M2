#ifndef __MYFSMCOLORFILTER_H
#define __MYFSMCOLORFILTER_H
#include "FsmColorFilter.h"

#define COLORCODE_NONE	0
#define COLORCODE_BLK	1
#define COLORCODE_WHT	2
#define COLORCODE_RED	3
#define COLORCODE_GRN	4
#define COLORCODE_BLU	5

using namespace smeyel;

class MyFsmColorFilter : public FsmColorFilter	// Warning! Derived from MY... to inherit LUT!
{
public:
	MyFsmColorFilter();
	void init();
	virtual uchar fsm(uchar state, uchar lutValue);
};

#endif
