#ifndef __TIMEMEASUREMENTCODEDEFINES_H_
#define __TIMEMEASUREMENTCODEDEFINES_H_

#include "TimeMeasurement.h"
using namespace LogConfigTime;

namespace CamClient
{
	class TimeMeasurementCodeDefs
	{
	public:
		const static int ReceiveCommand		= 1;	// Just the network part, no processing
		const static int ReadJson			= 2;	// Interpretation of the command
		const static int HandleJson			= 3;
		const static int HandleTakePicture	= 4;
		const static int WaitForTimestamp	= 5;
		const static int Capture			= 6;	// Capturing a frame from the input source
		const static int JpegCompression	= 7;	// 
		const static int AnsweringTakePicture	= 8;	// 
		const static int ShowImage			= 9;	// 
		const static int AnsweringSendlog	= 10;	// 

		static void setnames(TimeMeasurement *measurement)
		{
			measurement->setMeasurementName("CamClient remote command processing");

			measurement->setname(ReceiveCommand,"ReceiveCommand (+wait for command)");
			measurement->setname(ReadJson,"HandleJson.ReadJson");
			measurement->setname(HandleJson,"HandleJson");
			measurement->setname(HandleTakePicture,"HandleJson.HandleTakePicture");
			measurement->setname(WaitForTimestamp,"HandleJson.HandleTakePicture.WaitForTimestamp");
			measurement->setname(Capture,"HandleJson.HandleTakePicture.Capture");
			measurement->setname(JpegCompression,"HandleJson.HandleTakePicture.JpegCompression");
			measurement->setname(AnsweringTakePicture,"HandleJson.HandleTakePicture.AnsweringTakePicture");
			measurement->setname(ShowImage,"HandleJson.HandleTakePicture.ShowImage");
			measurement->setname(AnsweringSendlog,"AnsweringSendlog");
		}
	};
}

#endif
