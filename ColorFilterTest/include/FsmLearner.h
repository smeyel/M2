#ifndef __FSMLEARNER_H
#define __FSMLEARNER_H
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)

#include "LutColorFilter.h"
#include "FsmBuilder.h"

#define MAXNODECOUNTERNUM	5
#define COUNTERIDX_ON		1
#define COUNTERIDX_OFF		0

namespace smeyel
{
	class SequenceCounterTreeNode
	{
		/** Array of child node pointers. */
		SequenceCounterTreeNode **children;
		SequenceCounterTreeNode *parent;
		int inputValueNumber;
		int counter[MAXNODECOUNTERNUM];
	public:

		SequenceCounterTreeNode(const int inputValueNumber, SequenceCounterTreeNode* parentNode)
		{
			children = new SequenceCounterTreeNode*[inputValueNumber];
			for(int i=0; i<inputValueNumber; i++)
			{
				children[i]=NULL;
			}
			parent=parentNode;
			for(int i=0; i<MAXNODECOUNTERNUM; i++)
			{
				counter[i]=0;
			}
			this->inputValueNumber = inputValueNumber;
		}

		~SequenceCounterTreeNode()
		{
			for(int i=0; i<inputValueNumber; i++)
			{
				if (children[i]!=NULL)
				{
					delete children[i];
				}
			}
		}

		SequenceCounterTreeNode *getChildNode(const unsigned int inputValue)
		{
			if (children[inputValue] == NULL)
			{
				children[inputValue] = new SequenceCounterTreeNode(inputValueNumber,this);
			}
			return children[inputValue];
		}

		SequenceCounterTreeNode *getParentNode()
		{
			return this->parent;
		}

		/**
			Nodes are created on demand, return value is never NULL.
		*/
		SequenceCounterTreeNode *getNode(const unsigned int *inputValues, const int numberOfValues)
		{
			if (numberOfValues == 0)
			{
				return this;
			}
			if (numberOfValues == 1)
			{
				// Return direct child node (may be NULL)
				return getChildNode(*inputValues);
			}
			// Recursive call
			return getChildNode(*inputValues)->getNode(inputValues+1,numberOfValues-1);
		}

		void incrementCounter(int counterIdx)
		{
			OPENCV_ASSERT(counterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.incrementCounter","Counter IDX > max!");
			OPENCV_ASSERT(counterIdx>=0,"SequenceCounterTreeNode.incrementCounter","Counter IDX negative!");
			counter[counterIdx]++;
		}

		int getCounter(int counterIdx)
		{
			OPENCV_ASSERT(counterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.getCounter","Counter IDX > max!");
			OPENCV_ASSERT(counterIdx>=0,"SequenceCounterTreeNode.getCounter","Counter IDX negative!");
			return counter[counterIdx];
		}

		int getSubtreeSumCounter(int counterIdx)
		{
			OPENCV_ASSERT(counterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.getSubtreeSumCounter","Counter IDX > max!");
			OPENCV_ASSERT(counterIdx>=0,"SequenceCounterTreeNode.getSubtreeSumCounter","Counter IDX negative!");
			int sum = counter[counterIdx];
			for(int i=0; i<inputValueNumber; i++)
			{
				// Cannot use getChildNode() as that would create non-existing nodes infinite long...
				if (children[i]!=NULL)
				{
					sum += children[i]->getSubtreeSumCounter(counterIdx);
				}
			}
			return sum;
		}

		/** Overwrites counter of current node with sum of children, except if there are no children (or sum is 0). */
		int getAndStoreSubtreeSumCounter(int counterIdx)
		{
			OPENCV_ASSERT(counterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.getAndStoreSubtreeSumCounter","Counter IDX > max!");
			OPENCV_ASSERT(counterIdx>=0,"SequenceCounterTreeNode.getAndStoreSubtreeSumCounter","Counter IDX negative!");

			int childrenSum=0;
			for(int i=0; i<inputValueNumber; i++)
			{
				// Cannot use getChildNode() as that would create non-existing nodes infinite long...
				if (children[i]!=NULL)
				{
					childrenSum += children[i]->getAndStoreSubtreeSumCounter(counterIdx);
				}
			}

			if (childrenSum>0)
			{
				counter[counterIdx] = childrenSum;
			}

			return counter[counterIdx];
		}

		void writeIndent(int indent)
		{
			for(int i=0; i<indent; i++)
				cout << " ";
		}

		void showRecursive(int indent, int maxCounterIdx, bool showNullChildren)
		{
			OPENCV_ASSERT(maxCounterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.showRecursive","Max counter IDX > max!");
			OPENCV_ASSERT(maxCounterIdx>=0,"SequenceCounterTreeNode.showRecursive","Max counter IDX negative!");
			writeIndent(indent);
			cout << "SequenceCounterTreeNode local counters:";
			for(int i=0; i<=maxCounterIdx; i++)
			{
				cout << " #" << i << "=" << counter[i];
			}
			cout << endl;
			// show children
			for(int i=0; i<inputValueNumber; i++)
			{
				if (children[i])
				{
					writeIndent(indent);
					cout << "Child " << i << ":" << endl;
					children[i]->showRecursive(indent+1,maxCounterIdx,showNullChildren);
				}
				else
				{
					if (showNullChildren)
					{
						writeIndent(indent);
						cout << "Child " << i << ": NULL" << endl;
					}
				}
			}
		}

		void showCompactRecursive(int indent, int maxCounterIdx)
		{
			OPENCV_ASSERT(maxCounterIdx<MAXNODECOUNTERNUM,"SequenceCounterTreeNode.showCompactRecursive","Max counter IDX > max!");
			OPENCV_ASSERT(maxCounterIdx>=0,"SequenceCounterTreeNode.showCompactRecursive","Max counter IDX negative!");
			for(int i=0; i<=maxCounterIdx; i++)
			{
				cout << counter[i];
				if (i<maxCounterIdx)
				{
					cout << "/";
				}
			}
			cout << endl;
			// show children
			for(int i=0; i<inputValueNumber; i++)
			{
				if (children[i])
				{
					writeIndent(indent);
					cout << i << ": ";
					children[i]->showCompactRecursive(indent+1,maxCounterIdx);
				}
			}
		}

	};

	/** Used to calculate a transition probability statistic from a sequence of values. */
	class TransitionStat
	{
		/** Array containing the last input values. Length corresponds the order of the markov chain. */
		unsigned int *lastValues;
		/** Input values considered before the first input. */
		unsigned int initialValue;
		/** The number of possible input values, also the maximal input value + 1. */
		unsigned int inputValueNumber;
		/** The length of history taken into account (order of Markov Chain) */
		unsigned int markovChainOrder;

		/** Used to decide whether initial values are still in the lastValues array. */
		unsigned int valueNumberSinceSequenceStart;

	public: // for DEBUG
		/** Root node of counter tree */
		SequenceCounterTreeNode *counterTreeRoot;

	public:
		/**
			@param inputValueNumber	The number of possible input values, also the maximal input value + 1.
			@param markovChainOrder	The length of history taken into account (order of Markov Chain)
			@param initialValue		Inputs prior to first added value are considered to be this value.
				Similar to assumed color of pixels outside the image boundaries.
		*/
		TransitionStat(const unsigned int inputValueNumber, const unsigned int markovChainOrder, const unsigned int initialValue)
		{
			counterTreeRoot = new SequenceCounterTreeNode(inputValueNumber,NULL);
			lastValues = new unsigned int[markovChainOrder];
			this->inputValueNumber = inputValueNumber;
			this->markovChainOrder = markovChainOrder;
			this->initialValue = initialValue;
			startNewSequence();
		}

		~TransitionStat()
		{
			delete counterTreeRoot;
			delete lastValues;
			counterTreeRoot = NULL;
			lastValues = NULL;
		}

		void startNewSequence()
		{
			for(unsigned int i=0; i<markovChainOrder; i++)
				lastValues[i]=initialValue;
			valueNumberSinceSequenceStart=0;
		}

		/**	Adds a new value to the sequence.
			Call this function with every input sequence element to create the statistic.
			@param	inputValue	The value of the next element of the sequence the statistic is based on.
			@param	isTargetArea	Shows whether the current element is inside a target area or not.
		*/
		void addValue(const unsigned int inputValue, const bool isTargetArea)
		{
			OPENCV_ASSERT(inputValue<inputValueNumber,"TransitionStat.addValue","value > max!");
			valueNumberSinceSequenceStart++;
			// Update history (lastValues)
			for(unsigned int i=0; i<markovChainOrder-1; i++)
				lastValues[i]=lastValues[i+1];
			lastValues[markovChainOrder-1] = inputValue;

			// Increment the counter only if initial values are already shifted out of the lastValues array.
			if (valueNumberSinceSequenceStart>=markovChainOrder)
			{
				SequenceCounterTreeNode *node = NULL;
				node = counterTreeRoot->getNode(lastValues,markovChainOrder);

				// Increment respective counter
				node->incrementCounter(isTargetArea ? COUNTERIDX_ON : COUNTERIDX_OFF);
			}
		}
	};

	/** */
	class SequenceLearner
	{
	public:
		/** Learns the important sequences a FSM has to track to find the target areas.
			The result can be used to mechanically construct the FSM. The result also indicates where the detection
				states of the FSM should be.
			@param transitionStat	The TransitionStat object containing the state transition statistics.
		*/
		void learnSequence(TransitionStat &transitionStat);

	};



	/** Creates a FSM for use in FsmFilter or LutFsmLocator.
		The FSM is learned using
		- a set of (BGR) images with binary (0, 255) target mask 
		- a pre-configured Lut for color recognition
	*/
	class FsmLearner
	{
		/** Preprocesses the training images
			Input:
			- training image
			Output:
			- LutImage
		*/
		LutColorFilter lutColorFilter;

		/** Calculates markov chain transition probabilities
			for on/off (target and non-target) areas.
			Inputs:
			- LutImage
			- TargetMask
			Output:
			- two transition-probability matrices (on/off case) MCOn, MCOff
		*/
		TransitionStat transitionStat;

		/** Learns sequences useful to distinguish between
			on/off areas.
			Input:
			- MCOn, MCOff
			Output:
			- ObservedStateSequences
		*/
		SequenceLearner sequenceLearner;

		/** Used to build the FSM to recognize the observed sequences.
			Input:
			- ObservedStateSequences
			Output:
			- internally, it will contain the ready FSM.
		*/
		FsmBuilder fsmBuilder;


	};


}


#endif
