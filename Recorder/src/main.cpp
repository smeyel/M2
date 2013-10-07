#include <iostream>	// for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "CameraRemoteProxy.h"
#include "CameraLocalProxy.h"

using namespace std;
using namespace cv;

void help()
{
	cout
		<< "\n--------------------------------------------------------------------------" << endl
		<< "This program captures video from local camera to file."
		<< "--------------------------------------------------------------------------"   << endl
		<< endl;
}

void setAllCamParams(CameraProxy *proxy[], int cameraNumber, int exposure, int gain, int wb_red, int wb_green, int wb_blue)
{
	for (int camIdx=0; camIdx<cameraNumber; camIdx++)
	{
		proxy[camIdx]->SetNormalizedExposure(exposure);
		proxy[camIdx]->SetNormalizedGain(gain);
		proxy[camIdx]->SetNormalizedWhiteBalance(wb_red, wb_green, wb_blue);
	}
}

// Meant to record video from multiple cameras. Later, should be able to use CameraProxy and save images with timestamp for proper re-playing.
// Will support even multiple local and/or multiple remote cameras.
// Later todo: make a component like logging where the measurement host can push the just retrieved images and their timestamps.
// Record to multiple files by first capturing into memory and then saving into AVI at once. (Pre-allocate many buffer Mat-s)
int main(int argc, char *argv[], char *window_name)
{
	const int cameraNumber = 2;
	const bool isCameraLocal = true;
	CameraProxy *camProxy[cameraNumber];
	string filename[cameraNumber];
	VideoWriter *outputVideo[cameraNumber];
	Mat *frame[cameraNumber];

	if (!isCameraLocal)
	{
		std::cout << "Waiting 3s for remote cameras to settle..." << endl;
#ifdef WIN32
	Sleep(3000);
#else
#error TODO: Sleep not implemented for non-Win32.
#endif
	}

	for (int camIdx=0; camIdx<cameraNumber; camIdx++)
	{
		if (isCameraLocal)
		{
			camProxy[camIdx] = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,camIdx);
		}
		else
		{
			CameraRemoteProxy *remoteProxy = new CameraRemoteProxy();
			remoteProxy->Connect("127.0.0.1",6000+camIdx);
			camProxy[camIdx] = remoteProxy;
		}

		/*Size S = Size((int) inputVideo.get(CV_CAP_PROP_FRAME_WIDTH),    //Acquire input size
					  (int) inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT));*/
		Size S = Size(640,480);

		stringstream ss;
		ss.clear();
		ss << "recorded_cam" << camIdx << ".avi";
		filename[camIdx] = ss.str();

		outputVideo[camIdx] = new VideoWriter(filename[camIdx],CV_FOURCC('M','J','P','G'), 25.0, S, true);
		//outputVideo[camIdx]->open(filename[i], CV_FOURCC('M','J','P','G'), 25.0, S, true);

		if (!outputVideo[camIdx]->isOpened())
		{
			std::cout  << "Could not open the output video for write: " << filename[camIdx] << endl;
			return -1;
		}

		std::cout << "Camera " << camIdx << " records to file: " << ss.str() << endl;

		namedWindow(filename[camIdx].c_str(), CV_WINDOW_AUTOSIZE);
		frame[camIdx] = new Mat(480,640,CV_8UC4);
	}

	std::cout << "Init complete, starting recording..." << endl;

	int framecounter = 0;	// 0-based frame counter
	bool finished = false;
	while(!finished) //Show the image captured in the window and repeat
	{
		cout << "Frame " << framecounter << endl;

		for(int camIdx=0; camIdx<cameraNumber; camIdx++)
		{
			camProxy[camIdx]->CaptureImage(0,frame[camIdx]);
			imshow(filename[camIdx].c_str(),*frame[camIdx]);

			*outputVideo[camIdx] << *frame[camIdx];
		}

		char ch = waitKey(25);
		switch(ch)
		{
		case '1':
			setAllCamParams(camProxy,cameraNumber,1,1,30,30,30);
			cout << "Exposure set" << endl;
			break;
		case '2':
			setAllCamParams(camProxy,cameraNumber,5,1,30,30,30);
			cout << "Exposure set" << endl;
			break;
		case '3':
			setAllCamParams(camProxy,cameraNumber,10,1,30,30,30);
			cout << "Exposure set" << endl;
			break;
		case 27:
			finished=true;
			break;
		}

		framecounter++;
	}

	if (!isCameraLocal)
	{
		for(int camIdx=0; camIdx<cameraNumber; camIdx++)
		{
			CameraRemoteProxy *rp = (CameraRemoteProxy *)camProxy[camIdx];
			rp->Disconnect();
		}
	}

	cout << endl << "Capture complete." << endl;
	return 0;
}
