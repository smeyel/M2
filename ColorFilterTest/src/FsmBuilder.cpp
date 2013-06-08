#include <memory.h>	// for memset
#include <cstddef>	// for NULL
#include <assert.h>

#include "FsmBuilder.h"

FsmBuilder::FsmBuilder()
{
	transitions = NULL;
	maxStateNumber = 0;
	maxInputNumber = 0;
	currentStateNumber = 0;
	currentInputNumber = 0;
}

FsmBuilder::FsmBuilder(int maxStateNumber, int maxInputNumber, int defaultNextState)
{
	init(maxStateNumber,maxInputNumber,defaultNextState);
}

FsmBuilder::~FsmBuilder()
{
	if (transitions)
	{
		delete transitions;
	}
	transitions = NULL;
	maxStateNumber = 0;
	maxInputNumber = 0;
	currentStateNumber = 0;
	currentInputNumber = 0;
}

void FsmBuilder::init(int maxStateNumber, int maxInputNumber, int defaultNextState)
{
	if (transitions)
	{
		delete transitions;
	}
	this->maxStateNumber = maxStateNumber;
	this->maxInputNumber = maxInputNumber;
	currentStateNumber = 0;
	currentInputNumber = 0;
	transitions = new unsigned int[maxStateNumber*maxInputNumber];
	all(defaultNextState);
}

/** Set an element of the transition matrix */
void FsmBuilder::setNextState(unsigned int state, unsigned int input, unsigned int nextState)
{
	assert(state<maxStateNumber);
	assert(input<maxInputNumber);
	assert(nextState<maxStateNumber);

	if(state>currentStateNumber)
		currentStateNumber = state;
	if(nextState>currentStateNumber)
		currentStateNumber = nextState;
	if(input>currentInputNumber)
		currentInputNumber = input;

	int idx = state*maxStateNumber + input;
	transitions[idx]=nextState;
}

/** Defines trap state and resets all matrix entries to that.
*/
void FsmBuilder::all(unsigned int nextState)
{
	// Calling setNextState() allows centralized asserts and
	//	administration of current state and input number.
	for(int s=0; s<maxStateNumber; s++)
		for(int i=0; i<maxInputNumber; i++)
			setNextState(s, i, nextState);
}

void FsmBuilder::setDefaultForState(unsigned int state, unsigned int defaultNextState)
{
	for(int i=0; i<maxInputNumber; i++)
		setNextState(state, i, defaultNextState);
}

void FsmBuilder::setDefaultForInput(unsigned int input, unsigned int defaultNextState)
{
	for(int s=0; s<maxStateNumber; s++)
		setNextState(s, input, defaultNextState);
}

void FsmBuilder::copyState(unsigned int srcState, unsigned int dstState)
{
	int srcIdx, dstIdx;
	for(int i=0; i<maxInputNumber; i++)
	{
		srcIdx = srcState*maxStateNumber+i;
		dstIdx = dstState*maxStateNumber+i;
		transitions[dstIdx] = transitions[srcIdx];
	}
}

void FsmBuilder::setCounterState(unsigned int startState, unsigned int maxCount)
{
	for(int i=0; i<maxCount; i++)
	{
		copyState(startState, startState+i);
	}
}

void FsmBuilder::setCounterInput(unsigned int startState, unsigned int maxCount, unsigned int input, unsigned int fallbackState)
{
	assert(maxCount>1);
	for(int i=0; i<maxCount-1; i++)
	{
		setNextState(startState+i, input, startState+i+1);
	}
	setNextState(startState+maxCount-1, input, fallbackState);
}

unsigned int *FsmBuilder::createFsmTransitionMatrix(int &stateNumber, int &inputNumber)
{
	// Creates an array corresponding the real number of used states and inputs.
	stateNumber = currentStateNumber;
	inputNumber = currentInputNumber;
	unsigned int *result = new unsigned int[stateNumber*inputNumber];
	int srcIdx, dstIdx;
	for(int s=0; s<stateNumber; s++)
	{
		for(int i=0; i<inputNumber; i++)
		{
			srcIdx = s*maxStateNumber+i;
			dstIdx = s*stateNumber+i;
			result[dstIdx] = transitions[srcIdx]*currentStateNumber;	// IDX-encoding
		}
	}
	return result;
}

unsigned int FsmBuilder::getIdxOfStateID(int stateID)
{
	return stateID*currentStateNumber;
}
