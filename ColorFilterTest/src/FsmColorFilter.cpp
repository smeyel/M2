#include "FsmColorFilter.h"

using namespace smeyel;

FsmColorFilter::FsmColorFilter()
{
	this->transitions = NULL;
}

FsmColorFilter::~FsmColorFilter()
{
	if (this->transitions)
	{
		delete this->transitions;
		this->transitions = NULL;
		this->stateNumber = 0;
	}
}

void FsmColorFilter::Filter_Internal(cv::Mat &src, cv::Mat &dst)
{
	assert(this->transitions != NULL);

	// Assert for only 8UC1 output images
	assert(dst.type() == CV_8UC1);
	// Assert dst has same size as src
	assert(src.cols == dst.cols);
	assert(src.rows == dst.rows);

	// Assert destination image type is CV_8UC1
	assert(DetectionMask->type() == CV_8UC1);
	// Assert mask size
	assert(DetectionMask->cols == dst.cols);
	assert(DetectionMask->rows == dst.rows);

	uchar colorCode;
	uchar state = FSM_STATE_INIT;

	int lastDetectionCol = -1;
	int continuousDetectionStartCol = -1;

	unsigned int minStateId = this->minStateIdToSave;

	// Init processing of a new frame
	StartNewFrame();

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);
		// Result pointer
		uchar *resultPtr = (uchar *)(dst.data + row*dst.step);
		// Update mask data pointers
		uchar *detectionMaskPtr = (uchar *)(DetectionMask->data + row*DetectionMask->step);

		// Every row starts with the initial state.
		state = FSM_STATE_INIT;
		lastDetectionCol = -1;
		continuousDetectionStartCol = -1;

		// Go along every BGR colorspace pixel
		for (int col=0; col<src.cols; col++)
		{
			uchar B = *ptr++;
			uchar G = *ptr++;
			uchar R = *ptr++;
			unsigned int idxR = R >> 5;
			unsigned int idxG = G >> 5;
			unsigned int idxB = B >> 5;
			unsigned int idx = (idxR << 6) | (idxG << 3) | idxB;
			colorCode = this->RgbLut[idx];

			// FSM
			state = transitions[state*stateNumber+colorCode];
			//state = fsm(state, colorCode);

			*resultPtr++ = state;

			// Bounding box handling: bounding state values > 127
			if ((state >= minStateId))
			{
				// Handle detection collection
				if (lastDetectionCol == -1)	// Starting first detection in this column
				{
					// Start new continuous detection
					continuousDetectionStartCol = col;
				}
				else if (lastDetectionCol < col-1)	// There was a gap since last detection
				{
					// Register last continuous detection
					RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);

					// Start new continuous detection
					continuousDetectionStartCol = col;
				}
				lastDetectionCol = col;
			}
			detectionMaskPtr++;
		}	// end for col
		// Starting new row (if any...)
		if (lastDetectionCol != -1)
		{
			// There is still an unregistered detection left
			RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);
		}

		FinishRow(row);
	}	// end for row
}

void FsmColorFilter::Filter(cv::Mat *src, cv::Mat *dst, std::vector<cv::Rect> *resultBoundingBoxes)
{
	Filter_Internal(*src,*dst);
	ConsolidateBoundingBoxes();
	// Copy bounding boxes from internal vector
	*resultBoundingBoxes = boundingBoxes;
}
