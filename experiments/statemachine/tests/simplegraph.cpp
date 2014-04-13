/*
	Philip Romano
	4/9/2014
	simplegraph.cpp

	Test for GestureStateGraph
	A test with a simple graph that has three states:
	   - not one hand or too little motion
	   - horizontal motion
	   - vertical motion
*/

#include <iostream>
#include <string>
#include <math.h>

#include <Leap.h>
#include <SDL2/SDL.h>
#include <boost/shared_ptr.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"

class Node_Motion : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("MotionState");
			return name;
		}

		/**
		  StateNode
		  Determines if there is one hand, and it has enough motion in the
		  x-y plane to leave this state. Otherwise, if hand motion is
		  negligible, then remain in this state.

		  Slots:
		    1 if one hand is present, and the magnitude of velocity is >=
			    200mm/s
		    0 otherwise
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() == 1) {
				Leap::Hand h = (*hands.begin());
				Leap::Vector vel = h.palmVelocity();
				vel.z = 0.0; // Only check motion in x-y plane
				if (vel.magnitude() >= 200.0) {
					return 1;
				} else {
					std::cout << "No motion" << std::endl;
					return 0;
				}
			} else {
				std::cout << "No motion" << std::endl;
				return 0;
			}
		}
};

class Node_Direction : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("Direction");
			return name;
		}

		/**
		  DecisionNode
		  Determines the direction of motion in the xy-plane

		  Slots:
		    1 if [horizontal palm motion]  |yvel| <= |xvel|
		    0 if [vertical palm motion]    otherwise

		    2 if error! ...No hands
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() > 0) {
				Leap::Hand h = (*hands.begin());
				Leap::Vector vel = h.palmVelocity();
				if (abs(vel.y) <= abs(vel.x)) {
					std::cout << "HORZ" << std::endl;
					return 1;
				} else {
					std::cout << "VERT" << std::endl;
					return 0;
				}
			} else
				return 2;
		}
};

class Engine : public Leap::Listener {
	public:
		Engine() {
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Motion()));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Direction()));

			mGraph.addNode("MotionState", "motion");
			mGraph.addNode("Direction", "direction");
			mGraph.addNode("Direction", "horizontal");
			mGraph.addNode("Direction", "vertical");
			mGraph.setStart("motion");

			mGraph.addConnection("motion", 1, "direction");
			mGraph.addConnection("direction", 0, "vertical");
			mGraph.addConnection("direction", 1, "horizontal");

			mGraph.addConnection("horizontal", 1, "horizontal");
			mGraph.addConnection("vertical", 0, "vertical");
		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			mGraph.update(c.frame(0));
		}

		void run() {
			while (true)
				SDL_Delay(100);
		}

	private:
		GestureStateGraph mGraph;
};

int main(int argc, char **argv) {
	Engine e;
	Leap::Controller controller(e);
	e.run();
	return 0;
}

