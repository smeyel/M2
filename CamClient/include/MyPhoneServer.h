#ifndef __MYPHONESERVER_H
#define __MYPHONESERVER_H
#include "PhoneServer.h"
#include "CameraLocalProxy.h"
#include "myconfigmanager.h"

class MyPhoneServer : public PhoneServer
{
	/** Number of images captured.
		Incremented upon taking a picture.
		Reported in measurement log.
	*/
	int imageNumber;
	CameraLocalProxy *camProxy;

	class TimeMeasurementCodeDefs
	{
	public:
		// This class may use ID-s >20
		const static int ImageCapture			= 21;
		const static int JpegCompression		= 22;
		const static int ShowImage				= 23;

		static void setnames(TimeMeasurement *measurement)
		{
			measurement->setMeasurementName("CamClient internal time measurements");

			measurement->setname(ImageCapture,"CamClient-ImageCapture");
			measurement->setname(JpegCompression,"CamClient-JpegCompression");
			measurement->setname(ShowImage,"CamClient-ShowImage");
		}
	};

	static const char *imageWindowName;

public:
	/** @warning Do not forget to initialize! */
	MyConfigManager configManager;

	MyPhoneServer()
	{
		imageNumber=0;
		TimeMeasurementCodeDefs::setnames(&timeMeasurement);
	}

	~MyPhoneServer()
	{
		if (camProxy)
		{
			delete camProxy;
			camProxy = NULL;
		}
	}

	void init(char *inifilename, int argc, char **argv);

	/** Ping callback */
	virtual JsonMessage *PingCallback(PingMessage *msg);

	/** TakePicture callback */
	virtual JsonMessage *TakePictureCallback(TakePictureMessage *msg);

	/** SendPosition callback */
	virtual JsonMessage *SendPositionCallback(SendPositionMessage *msg);

	/** SendLog callback */
	virtual JsonMessage *SendLogCallback(SendlogMessage *msg);
};




#endif