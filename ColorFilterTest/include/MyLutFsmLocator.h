#ifndef __MYLUTFSMLOCATOR_H
#define __MYLUTFSMLOCATOR_H
#include "LutFsmLocator.h"

// Value 0 skipped and used by FSM for initial state
#define COLORCODE_NONE	1
#define COLORCODE_BLK	2
#define COLORCODE_WHT	3
#define COLORCODE_RED	4
#define COLORCODE_GRN	5
#define COLORCODE_BLU	6

namespace smeyel
{
	class MyLutFsmLocator : public LutFsmLocator
	{
		virtual void processBoundingBoxes(std::vector<cv::Rect> &boundingBoxes);
		virtual void processSingleBoundingBox(cv::Rect &boundingBox, cv::Mat &lutImage, cv::Mat &originalImage);

	public:
		MyLutFsmLocator();

		void init();
	};
}

#endif
