#include "ColorFilter.h"
#include "Logger.h"

#define LOG_TAG "ColorFilter"

using namespace smeyel;

ColorFilter::ColorFilter()
{
	// Pre-allocate space
	detectionRects.reserve(100);
	initialBoundingBoxes.reserve(100);
	boundingBoxes.reserve(100);
	detectionRects.clear();
	initialBoundingBoxes.clear();
	boundingBoxes.clear();
}

void ColorFilter::StartNewFrame()
{
	detectionRects.clear();
	initialBoundingBoxes.clear();
	boundingBoxes.clear();
}

void ColorFilter::RegisterDetection(int row, int colStart, int colEnd)
{
	// Search for detection rects with overlapping with given interval
	bool newRect=true;
	int detectionRectNum = detectionRects.size();
	for(int i=0; i<detectionRectNum; i++)
	{
		if (!detectionRects[i].isAlive)
		{
			continue;
		}
		// If there is an overlap (no sure gap on either sides)
		if (!(
			// detection is before the rect
			(detectionRects[i].colMin > colEnd) 
			// detection is after the rect
			|| (detectionRects[i].colMax < colStart) ))
		{
			// Update boundaries colMin and colMax
			if (detectionRects[i].colMin > colStart)
			{
				detectionRects[i].colMin = colStart;
			}
			if (detectionRects[i].colMax < colEnd)
			{
				detectionRects[i].colMax = colEnd;
			}

			// Register to keep this rect alive
			detectionRects[i].isDetectedInCurrentRow = true;

			// Should not create a new detection for this
			newRect=false;
		}
	}

	if(newRect)
	{
		// Create new detection rect (find a place for it...)
		bool foundFreeDetectionRect = false;
		for(int i=0; i<detectionRectNum; i++)	// Find first !isAlive
		{
			if (!detectionRects[i].isAlive)
			{
				detectionRects[i].isAlive = true;
				detectionRects[i].rowMin = row;
				detectionRects[i].colMin = colStart;
				detectionRects[i].colMax = colEnd;
				detectionRects[i].isDetectedInCurrentRow = true;
				foundFreeDetectionRect = true;
				break;	// Stop searching
			}
		}
		// If there was no free detectionRect, do nothing...
		if (!foundFreeDetectionRect)
		{
			DetectionRect newDetectionRect;
			newDetectionRect.isAlive = true;
			newDetectionRect.rowMin = row;
			newDetectionRect.colMin = colStart;
			newDetectionRect.colMax = colEnd;
			newDetectionRect.isDetectedInCurrentRow = true;
			detectionRects.push_back(newDetectionRect);
			// Send warning in this case!
			LogConfigTime::Logger::getInstance()->Log(LogConfigTime::Logger::LOGLEVEL_VERBOSE, LOG_TAG, "ColorFilter: has to extend detectionRects size\n");
		}
	}
}

void ColorFilter::ConsolidateBoundingBoxes()
{
	// Simply copy qualifying bounding boxes
	// TODO: later add merges, overlap checks etc.
	for (int i=0; i<initialBoundingBoxes.size(); i++)
	{
		if (this->boundingBoxCheckParams.check(initialBoundingBoxes[i]))
			boundingBoxes.push_back(initialBoundingBoxes[i]);
	}
}

void ColorFilter::SetBoundingBoxCheckParams(BoundingBoxCheckParams params)
{
	this->boundingBoxCheckParams = params;
}


void ColorFilter::FinishRow(int rowIdx)
{
	// Create marker candidate from every discontinued detection rectangle
	// TODO: this will not create candidate rect for detections touching the last row of the image!
	int detectionRectNum = detectionRects.size();

	for(int i=0; i<detectionRectNum; i++)
	{
		if (!detectionRects[i].isAlive)
		{
			continue;
		}
		// Ends in current row or not?
		if (!detectionRects[i].isDetectedInCurrentRow)
		{
			// Not seen anymore, store as candidate
			cv::Rect newBoundingBox;
			newBoundingBox.x = detectionRects[i].colMin;
			newBoundingBox.y = detectionRects[i].rowMin;
			newBoundingBox.width = detectionRects[i].colMax-detectionRects[i].colMin;
			newBoundingBox.height = rowIdx-detectionRects[i].rowMin;
			initialBoundingBoxes.push_back(newBoundingBox);

			// Deactivate detection rect
			detectionRects[i].isAlive = false;
		}
		else
		{
			// Reset detection status for next row
			detectionRects[i].isDetectedInCurrentRow = false;
		}
	}
}

void ColorFilter::ShowBoundingBoxes(cv::Mat &img, cv::Scalar color)
{
		for(int i=0; i<boundingBoxes.size(); i++)
		{
			rectangle(img,boundingBoxes[i],color);
		}
}