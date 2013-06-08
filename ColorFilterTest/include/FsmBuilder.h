#ifndef __FSMBUILDER_H
#define __FSMBUILDER_H

class FsmBuilder
{
	unsigned int *transitions;
	int maxStateNumber;
	int currentStateNumber;
	int maxInputNumber;
	int currentInputNumber;

public:
	FsmBuilder();

	FsmBuilder(int maxStateNumber, int maxInputNumber, int defaultNextState=0);

	~FsmBuilder();

	void init(int maxStateNumber, int maxInputNumber, int defaultNextState=0);

	/** Set an element of the transition matrix */
	void setNextState(unsigned int currentState, unsigned int currentInput, unsigned int nextState);

	/** Defines default next state (for example trap state) and resets all matrix entries to that.
	*/
	void all(unsigned int nextState);

	void setDefaultForState(unsigned int state, unsigned int defaultNextState);

	void setDefaultForInput(unsigned int input, unsigned int defaultNextState);

	unsigned int *createFsmTransitionMatrix(int &stateNumber, int &inputNumber);
};

#endif