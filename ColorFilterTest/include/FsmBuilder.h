#ifndef __FSMBUILDER_H
#define __FSMBUILDER_H

class FsmBuilder
{
	unsigned int *transitions;
	int stateNumber;
	int inputNumber;
public:
	FsmBuilder();

	FsmBuilder(int aStateNumber, int aInputNumber);

	~FsmBuilder();

	void init(int aStateNumber, int aInputNumber);

	/** Set an element of the transition matrix */
	void setNextState(unsigned int currentState, unsigned int currentInput, unsigned int nextState);

	/** Defines trap state and resets all matrix entries to that.
	*/
	void trapStateAll(unsigned int trapState);

	void setDefaultForState(unsigned int state, unsigned int defaultNextState);

	void setDefaultForInput(unsigned int input, unsigned int defaultNextState);

	unsigned int *createFsmTransitionMatrix();
};

#endif