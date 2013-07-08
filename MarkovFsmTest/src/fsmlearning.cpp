#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "MyLutColorFilter.h"
#include "StdoutLogger.h"

using namespace cv;
using namespace LogConfigTime;
using namespace smeyel;

#include "FsmLearner.h"

void test_graphOpt()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);
	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();
	FsmLearner *fsmlearner = new FsmLearner(8,3,COLORCODE_NONE);

	vector<string> inputValueNames(7);
	inputValueNames[COLORCODE_BLK]=string("BLK");
	inputValueNames[COLORCODE_WHT]=string("WHT");
	inputValueNames[COLORCODE_RED]=string("RED");
	inputValueNames[COLORCODE_GRN]=string("GRN");
	inputValueNames[COLORCODE_BLU]=string("BLU");
	inputValueNames[COLORCODE_NONE]=string("NON");

	Mat src(50,50,CV_8UC3);
	Mat lut(50,50,CV_8UC1);
	Mat lutVis(50,50,CV_8UC3);
	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,25),Scalar(255,0,0));
	rectangle(src,Point2d(25,26),Point2d(30,49),Scalar(0,255,0));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	waitKey(0);
	fsmlearner->addImage(lut, true);

	//cout << "------------- ON train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,49),Scalar(0,0,255));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	waitKey(0);
	fsmlearner->addImage(lut, false);

	//cout << "------------- OFF train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);
	fsmlearner->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);
	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	//	stat->findClassifierSequences(callback);
	unsigned int valuesA[] = {COLORCODE_BLK,COLORCODE_GRN,COLORCODE_BLK};
	unsigned int valuesB[] = {COLORCODE_BLK,COLORCODE_BLU,COLORCODE_BLK};
	SequenceCounterTreeNode *nodeA = fsmlearner->counterTreeRoot->getNode(valuesA,2,false);
	SequenceCounterTreeNode *nodeB = fsmlearner->counterTreeRoot->getNode(valuesB,2,false);

	//bool canCombine = stat->checkCanCombineNodes(nodeA,nodeB,0.7F);

	//stat->combineNodes(nodeA,nodeB);

	//checkNodePair(stat,nodeA,nodeB,0.7F);
	vector<SequenceCounterTreeNode *> *allNodes = new vector<SequenceCounterTreeNode *>();
	vector<SequenceCounterTreeNode *> *newAllNodes = new vector<SequenceCounterTreeNode *>();

	bool running=true;
	while (running)
	{
		allNodes->clear();
		fsmlearner->collectNodesBackwards(allNodes,fsmlearner->counterTreeRoot);

		pair<SequenceCounterTreeNode *,SequenceCounterTreeNode *> toCombine =
			fsmlearner->checkAllCombinationsForMerge(fsmlearner,allNodes,0.7F);
		if (toCombine.first==NULL || toCombine.second==NULL)
		{
			running=false;
			break;
		}

		int idA = toCombine.first->getNodeID();
		int idB = toCombine.second->getNodeID();
		cout << "------------- combining nodes... -------------" << endl;
		cout << "COMBING: " << toCombine.first->getNodeID() << " and " << toCombine.second->getNodeID() << endl;
		fsmlearner->combineNodes(toCombine.first,toCombine.second);

		// TODO: delete should be done inside combineNodes!!!
		newAllNodes->clear();
		fsmlearner->collectNodesBackwards(newAllNodes,fsmlearner->counterTreeRoot);
		fsmlearner->deleteRemovedNodes(allNodes,newAllNodes);

		fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);
	}

	delete allNodes;
	allNodes=NULL;

	//stat->optimizeGraph();

	cout << "------------- combining nodes... -------------" << endl;
	fsmlearner->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);



	cout << "done" << endl;

}

