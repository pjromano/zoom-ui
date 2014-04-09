/*
	Philip Romano
	4/7/2014
	DecisionNode interface
*/

#ifndef DECISIONNODE_H
#define DECISIONNODE_H

#include <string>
#include "gesturenode.h"

class DecisionNode {
	public:
		/**
		  DecisionNode is not instantiable.
		*/
		virtual ~DecisionNode() = 0;

		/**
		  Processes the provided Leap Motion frame data and returns an integer
		  that corresponds to the branch index that should be followed after
		  this node.

		  DecisionNodes should return 0 to indicate the false branch, and 1
		  to indicate the true branch.

		  This implementation calls decide() and returns 1 if it returns
		  true or 0 if it returns false.
		*/
		virtual int evaluate(const Leap::Frame &frame);

		virtual bool decide(const Leap::Frame &frame) = 0;
};

#endif

