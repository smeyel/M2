#ifndef __MYLUTCOLORFILTER_H
#define __MYLUTCOLORFILTER_H
#include "LutColorFilter.h"

#define COLORCODE_NONE	0
#define COLORCODE_BLK	1
#define COLORCODE_WHT	2
#define COLORCODE_RED	3
#define COLORCODE_GRN	4
#define COLORCODE_BLU	5

using namespace smeyel;

class MyLutColorFilter : public LutColorFilter
{
public:
	MyLutColorFilter();
	void init();
};

#endif
