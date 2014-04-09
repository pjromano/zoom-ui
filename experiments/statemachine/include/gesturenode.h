/*
	Philip Romano
	4/7/2014
	gesturenode.h
*/

#ifndef GESTURENODE_H
#define GESTURENODE_H

#include <string>
#include <Leap.h>

/**
	GestureNode is an abstract base class that defines the behavior of nodes
	in a GestureStateGraph.
*/
class GestureNode {
	public:
		/**
			Get the name used to identify the specific type of GestureNode that
			this object is. Ideally, the name should describe the behavior
			that this GestureNode imparts on a node as part of the
			GestureStateGraph.
		*/
		virtual std::string& getName() = 0;

		/**
		  Processes the provided Leap Motion frame data and returns an integer
		  that corresponds to the branch index that should be followed after
		  this node.
		*/
		virtual int evaluate(const Leap::Frame &frame) = 0;
};

#endif

