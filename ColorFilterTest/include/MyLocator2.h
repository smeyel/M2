#ifndef __MYLOCATOR2_H
#define __MYLOCATOR2_H
#include "LutFsmLocator.h"

namespace smeyel
{
	class MyLocator2 : public LutFsmLocator
	{
		virtual void processBoundingBoxes(std::vector<cv::Rect> &boundingBoxes);
		virtual void processSingleBoundingBox(cv::Rect &boundingBox, cv::Mat &lutImage, cv::Mat &originalImage);

	public:
		MyLocator2();

		void init();
	};
}

#endif
