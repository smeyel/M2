#ifndef __FSMLEARNER_H
#define __FSMLEARNER_H
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)

#include "LutColorFilter.h"
#include "FsmBuilder.h"

namespace smeyel
{
	class SequenceCounterTreeNode
	{
		/** Array of child node pointers. */
		SequenceCounterTreeNode **children;
		int inputValueNumber;
		int counter;
	public:

		SequenceCounterTreeNode(const int inputValueNumber)
		{
			children = new SequenceCounterTreeNode*[inputValueNumber];
			for(int i=0; i<inputValueNumber; i++)
			{
				children[i]=NULL;
			}
			counter=0;
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
				children[inputValue] = new SequenceCounterTreeNode(inputValueNumber);
			}
			return children[inputValue];
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

		void incrementCounter()
		{
			counter++;
		}

		int getCounter()
		{
			return counter;
		}

		int getSubtreeSumCounter()
		{
			int sum = counter;
			for(int i=0; i<inputValueNumber; i++)
			{
				// Cannot use getChildNode() as that would create non-existing nodes infinite long...
				if (children[i]!=NULL)
				{
					sum += children[i]->getSubtreeSumCounter();
				}
			}
			return sum;
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

		/** Root node of "on" counter tree */
		SequenceCounterTreeNode *counterTreeNodeOn;
		/** Root node of "off" counter tree */
		SequenceCounterTreeNode *counterTreeNodeOff;

	public:
		/**
			@param inputValueNumber	The number of possible input values, also the maximal input value + 1.
			@param markovChainOrder	The length of history taken into account (order of Markov Chain)
			@param initialValue		Inputs prior to first added value are considered to be this value.
				Similar to assumed color of pixels outside the image boundaries.
		*/
		TransitionStat(const unsigned int inputValueNumber, const unsigned int markovChainOrder, const unsigned int initialValue)
		{
			counterTreeNodeOn = new SequenceCounterTreeNode(inputValueNumber);
			counterTreeNodeOff = new SequenceCounterTreeNode(inputValueNumber);
			lastValues = new unsigned int[markovChainOrder];
			this->inputValueNumber = inputValueNumber;
			this->markovChainOrder = markovChainOrder;
			this->initialValue = initialValue;
			startNewSequence();
		}

		~TransitionStat()
		{
			delete counterTreeNodeOn;
			delete counterTreeNodeOff;
			delete lastValues;
			counterTreeNodeOn = NULL;
			counterTreeNodeOff = NULL;
			lastValues = NULL;
		}

		void startNewSequence()
		{
			for(int i=0; i<markovChainOrder; i++)
				lastValues[i]=initialValue;
		}

		/**	Adds a new value to the sequence.
			Call this function with every input sequence element to create the statistic.
			@param	inputValue	The value of the next element of the sequence the statistic is based on.
			@param	isTargetArea	Shows whether the current element is inside a target area or not.
		*/
		void addValue(const int inputValue, const bool isTargetArea)
		{
			// Update history (lastValues)
			for(int i=0; i<markovChainOrder-1; i++)
				lastValues[i]=lastValues[i+1];
			lastValues[markovChainOrder-1] = inputValue;

			SequenceCounterTreeNode *node = NULL;
			if (isTargetArea)
			{
				node = counterTreeNodeOn->getNode(lastValues,markovChainOrder);
			}
			else
			{
				node = counterTreeNodeOff->getNode(lastValues,markovChainOrder);
			}

			// Increment respective counter
			node->incrementCounter();
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
