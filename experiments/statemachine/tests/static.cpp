/*
	Philip Romano
	3/13/2014
	Gestures as a state machine
*/

#include <iostream>
#include <string>
#include <math.h>

#include <Leap.h>
#include <SDL2/SDL.h>
#include <boost/shared_ptr.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"

class Node_Horizontal : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("Horizontal");
			return name;
		}

		/**
		  Returns
		  1 if abs(yvel) <= 1/2 abs(xvel)
		  0 otherwise
		*/
		virtual int evaluate(const Leap::Frame &frame,
				const std::string& nodeid) {
			Leap::Hand h = frame.hand(0);
			if (h.isValid()) {
				Leap::Vector vel = h.palmVelocity();
				return (abs(vel.y) <= 0.5 * abs(vel.x))
						? 1 : 0;
			} else
				return 0;
		}
};

class Node_Vertical : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("Vertical");
			return name;
		}

		/**
		  Returns
		  1 if abs(xvel) <= 1/2 abs(yvel)
		  0 otherwise
		*/
		virtual int evaluate(const Leap::Frame &frame,
				const std::string& nodeid) {
			Leap::Hand h = frame.hand(0);
			if (h.isValid()) {
				Leap::Vector vel = h.palmVelocity();
				return (abs(vel.x) <= 0.5 * abs(vel.y))
						? 1 : 0;
			} else
				return 0;
		}
};

int main(int argc, char **argv) {
	GestureStateGraph mGraph;

	mGraph.createNodeType(boost::shared_ptr<GestureNode>(
				new Node_Horizontal()));
	mGraph.createNodeType(boost::shared_ptr<GestureNode>(
				new Node_Vertical()));

	std::cout << "(Before)" << std::endl;
	std::cout << "nodeExists: "
		<< mGraph.nodeExists("horizontal1")
		<< std::endl;

	mGraph.addNode("Horizontal", "horizontal1");
	mGraph.addNode("Vertical", "vertical1");
	mGraph.addConnection("horizontal1", 0, "horizontal1");
	mGraph.addConnection("horizontal1", 1, "vertical1");
	mGraph.addConnection("vertical1", 0, "vertical1");
	mGraph.addConnection("vertical1", 1, "horizontal1");

	std::cout << std::endl << "(After)" << std::endl;
	std::cout << "nodeExists: " << mGraph.nodeExists("horizontal1")
		<< std::endl;
	std::cout << "getTypeFromNode: "
		<< mGraph.getTypeFromNode("horizontal1")->getName()
		<< std::endl;
	std::cout << "getSlot(0): " << mGraph.getSlot("horizontal1", 0)
		<< std::endl;
	std::cout << "getSlot(1): " << mGraph.getSlot("horizontal1", 1)
		<< std::endl;

	mGraph.removeConnection("horizontal1", 1);
	std::cout << std::endl << "(Deleted)" << std::endl;
	std::cout << "getSlot(0): " << mGraph.getSlot("horizontal1", 0)
		<< std::endl;
	std::cout << "getSlot(1): " << mGraph.getSlot("horizontal1", 1)
		<< std::endl;

	return 0;
}

