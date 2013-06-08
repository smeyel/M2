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

	nextFreeIdx = 0;
	currentIdx = 0;
	currentMaxCount = 0;
}

/** Set an element of the transition matrix */
void FsmBuilder::setNextState(unsigned int state, unsigned int input, unsigned int nextState)
{
	assert(state<maxStateNumber);
	assert(input<maxInputNumber);
	assert(nextState<maxStateNumber);

	if(state>currentStateNumber)
		currentStateNumber = state;
	if(nextState>=currentStateNumber)
		currentStateNumber = nextState+1;
	if(input>=currentInputNumber)
		currentInputNumber = input+1;

	int idx = state*maxStateNumber + input;
	transitions[idx]=nextState;
}

/** Defines trap state and resets all matrix entries to that.
*/
void FsmBuilder::all(unsigned int nextState)
{
	// Calling setNextState() would confuse input number...
	for(int s=0; s<maxStateNumber; s++)
		for(int i=0; i<maxInputNumber; i++)
		{
			int idx = s*maxStateNumber + i;
			transitions[idx]=nextState;
		}
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

void FsmBuilder::newState(unsigned int id, unsigned int maxCount)
{
	currentIdx = nextFreeIdx;
	currentMaxCount = maxCount;
	stateId2IdxMap[id]=currentIdx;
	stateId2MaxcountMap[id]=currentMaxCount;
	nextFreeIdx += maxCount;
	assert(nextFreeIdx<maxStateNumber);
}

/** Select given ID as current state. */
void FsmBuilder::selectState(unsigned int id)
{
	currentIdx = stateId2IdxMap[id];
	currentMaxCount = stateId2MaxcountMap[id];
}

void FsmBuilder::setDefault()
{
	all(currentIdx);
}

void FsmBuilder::setCurrentColor(unsigned char r, unsigned char g, unsigned char b)
{
	// TODO: go along every state of current set and set InverseLUT color
	RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	stateIdx2ColorMap[currentIdx]=rgb;
}

/** Set transition from current state. */
void FsmBuilder::setTransition(unsigned int input, unsigned int nextStateID)
{
	unsigned int nextStateIdx = stateId2IdxMap[nextStateID];
	for(int i=0; i<currentMaxCount; i++)
		setNextState(currentIdx+i,input,nextStateIdx);
}

/** Set counting state-sequence for given input. */
void FsmBuilder::setCountingTransition(unsigned int input, unsigned int endStateID)
{
	unsigned int endStateIdx = stateId2IdxMap[endStateID];
	for(int i=0; i<currentMaxCount-1; i++)
		setNextState(currentIdx+i,input,currentIdx+i+1);
	setNextState(currentIdx+currentMaxCount-1,input,endStateIdx);
}

void FsmBuilder::showCurrent()
{
	cout << "Current state IDX:" << currentIdx << endl;
	for(int sp=0; sp<currentMaxCount; sp++)
	{
		for(int i=0; i<currentInputNumber; i++)
		{
			cout << " ["<<i<<"]->"<<getTransition(currentIdx+sp,i);
		}
		cout << endl;
	}
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
			srcIdx = s*maxInputNumber+i;
			dstIdx = s*inputNumber+i;
			result[dstIdx] = transitions[srcIdx]*currentInputNumber;	// IDX-encoding
		}
	}
	return result;
}

unsigned int FsmBuilder::getIdxOfStateID(int stateID)
{
	return stateID*currentInputNumber;
}

void FsmBuilder::setupRgbInverseLut(unsigned char *inverseLut)
{
	map<int, int>::const_iterator iter;
	// go along every mapped state
	for (iter=stateId2IdxMap.begin(); iter != stateId2IdxMap.end(); ++iter)
	{
		// Now state ID is iter->first, IDX is iter->second
		int maxCount = stateId2MaxcountMap[iter->first];
		// For every state of this counter, set the color
		for(int i=0; i<maxCount; i++)
		{
			// Set the color for stateIDX iter->second+i
			// Warning: createFsmTransitionMatrix substitutes stateIDX with its base index for acceleration!
			int baseIDX = getIdxOfStateID(iter->second+i);

			RGB rgb = stateIdx2ColorMap[iter->second];
			inverseLut[baseIDX*3+0]=rgb.r;
			inverseLut[baseIDX*3+1]=rgb.g;
			inverseLut[baseIDX*3+2]=rgb.b;
		}
	}
}
