#ifndef __FSMCOLORFILTER_H
#define __FSMCOLORFILTER_H

#include "LutColorFilter.h"

#define FSM_STATE_INIT	0

namespace smeyel
{
	/** LUT based color filter */
	class FsmColorFilter : public LutColorFilter
	{
	public:
		/** Constructor */
		FsmColorFilter() { }

		/** Sets LUT to given colorcode for given RGB color.
			Used for runtime color LUT adjustments.
		*/
		void SetLutItem(uchar r, uchar g, uchar b, uchar colorCode);

		/** State image, stores for every pixel the current value of the FSM. Type is CV_8UC1. */
		cv::Mat *StateImage;

		/** Execute filter on an image
			@param src	Source image
			@param dst	Destination image (may be NULL), contains the color codes
			@param boundingBoxes	Vector to collect detected areas (if not NULL)
		*/
		virtual void Filter(cv::Mat *src, cv::Mat *dst, std::vector<cv::Rect> *resultBoundingBoxes)
		{
			Filter_Internal(*src,*dst);
			// Copy bounding boxes from internal vector
			*resultBoundingBoxes = boundingBoxes;
		}

		void Filter_Internal(cv::Mat &src, cv::Mat &dst);


		/** Finite state machine function
			@param	lutValue	the value of the current pixel, retrieved from the LUT (@see LutColorFilter)
			@return	The state of the FSM to be stored in the out.

			If you want to return bounding boxes, use the bounding box functions from ColorFilter inside this function.
			@warning TODO: Virtual function call introduces critical overhead! Use function pointer instead?
		*/
		virtual uchar fsm(uchar state, uchar lutValue) = 0;
	};
}

#endif
