#include "CameraProxy.h"

#include "Logger.h"
#include "JpegMessage.h"
#include "MatImageMessage.h"

using namespace LogConfigTime;

CameraProxy::CameraProxy(PhoneProxy *aPhoneProxy, Camera *aCamera)
{
	phoneproxy = aPhoneProxy;
	camera = aCamera;

	initDefaults();
}

CameraProxy::CameraProxy()
{
	initDefaults();
}

void CameraProxy::initDefaults()
{
	lastImageTaken = new Mat();

	phoneproxy = default_phoneproxy = new PhoneProxy();
	camera = default_camera = new Camera();
	chessboarddetector = default_chessboarddetector = new ChessboardDetector(Size(9,6),36.1);
}


CameraProxy::~CameraProxy()
{
	if (lastImageTaken)
		delete lastImageTaken;
	lastImageTaken=NULL;

	if (phoneproxy == default_phoneproxy)
		phoneproxy = NULL;
	if (default_phoneproxy)
		delete default_phoneproxy;
	default_phoneproxy=NULL;

	if (camera == default_camera)
		camera = NULL;
	if (default_camera)
		delete default_camera;
	default_camera=NULL;

	if (chessboarddetector == default_chessboarddetector)
		chessboarddetector = NULL;
	if (default_chessboarddetector)
		delete default_chessboarddetector;
	default_chessboarddetector=NULL;
}

void CameraProxy::CaptureImage()
{
	CaptureImage(lastImageTaken);
}

// CaptureImage
void CameraProxy::CaptureImage(Mat *target)
{
	// Request image from the phone
	phoneproxy->RequestPhoto(0);
	// Receiving picture
	JsonMessage *msg = NULL;
	bool isImgValid = false;
	msg = phoneproxy->ReceiveNew();
	if (msg->getMessageType() == Jpeg)
	{
		JpegMessage *jpegMsg = NULL;
		jpegMsg = (JpegMessage *)msg;
		jpegMsg->Decode(target);
			
		if(target->type()==CV_8UC4)	// Convert frames from CV_8UC4 to CV_8UC3
			cvtColor(*target,*target,CV_BGRA2BGR);

		isImgValid = true;
	}
	else if (msg->getMessageType() == MatImage)
	{
		MatImageMessage *matimgMsg = NULL;
		matimgMsg = (MatImageMessage *)msg;
		if (matimgMsg->size != 0)
		{
			matimgMsg->Decode();
			matimgMsg->getMat()->copyTo(*target);	// TODO: avoid this copy...
			isImgValid = true;
		}
	}
	else
	{
		cout << "Error... received something else than JPEG or MAT... see the log for details!" << endl;
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
		msg->log();
	}
	delete msg;
	msg = NULL;
}

// Capture image and try to find chessboard
bool CameraProxy::CaptureAndTryCalibration(bool showResultOnImage)
{
	CaptureImage();
	return TryCalibration(showResultOnImage);
}

bool CameraProxy::TryCalibration(bool showResultOnImage)
{
	return TryCalibration(lastImageTaken, showResultOnImage);
}

bool CameraProxy::TryCalibration(Mat *image, bool showResultOnImage)
{
	if (chessboarddetector->findChessboardInFrame(*image))
	{
		camera->calculateExtrinsicParams(chessboarddetector->chessboard.corners,chessboarddetector->pointBuf);

		// Show calibration data on the frame
		if (showResultOnImage)
		{
			drawChessboardCorners(*image,Size(9,6),chessboarddetector->pointBuf,true);
			char txt[50];
			Matx44f T = camera->GetT();
			for(int i=0; i<16; i++)
			{
				sprintf(txt, "%4.2lf",T.val[i] );
				putText( *lastImageTaken, string(txt), cvPoint( 25+(i%4)*75, 20+(i/4)*20 ), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,0) );
			}
		}
		return true;
	}
	return true;
}


// Ask for images until calibration is successful
bool CameraProxy::CaptureUntilCalibrated(int maxFrameNum)
{
	for(int i=0; i<maxFrameNum; i++)
	{
		if (CaptureAndTryCalibration())
			return true;
	}
	return false;
}

// Return Ray3D for given image coordinates

