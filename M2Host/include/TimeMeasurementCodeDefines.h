#ifndef __TIMEMEASUREMENTCODEDEFINES_H_
#define __TIMEMEASUREMENTCODEDEFINES_H_

#include "TimeMeasurement.h"

using namespace LogConfigTime;

namespace M2
{
	class TimeMeasurementCodeDefs
	{
	public:
		const static int FrameAll			= 0;	// Processing a frame
		const static int Send				= 1;	// Send image request and wait for reception
		const static int WaitAndReceive		= 2;	// Receiving the image
		const static int Chessboard			= 3;	// Detection of chessboard (not always successful)
		const static int Tracking			= 4;	// Marker detection
		const static int FullExecution		= 5;	// Full execution of the program

		static void setnames(TimeMeasurement *measurement)
		{
			measurement->setMeasurementName("M2Host main loop");

			measurement->setname(FrameAll,"FrameAll");
			measurement->setname(Send,"Send");
			measurement->setname(WaitAndReceive,"WaitAndReceive");
			measurement->setname(Chessboard,"Chessboard");
			measurement->setname(Tracking,"Tracking");
			measurement->setname(FullExecution,"FullExecution");
		}
	};
}

#endif
