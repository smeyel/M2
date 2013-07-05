#ifndef __FSMLEARNER_H
#define __FSMLEARNER_H
#include "TransitionStat.h"

namespace smeyel
{
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
