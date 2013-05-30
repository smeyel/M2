#include <iostream>	// for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

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

// Means to record video from multiple cameras. Later, should be able to use CameraProxy and save images with timestamp for proper re-playing.
int main(int argc, char *argv[], char *window_name)
{
	//help();
	/*if (argc != 5)
	{
		cout << "Parameters: input file, output file, first frame, last frame" << endl;
		return -1;
	}*/

	const string target = "test.avi";//argv[2];            // the target file name

/*	const string source = "d:\\SMEyeL\\inputmedia\\MultiViewVideo1\\2011-10-01-017.mp4";
	const string target = "d:\\SMEyeL\\inputmedia\\MultiViewVideo1\\part_2011-10-01-017.avi";
	int firstframe = 200;
	int lastframe = 500;*/

	//cout << "Copy video" << endl << "   " << source << endl << "to" << endl << "   " << target << endl << " from frame " << firstframe << " until frame " << lastframe << endl;

	VideoCapture inputVideo(0);        // Open input
	if ( !inputVideo.isOpened())
	{
		cout  << "Could not open the input video source." << endl;
		return -1;
	}

	//int ex = static_cast<int>(inputVideo.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

	// Transform from int to char via Bitwise operators
	//char EXT[] = {ex & 0XFF , (ex & 0XFF00) >> 8,(ex & 0XFF0000) >> 16,(ex & 0XFF000000) >> 24, 0};

	Size S = Size((int) inputVideo.get(CV_CAP_PROP_FRAME_WIDTH),    //Acquire input size
				  (int) inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT));

	VideoWriter outputVideo;                                        // Open the output
	//outputVideo.open(string(target), CV_FOURCC('M','J','P','G'), inputVideo.get(CV_CAP_PROP_FPS),S, true);
	outputVideo.open(string(target), CV_FOURCC('M','J','P','G'), 25.0 ,S, true);

	if (!outputVideo.isOpened())
	{
		cout  << "Could not open the output video for write: " << target << endl;
		return -1;
	}

	union { int v; char c[5];} uEx ;
	//uEx.v = ex;                              // From Int to char via union
	//uEx.c[4]='\0';

	cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
		<< " of nr#: " << inputVideo.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	//cout << "Input codec type: " << EXT << endl;

	//namedWindow("Video", CV_WINDOW_AUTOSIZE);

	Mat src;
	int framecounter = 0;	// 0-based frame counter
	while(true) //Show the image captured in the window and repeat
	{
		inputVideo >> src;              // read
		if( src.empty()) 
		{
			cout << "No more input frames..." << endl;
		}
		imshow("Video",src);
		cout << "Frame " << framecounter;


		outputVideo << src;

		framecounter++;

		char ch = waitKey(25);
		if (ch==27)
		{
			break;
		}
	}

	cout << endl << "Capture complete." << endl;
	return 0;
}
