#ifndef __MYLUTFSMLOCATOR_H
#define __MYLUTFSMLOCATOR_H
#include "LutFsmLocator.h"

#include "MyColorCodes.h"


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
