#include "LutColorFilter.h"

using namespace smeyel;
using namespace cv;

#define LASTDETECTIONCOL_NONE -1

LutColorFilter::LutColorFilter()
{
	memset(RgbLut,0,512);
	inverseLut=NULL;
}

LutColorFilter::~LutColorFilter()
{
	memset(RgbLut,0,512);
	if (inverseLut)
	{
		delete inverseLut;
		inverseLut = NULL;
	}
}

void LutColorFilter::InitLut(uchar colorCode)
{
	memset(RgbLut,colorCode,512);
}


void LutColorFilter::SetLutItem(uchar r, uchar g, uchar b, uchar colorCode)
{
	unsigned int idxR = r >> 5;
	unsigned int idxG = g >> 5;
	unsigned int idxB = b >> 5;
	unsigned int idx = (idxR << 6) | (idxG << 3) | idxB;
	RgbLut[idx] = colorCode;
}

void LutColorFilter::FilterRoI(Mat &src, Rect &roi, Mat &dst)
{
	assert(src.type() == CV_8UC3);
	assert(dst.type() == CV_8UC1);

	Mat srcRoi(src,roi);
	Mat dstRoi(dst,roi);

	Filter_NoBoundingBox(srcRoi,dstRoi);
}

void LutColorFilter::Filter(Mat *src, Mat *dst, std::vector<Rect> *resultBoundingBoxes)
{
	// Assert for only 8UC3 input images (BGR)
	assert(src->type() == CV_8UC3);

	if (dst!=NULL && resultBoundingBoxes!=NULL)
	{
		Filter_All(*src,*dst);
		// Copy bounding boxes from internal vector
		*resultBoundingBoxes = boundingBoxes;
	}
	else if (dst!=NULL && resultBoundingBoxes==NULL)
	{
		Filter_NoBoundingBox(*src,*dst);
	}
	else if (dst==NULL && resultBoundingBoxes!=NULL)
	{
		Filter_NoMatOutputNoMask(*src);
		// Copy bounding boxes from internal vector
		*resultBoundingBoxes = boundingBoxes;
	}
}

void LutColorFilter::Filter_All(cv::Mat &src, cv::Mat &dst)
{
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
	uchar detectColorCode = this->ColorCodeToFind; // Stored for faster access

	int lastDetectionCol = LASTDETECTIONCOL_NONE;
	int continuousDetectionStartCol = -1;

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

		// Reset tracking of continuous detections
		lastDetectionCol = LASTDETECTIONCOL_NONE;
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
			colorCode = RgbLut[idx];
			*resultPtr++ = colorCode;

			// Handle masks (2 masks)
			uchar isDetectColor = (colorCode==detectColorCode)?255:0;
			*detectionMaskPtr = isDetectColor;
			
			if (isDetectColor)
			{
				// Handle detection collection
				if (lastDetectionCol == LASTDETECTIONCOL_NONE)	// Starting first detection in this column
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
		if (lastDetectionCol != LASTDETECTIONCOL_NONE)
		{
			// There is still an unregistered detection left
			RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);
		}

		FinishRow(row);
	}	// end for row
}

void LutColorFilter::Filter_NoBoundingBox(cv::Mat &src, cv::Mat &dst)
{
	// Assert for only 8UC1 output images
	assert(dst.type() == CV_8UC1);
	// Assert dst has same size as src
	assert(src.cols == dst.cols);
	assert(src.rows == dst.rows);

	uchar colorCode;

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);
		// Result pointer
		uchar *resultPtr = (uchar *)(dst.data + row*dst.step);

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
			colorCode = RgbLut[idx];
			*resultPtr++ = colorCode;
		}	// end for col
	}	// end for row
}

void LutColorFilter::Filter_NoMatOutputNoMask(cv::Mat &src)
{
	// Bounding boxes only...

	uchar colorCode;
	uchar detectColorCode = this->ColorCodeToFind; // Stored for faster access

	int lastDetectionCol = LASTDETECTIONCOL_NONE;
	int continuousDetectionStartCol = -1;

	// Init processing of a new frame
	StartNewFrame();

	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);

		// Reset tracking of continuous detections
		lastDetectionCol = LASTDETECTIONCOL_NONE;
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
			colorCode = RgbLut[idx];

			// Handle masks (2 masks)
			uchar isDetectColor = (colorCode==detectColorCode)?255:0;
			
			if (isDetectColor)
			{
				// Handle detection collection
				if (lastDetectionCol == LASTDETECTIONCOL_NONE)	// Starting first detection in this column
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
		}	// end for col
		// Starting new row (if any...)
		if (lastDetectionCol != LASTDETECTIONCOL_NONE)
		{
			// There is still an unregistered detection left
			RegisterDetection(row,continuousDetectionStartCol,lastDetectionCol);
		}

		FinishRow(row);
	}	// end for row
}

void LutColorFilter::InverseLut(cv::Mat &src, cv::Mat &dst)
{
	assert(inverseLut!=NULL);
	// Assert for only 8UC1 input images (colorCodes)
	assert(src.type() == CV_8UC1);
	// Assert for only 8UC3 output images (BGR)
	assert(dst.type() == CV_8UC3);
	// Assert dst has same size as src
	assert(src.cols == dst.cols);
	assert(src.rows == dst.rows);

	uchar colorCode;
	uchar *iLutPtr = NULL;
	// Go along every pixel and do the following:
	for (int row=0; row<src.rows; row++)
	{
		// Calculate pointer to the beginning of the current row
		const uchar *ptr = (const uchar *)(src.data + row*src.step);
		// Result pointer
		uchar *resultPtr = (uchar *)(dst.data + row*dst.step);
		// Go along every colorCode (uchar) colorspace pixel
		for (int col=0; col<src.cols; col++)
		{
			colorCode = *ptr;
			iLutPtr = inverseLut + 3*colorCode;
			// Generate B channel
			*resultPtr++ = iLutPtr[2];
			// Generate G channel
			*resultPtr++ = iLutPtr[1];
			// Generate R channel
			*resultPtr++ = iLutPtr[0];
			ptr++;
		}
	}
}

void LutColorFilter::InitInverseLut(uchar r, uchar g, uchar b)
{
	if (!inverseLut)
	{
		inverseLut = new uchar[3*256];
	}
	for(int i=0; i<256; i++)
	{
		inverseLut[i*3+0]=r;
		inverseLut[i*3+1]=g;
		inverseLut[i*3+2]=b;
	}
}

void LutColorFilter::SetInverseLut(uchar colorCode, uchar r, uchar g, uchar b)
{
	assert(inverseLut != NULL);
	inverseLut[colorCode*3+0]=r;
	inverseLut[colorCode*3+1]=g;
	inverseLut[colorCode*3+2]=b;
}
