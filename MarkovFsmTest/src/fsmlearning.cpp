#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write

#include "MyLutColorFilter.h"
#include "StdoutLogger.h"

using namespace cv;
using namespace LogConfigTime;
using namespace smeyel;

#include "FsmLearner.h"

void collectNodesBackwards(vector<SequenceCounterTreeNode *> *allNodes, SequenceCounterTreeNode *node)
{
	if (node==NULL)
	{
		return;
	}
	int n = node->getInputValueNumber();
	// first, recursive call
	for(int i=0; i<n; i++)
		collectNodesBackwards(allNodes, node->getChildNode(i));

	// second, add after all children are added
	for(unsigned int i=0; i<allNodes->size(); i++)
		if ((*allNodes)[i]==node)
			return;	// Already added
	allNodes->push_back(node);
}

pair<SequenceCounterTreeNode *,SequenceCounterTreeNode *>
	checkAllCombinationsForMerge(FsmLearner *stat, vector<SequenceCounterTreeNode *> *allNodes, float minPrecision)
{
	int inputValueNumber = (*allNodes)[0]->getInputValueNumber();
	int nodeNumber = allNodes->size();

	for(unsigned int a=1; a<allNodes->size(); a++)	// "a" is always the next new node
	{
		for(unsigned int b=0; b<a; b++)	// "b" is iterating on all previously checked nodes
		{
			assert(a!=b);

			// Check nodes a and b
			SequenceCounterTreeNode *nodeA = (*allNodes)[a];
			SequenceCounterTreeNode *nodeB = (*allNodes)[b];

			int idA = nodeA->getNodeID();
			int idB = nodeA->getNodeID();

			if (nodeA->getNodeID() == nodeB->getNodeID())
			{
				continue;	// May happen due to previous merges...
			}

			// They cannot be parents of each other
			if (nodeA->isParentOf(nodeB) || nodeB->isParentOf(nodeA))
			{
				continue;
			}

			cout << "--- Checking node pair: " << idA << " and " << idB << endl;

			bool canCombine = stat->checkCanCombineNodes(nodeA,nodeB,minPrecision);
			if (canCombine)
			{
				cout << "COMBINE: " << nodeA->getNodeID() << " and " << nodeB->getNodeID() << endl;
				// Do not search for further combination possibilities...
				pair<SequenceCounterTreeNode *,SequenceCounterTreeNode *> result;
				result.first = nodeA;
				result.second = nodeB;
				return result;
			}
		}
	}
	return pair<SequenceCounterTreeNode *,SequenceCounterTreeNode *>(
		(SequenceCounterTreeNode *)NULL,(SequenceCounterTreeNode *)NULL);	// No more possibilities found
}

void deleteRemovedNodes(
	vector<SequenceCounterTreeNode *> *oldNodes,
	vector<SequenceCounterTreeNode *> *newNodes)
{
	for(unsigned int i=0; i<oldNodes->size(); i++)
	{
		bool found = false;
		for(unsigned int j=0; j<newNodes->size(); j++)
		{
			if ((*newNodes)[j]==(*oldNodes)[i])
				found=true;
		}
		if (!found)
		{
			// First, disconnect all children, otherwise, delete goes on!!!
			(*oldNodes)[i]->disconnectAllChildren();
			delete (*oldNodes)[i];
			(*oldNodes)[i]=NULL;	// just to make sure...
		}
	}
}


void test_graphOpt()
{
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_WARNING);
	MyLutColorFilter *lutColorFilter = new MyLutColorFilter();
	FsmLearner *stat = new FsmLearner(8,3,COLORCODE_NONE);

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
	stat->addImage(lut, true);

	//cout << "------------- ON train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	src.setTo(Scalar(0,0,0));
	rectangle(src,Point2d(25,0),Point2d(30,49),Scalar(0,0,255));
	lutColorFilter->Filter(&src,&lut,NULL);
	lutColorFilter->InverseLut(lut,lutVis);
	imshow("TestImage",src);
	imshow("LUT",lutVis);
	waitKey(0);
	stat->addImage(lut, false);

	//cout << "------------- OFF train -------------" << endl;
	//stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_ON);
	stat->counterTreeRoot->calculateSubtreeCounters(COUNTERIDX_OFF);
	stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);

	//	stat->findClassifierSequences(callback);
	unsigned int valuesA[] = {COLORCODE_BLK,COLORCODE_GRN,COLORCODE_BLK};
	unsigned int valuesB[] = {COLORCODE_BLK,COLORCODE_BLU,COLORCODE_BLK};
	SequenceCounterTreeNode *nodeA = stat->counterTreeRoot->getNode(valuesA,2,false);
	SequenceCounterTreeNode *nodeB = stat->counterTreeRoot->getNode(valuesB,2,false);

	//bool canCombine = stat->checkCanCombineNodes(nodeA,nodeB,0.7F);

	//stat->combineNodes(nodeA,nodeB);

	//checkNodePair(stat,nodeA,nodeB,0.7F);
	vector<SequenceCounterTreeNode *> *allNodes = new vector<SequenceCounterTreeNode *>();
	vector<SequenceCounterTreeNode *> *newAllNodes = new vector<SequenceCounterTreeNode *>();

	bool running=true;
	while (running)
	{
		allNodes->clear();
		collectNodesBackwards(allNodes,stat->counterTreeRoot);

		pair<SequenceCounterTreeNode *,SequenceCounterTreeNode *> toCombine =
			checkAllCombinationsForMerge(stat,allNodes,0.7F);
		if (toCombine.first==NULL || toCombine.second==NULL)
		{
			running=false;
			break;
		}

		int idA = toCombine.first->getNodeID();
		int idB = toCombine.second->getNodeID();
		cout << "------------- combining nodes... -------------" << endl;
		cout << "COMBING: " << toCombine.first->getNodeID() << " and " << toCombine.second->getNodeID() << endl;
		stat->combineNodes(toCombine.first,toCombine.second);

		// TODO: delete should be done inside combineNodes!!!
		newAllNodes->clear();
		collectNodesBackwards(newAllNodes,stat->counterTreeRoot);
		deleteRemovedNodes(allNodes,newAllNodes);


		stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);
	}

	delete allNodes;
	allNodes=NULL;

	//stat->optimizeGraph();

	cout << "------------- combining nodes... -------------" << endl;
	stat->counterTreeRoot->showCompactRecursive(0,1,&inputValueNames);



	cout << "done" << endl;

}

