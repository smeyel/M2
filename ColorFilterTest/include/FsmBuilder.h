#ifndef __FSMBUILDER_H
#define __FSMBUILDER_H
#include <iostream>
#include <map>

using namespace std;

class FsmBuilder
{
	unsigned int *transitions;
	int maxStateNumber;
	int currentStateNumber;
	int maxInputNumber;
	int currentInputNumber;

	// -------- For higher functions
	std::map<int,int> stateId2IdxMap;
	std::map<int,int> stateId2MaxcountMap;
	int nextFreeIdx;
	int currentIdx;
	int currentMaxCount;

	struct RGB
	{
		unsigned char r, g, b;
	};
	std::map<int,RGB> stateIdx2ColorMap;

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

	/** Copy state. Used for creating counting state sets */
	void copyState(unsigned int srcState, unsigned int dstState);

	/** Copies startState maxCount times after each other */
	void setCounterState(unsigned int startState, unsigned int maxCount);
	void setCounterInput(unsigned int startState, unsigned int maxCount, unsigned int input, unsigned int fallbackState);

	unsigned int getTransition(unsigned int state, unsigned int input)
	{
		int idx = state*maxStateNumber + input;
		return transitions[idx];
	}

	// -------- Higher functions

	/** Allocate new state for given ID and selects it as current. */
	void newState(unsigned int id, unsigned int maxCount=1);

	/** Select given ID as current state. */
	void selectState(unsigned int id);

	void setCurrentColor(unsigned char r, unsigned char g, unsigned char b);

	void setDefault();

	/** Set transition from current state. */
	void setTransition(unsigned int input, unsigned int nextStateID);

	/** Set counting state-sequence for given input. */
	void setCountingTransition(unsigned int input, unsigned int endStateID);

	void showCurrent();

	// --- Builder functions
	unsigned int *createFsmTransitionMatrix(int &stateNumber, int &inputNumber);

	/** When building the FSM, FsmBuilder replaces the transition matrix elements from
		state indices to state base indexes, that is, to its own location in the matrix.
		This avoids a multiplication during the FSM run.
	*/
	unsigned int getIdxOfStateID(int stateID);

	void setupRgbInverseLut(unsigned char *inverseLut);
};

#endif