#ifndef __TIMEMEASUREMENTCODEDEFINES_H_
#define __TIMEMEASUREMENTCODEDEFINES_H_

#include "TimeMeasurement.h"
using namespace LogConfigTime;

namespace CamClient
{
	class TimeMeasurementCodeDefs
	{
	public:
		const static int FrameAll			= 0;	// Processing a frame
		const static int ReceiveCommand		= 1;	// All types of commands (!)
		const static int Capture			= 2;	// Capturing a frame from the input source
		const static int ShowImage			= 3;	// 
		const static int JpegCompression	= 4;	// 
		const static int AnsweringTakePicture	= 5;	// 
		const static int AnsweringSendlog		= 6;	// 
		const static int FullExecution		= 7;	// 

		static void setnames(TimeMeasurement *measurement)
		{
			measurement->setMeasurementName("Main loop");

			measurement->setname(FrameAll,"FrameAll");
			measurement->setname(ReceiveCommand,"ReceiveCommand");
			measurement->setname(Capture,"Capture");
			measurement->setname(ShowImage,"ShowImage");
			measurement->setname(JpegCompression,"JpegCompression");
			measurement->setname(AnsweringTakePicture,"AnsweringTakePicture");
			measurement->setname(AnsweringSendlog,"AnsweringSendlog");
			measurement->setname(FullExecution,"FullExecution");
		}
	};
}

#endif
