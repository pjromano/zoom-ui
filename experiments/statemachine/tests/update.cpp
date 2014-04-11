/*
	Philip Romano
	4/9/2014
	update.cpp

	Test for GestureStateGraph
	Testing the update() function to make sure it is logically correct.
*/

#include <iostream>
#include <string>
#include <math.h>

#include <Leap.h>
#include <SDL2/SDL.h>
#include <boost/shared_ptr.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"

class Node_OneHand : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("OneHand");
			return name;
		}

		/**
		  DecisionNode

		  Returns:
		  1 if one hand is present
		  0 otherwise
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() == 1) {
				return 1;
			} else {
				std::cout << "No hands" << std::endl;
				return 0;
			}
		}
};

class Node_Horizontal : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("Horizontal");
			return name;
		}

		/**
		  DecisionNode

		  Returns
		  1 if abs(yvel) <= 1/2 abs(xvel) [horizontal palm motion]
		  0 otherwise
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() > 0) {
				Leap::Hand h = (*hands.begin());
				Leap::Vector vel = h.palmVelocity();
				if (abs(vel.y) <= 0.5 * abs(vel.x)) {
					std::cout << "HORZ : " << vel.magnitude() << std::endl;
					return 1;
				} else
					return 0;
			} else
				return 1;
		}
};

class Node_Vertical : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("Vertical");
			return name;
		}

		/**
		  DecisionNode

		  Returns
		  1 if abs(xvel) <= 1/2 abs(yvel) [vertical palm motion]
		  0 otherwise
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() > 0) {
				Leap::Hand h = (*hands.begin());
				Leap::Vector vel = h.palmVelocity();
				if (abs(vel.x) < 0.5 * abs(vel.y)) {
					std::cout << "VERT" << std::endl;
					return 1;
				} else
					return 0;
			} else
				return 1;
		}
};

class Node_HorizontalSwipe : public GestureNode {
	public:
		virtual std::string& getName() {
			static std::string name("HorizontalSwipe");
			return name;
		}

		/**
		  StateNode

		  Returns
		  1 if abs(vel.x) > 50 [stay in state]
		  0 otherwise          [leave state]
		*/
		virtual int evaluate(const Leap::Frame& frame,
				const std::string& nodeid) {
			Leap::HandList hands = frame.hands();
			if (hands.count() > 0) {
				Leap::Hand h = (*hands.begin());
				Leap::Vector vel = h.palmVelocity();
				if (abs(vel.x) < 0.5 * abs(vel.y)) {
					std::cout << "VERT" << std::endl;
					return 1;
				} else
					return 0;
			} else
				return 1;
		}
};

class Engine : public Leap::Listener {
	public:
		Engine() {
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Horizontal()));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Vertical()));

			mGraph.addNode("Horizontal", "horizontal1");
			mGraph.addNode("Vertical", "vertical1");
			mGraph.addConnection("horizontal1", 1, "horizontal1");
			mGraph.addConnection("horizontal1", 0, "vertical1");
			mGraph.addConnection("vertical1", 1, "vertical1");
			mGraph.addConnection("vertical1", 0, "horizontal1");
		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			std::cout << "Frame" << std::endl;
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

