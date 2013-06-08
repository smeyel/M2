#include "FsmColorFilter.h"

using namespace smeyel;

FsmColorFilter::FsmColorFilter()
{
	this->transitions = NULL;
	this->initialState = 0;
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
	assert(this->minStateToSave <=  this->minStateToCommit);	// Must have forgotten to set...

	// Assert for only 8UC1 output images
	assert(dst.type() == CV_8UC1);
	// Assert dst has same size as src
	assert(src.cols == dst.cols);
	assert(src.rows == dst.rows);

	uchar colorCode;
	unsigned int initStateID = this->initialState;
	unsigned int stateIdx = initStateID;
	int colNum = src.cols;	// for acceleration

	int lastDetectionCol = -1;
	int continuousDetectionStartCol = -1;
	bool isDetectionSaveCommitted = false;

	unsigned int minStateSave = this->minStateToSave;
	unsigned int minStateCommit = this->minStateToCommit;

	// Init processing of a new frame
	StartNewFrame();

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);
		// Result pointer
		uchar *resultPtr = (uchar *)(dst.data + row*dst.step);

		// Every row starts with the initial state.
		stateIdx = initStateID;
		lastDetectionCol = -1;
		continuousDetectionStartCol = -1;

		// Go along every BGR colorspace pixel
		for (int col=0; col<colNum; col++)
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
			stateIdx = transitions[stateIdx+colorCode];	// by IDX encoding

			*resultPtr++ = stateIdx;

			// Bounding box handling
			if ((stateIdx >= minStateSave))
			{
				// Handle detection collection
				if (lastDetectionCol == -1)	// Starting first detection in this column
				{
					// Start new continuous detection
					continuousDetectionStartCol = col;
				}
				else if (lastDetectionCol < col-1)	// There was a gap since last detection
				{
					if (isDetectionSaveCommitted)
					{
						// Register last continuous detection
						RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);
					}

					// Start new continuous detection
					continuousDetectionStartCol = col;
					isDetectionSaveCommitted = false;
				}
				lastDetectionCol = col;	// update end of detection area

				// Cannot do this earlier, at RegisterDetection we need previous value
				if (stateIdx >= minStateCommit)
					isDetectionSaveCommitted = true;
			}	// end of bounding box handling
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

void FsmColorFilter::Filter_Internal_NoOutput(cv::Mat &src)
{
	assert(this->transitions != NULL);
	assert(this->minStateToSave <=  this->minStateToCommit);	// Must have forgotten to set...

	uchar colorCode;
	unsigned int stateIdx = FSM_STATE_INIT;
	int colNum = src.cols;	// for acceleration

	int lastDetectionCol = -1;
	int continuousDetectionStartCol = -1;
	bool isDetectionSaveCommitted = false;

	unsigned int minStateSave = this->minStateToSave;
	unsigned int minStateCommit = this->minStateToCommit;

	// Init processing of a new frame
	StartNewFrame();

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);

		// Every row starts with the initial state.
		stateIdx = FSM_STATE_INIT;
		lastDetectionCol = -1;
		continuousDetectionStartCol = -1;

		// Go along every BGR colorspace pixel
		for (int col=0; col<colNum; col++)
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
			stateIdx = transitions[stateIdx+colorCode];	// by IDX encoding

			// Bounding box handling
			if ((stateIdx >= minStateSave))
			{
				// Handle detection collection
				if (lastDetectionCol == -1)	// Starting first detection in this column
				{
					// Start new continuous detection
					continuousDetectionStartCol = col;
				}
				else if (lastDetectionCol < col-1)	// There was a gap since last detection
				{
					if (isDetectionSaveCommitted)
					{
						// Register last continuous detection
						RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);
					}

					// Start new continuous detection
					continuousDetectionStartCol = col;
					isDetectionSaveCommitted = false;
				}
				lastDetectionCol = col;	// update end of detection area

				// Cannot do this earlier, at RegisterDetection we need previous value
				if (stateIdx >= minStateCommit)
					isDetectionSaveCommitted = true;
			}	// end of bounding box handling
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
	if (dst)
	{
		Filter_Internal(*src,*dst);
	}
	else
	{
		Filter_Internal_NoOutput(*src);
	}
	ConsolidateBoundingBoxes();
	// Copy bounding boxes from internal vector
	*resultBoundingBoxes = boundingBoxes;
}
