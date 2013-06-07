#include <memory.h>	// for memset
#include <cstddef>	// for NULL
#include <assert.h>

#include "FsmBuilder.h"

FsmBuilder::FsmBuilder()
{
	transitions = NULL;
	stateNumber = 0;
	inputNumber = 0;
}

FsmBuilder::FsmBuilder(int aStateNumber, int aInputNumber)
{
	init(aStateNumber,aInputNumber);
}


FsmBuilder::~FsmBuilder()
{
	if (transitions)
	{
		delete transitions;
	}
	transitions = NULL;
	stateNumber = 0;
	inputNumber = 0;
}

void FsmBuilder::init(int aStateNumber, int aInputNumber)
{
	if (transitions)
	{
		delete transitions;
	}
	stateNumber = aStateNumber;
	inputNumber = aInputNumber;
	transitions = new unsigned int[aStateNumber*aInputNumber];
}

/** Set an element of the transition matrix */
void FsmBuilder::setNextState(unsigned int state, unsigned int input, unsigned int nextState)
{
	assert(state<stateNumber);
	assert(input<inputNumber);
	assert(nextState<stateNumber);

	int idx = state*stateNumber + input;
	transitions[idx]=nextState;
}

/** Defines trap state and resets all matrix entries to that.
*/
void FsmBuilder::trapStateAll(unsigned int trapState)
{
	assert(trapState<stateNumber);
	for(int i=0; i<stateNumber*inputNumber; i++)
		transitions[i] = trapState;
}

void FsmBuilder::setDefaultForState(unsigned int state, unsigned int defaultNextState)
{
	assert(state<stateNumber);
	assert(defaultNextState<stateNumber);
	for(int i=0; i<inputNumber; i++)
		setNextState(state, i, defaultNextState);
}

void FsmBuilder::setDefaultForInput(unsigned int input, unsigned int defaultNextState)
{
	assert(input<inputNumber);
	assert(defaultNextState<stateNumber);
	for(int i=0; i<stateNumber; i++)
		setNextState(i, input, defaultNextState);
}

unsigned int *FsmBuilder::createFsmTransitionMatrix()
{
	unsigned int *result = new unsigned int[stateNumber*inputNumber];
	memcpy(result, transitions, stateNumber*inputNumber * sizeof(unsigned int));
	return result;
}
