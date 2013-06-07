#ifndef __COLORFILTER_H
#define __COLORFILTER_H
#include <stdlib.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>

namespace smeyel
{
	/** Abstract base class for color filters */
	class ColorFilter
	{
	public:
		class BoundingBoxCheckParams
		{
		public:
			BoundingBoxCheckParams()
			{
				// Set defaults
				minWidht = 5;
				maxWidth = 300;
				minHeight = 5;
				maxHeight = 300;
				minRatio = 0.2;
				maxRatio = 5;
			}

			int minWidht, maxWidth;
			int minHeight, maxHeight;
			float minRatio, maxRatio;	// width / height

			bool check(cv::Rect rect)
			{
				if (rect.width < minWidht || rect.width > maxWidth)
					return false;
				if (rect.height < minHeight || rect.height > maxHeight)
					return false;
				float ratio = (float)rect.width / (float)rect.height;
				if (ratio < minRatio || ratio > maxRatio)
					return false;
				return true;
			}
		};

	private:
		/** Represents a bounding box. Used to keep track of the blobs to
			finally get the location and size of the blob.
		*/
		struct DetectionRect
		{
			int rowMin;	// First row of detection, set at first encounter
			//int rowMax;	// Last row, set at the end of detection
			int colMin, colMax;	// horizontal boundaries, kept up-to-date
			bool isAlive;	// is this struct used at all?
			bool isDetectedInCurrentRow;	// Was there a detection in the current row?
		};

		/** Operations used for bounding box creation.
			During a row-by-row detection, do the following:
			- call StartNewFrame() before processing a new frame
			- if you detect the target in a row, call RegisterDetection()
			- at the end of the row, call FinishRow()
			After finishing the processing of the frame, resulting bounding boxes
			will be collected in the vector boundingBoxes.
		///@{ */

		/** Internally used by bounding box detection.
			A DetectionRect is a bounding box which may continue in the
			successive rows. This way, bottom row is unknown and cannot be saved yet
			into the vector boundingBoxes.
		*/
		std::vector<DetectionRect> detectionRects;

	protected:
		/** After the detection in a frame, the bounding boxes of the detections
			are collected in this vector.
			Handled by the bounding box generation functions below.
			Implementations should copy this vector after processing is completed.
		*/
		std::vector<cv::Rect> initialBoundingBoxes;

		/** Final bounding boxes. */
		std::vector<cv::Rect> boundingBoxes;

		/**  initialBoundingBoxes -> boundingBoxes using boundingBoxCheckParams
			@warning: filter functions should not forget to call this before using boundingBoxes
		*/
		void ConsolidateBoundingBoxes();

		BoundingBoxCheckParams boundingBoxCheckParams;

		/** Inits processing of a new frame */
		void StartNewFrame();

		/** Indicates the detection between pixels from colStart and colEnd in the current pixel row.
			@param row	Index if the current row
			@param colStart	Column where the detection starts
			@param colEnd	Column where the detection ends
		*/
		void RegisterDetection(int row, int colStart, int colEnd);

		/** Post-processes the detections of the current row. */
		void FinishRow(int rowIdx);

		/** Add bounding box if it conforms the checks */
		void AddBoundingBoxIfQualifies(cv::Rect boundingBox);

		/**///@} */

	public:

		/** Constructor */
		ColorFilter();

		void SetBoundingBoxCheckParams(BoundingBoxCheckParams params);

		/** Execute filter on an image
			@param src	Source image
			@param dst	Destination image (may be NULL)
			@param boundingBoxes	Vector to collect detected areas (if applicable and not NULL)
		*/
		virtual void Filter(cv::Mat *src, cv::Mat *dst, std::vector<cv::Rect> *resultBoundingBoxes = NULL) = 0;

		void ShowBoundingBoxes(cv::Mat &img, cv::Scalar color);
	};





}



#endif