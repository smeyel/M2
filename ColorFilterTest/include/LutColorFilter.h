#ifndef __LUTCOLORFILTER_H
#define __LUTCOLORFILTER_H

#include "ColorFilter.h"

namespace smeyel
{
	/** LUT based color filter */
	class LutColorFilter : public ColorFilter
	{
		void Filter_All(cv::Mat &src, cv::Mat &dst);
		void Filter_NoBoundingBox(cv::Mat &src, cv::Mat &dst);
		void Filter_NoMatOutputNoMask(cv::Mat &src);
	protected:
		/** RGB Look-up table
			3 bit / RGB color layer -> 9 bit, 512 values. 0 means undefined color.
			Use SetLutItem to set. Take quantizing to 3 bits into account!
		*/
		uchar RgbLut[512];

		uchar *inverseLut;	// RGB order!

	public:
		/** Constructor */
		LutColorFilter();

		~LutColorFilter();

		/** Sets LUT to given colorcode for given RGB color.
			Used for runtime color LUT adjustments.
		*/
		void SetLutItem(uchar r, uchar g, uchar b, uchar colorCode);

		void InitLut(uchar colorCode);


		/** Color code to find. The bounding boxes will be created for areas of this color. */
		unsigned char ColorCodeToFind;

		/** Binary (0, 255) detection mask. Inidcates areas where ColorCodeToFind is located.
			@warning Only used if Filter is given dst and resultBoundingBoxes as well! Otherwise, omitted.
		*/

		cv::Mat *DetectionMask;

		/** Execute filter on an image
			@param src	Source image
			@param dst	Destination image (may be NULL), contains the color codes
			@param boundingBoxes	Vector to collect detected areas (if not NULL)
		*/
		virtual void Filter(cv::Mat *src, cv::Mat *dst, std::vector<cv::Rect> *resultBoundingBoxes = NULL);

		void FilterRoI(cv::Mat &src, cv::Rect &roi, cv::Mat &dst);


		/** Inverse LUT (for visualization) */
		void InverseLut(cv::Mat &src, cv::Mat &dst);
		void InitInverseLut(uchar r, uchar g, uchar b);
		void SetInverseLut(uchar colorCode, uchar r, uchar g, uchar b);
	};
}

#endif
