#ifndef __FSMCOLORFILTER_H
#define __FSMCOLORFILTER_H

#include "LutColorFilter.h"

#define FSM_STATE_INIT	0

namespace smeyel
{
	/** Finite state machine based color filter.
		The output is a CV_8UC1 type image containing the current
		state of the FSM for every pixel.
	*/
	class FsmColorFilter : public LutColorFilter
	{
	protected:
		/** State transition vector. You may use FsmBuilder to create it. */
		unsigned int *transitions;
		/** Number of states. */
		unsigned int stateNumber;
		/** Image areas with state ID >= minStateIdToSave will be marked with bounding boxes.*/
		unsigned int minStateIdToSave;

		/** Internal filtering function called by Filter(). */
		void Filter_Internal(cv::Mat &src, cv::Mat &dst);

	public:
		/** Constructor */
		FsmColorFilter();

		/** Destructor */
		~FsmColorFilter();

		/** Execute filter on an image.
			Internally, performs the filtering and consolidates the bounding boxes.

			@param src	Source image
			@param dst	Destination image (may be NULL), contains the color codes
			@param resultBoundingBoxes	Vector to collect detected areas (if not NULL)

			The FSM is initialized to state 0 at the begining of every image line.
			Bounding boxes surround areas with state ID >= minStateIdToSave.
		*/
		virtual void Filter(cv::Mat *src, cv::Mat *dst, std::vector<cv::Rect> *resultBoundingBoxes);
	};
}

#endif
