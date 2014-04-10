/*
	Philip Romano
	4/9/2014
	swipe.cpp

	Test for GestureStateGraph
	A test with a more complex graph that detects swiping within the xy-plane.
	The goal is to isolate the "forward" part of the swipe and ignore the
	"backward" part of the swipe motion.
*/

#include <iostream>
#include <string>
#include <math.h>

#include <Leap.h>
#include <SDL2/SDL.h>
#include <boost/shared_ptr.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"


class Engine : public Leap::Listener {
	private:
		GestureStateGraph mGraph;

		Leap::Hand   mMainHand;
		Leap::Vector mHandVelocity;
		float        mXYHandSpeed;

	public:
		class Node_Motion : public GestureNode {
			private:
				Engine *e;

			public:
				Node_Motion(Engine *creator) {
					e = creator;
				}

				virtual std::string& getName() {
					static std::string name("Motion");
					return name;
				}

				/**
				  Determines if there is one hand, and its speed is greater
				  than a threshold value.

				  Slots:
					1 if one hand is present, and the magnitude of velocity is
					    >= 200 mm/s
					0 otherwise
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					if (e->mMainHand.isValid() && e->mXYHandSpeed >= 200.0)
						return 1;
					else
						return 0;
				}
		};

		class Node_CoarseDirection : public GestureNode {
			private:
				Engine *e;

			public:
				Node_CoarseDirection(Engine *creator) {
					e = creator;
				}

				virtual std::string& getName() {
					static std::string name("CoarseDirection");
					return name;
				}

				/**
				  Determines the direction of motion in the xy-plane; either
				  horizontal or vertical, split by the lines y = x and y = -x

				  Slots:
					0 if vertical motion   [ |vx| < |vy| ]
					1 if horizontal motion [ otherwise ]

					2 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					// TODO Try getting rid of this check... it shouldn't be
					// necessary with the right flow of states
					if (e->mMainHand.isValid()) {
						if (abs(e->mHandVelocity.x) < abs(e->mHandVelocity.y))
							return 0;
						else
							return 1;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 2;
					}
				}
		};

		class Node_LeftRight : public GestureNode {
			private:
				Engine *e;

			public:
				Node_LeftRight(Engine *creator) {
					e = creator;
				}

				virtual std::string& getName() {
					static std::string name("LeftRight");
					return name;
				}

				/**
				  Determines whether the motion is leftwards or rightwards.

				  Slots:
					0 if leftward motion  [ vx <= 0 ]
					1 if rightward motion [ vx >  0 ]

					2 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					// TODO Try getting rid of this check... it shouldn't be
					// necessary with the right flow of states
					if (e->mMainHand.isValid()) {
						if (e->mHandVelocity.x <= 0) {
							return 0;
						} else
							return 1;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 2;
					}
				}

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
//					if (nodeid.compare("swL") == 0)
//						std::cout << "Swipe Left start" << std::endl;
//					if (nodeid.compare("swR") == 0)
//						std::cout << "Swipe Right start" << std::endl;
				}
		};

		class Node_UpDown : public GestureNode {
			private:
				Engine *e;

			public:
				Node_UpDown(Engine *creator) {
					e = creator;
				}

				virtual std::string& getName() {
					static std::string name("UpDown");
					return name;
				}

				/**
				  Determines whether the motion is upwards or downwards.

				  Slots:
					0 if upward motion   [ vy <= 0 ]
					1 if downward motion [ vy >  0 ]

					2 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					// TODO Try getting rid of this check... it shouldn't be
					// necessary with the right flow of states
					if (e->mMainHand.isValid()) {
						if (e->mHandVelocity.y <= 0)
							return 0;
						else
							return 1;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 2;
					}
				}
		};

		Engine() {
			mMainHand = Leap::Hand::invalid();

			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Motion(this)));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_CoarseDirection(this)));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_LeftRight(this)));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_UpDown(this)));

			mGraph.addNode("Motion", "noMotion");
			mGraph.addNode("CoarseDirection", "coarse");
			mGraph.addNode("LeftRight", "stHorizontal");
			mGraph.addNode("UpDown", "stVertical");

			mGraph.addNode("LeftRight", "swL");
			mGraph.addNode("LeftRight", "swR");
			mGraph.addNode("Motion", "swLToBacktrack");
			mGraph.addNode("LeftRight", "swLBacktrack");
			mGraph.addNode("Motion", "swLFromBacktrack");
			mGraph.addNode("Motion", "swRToBacktrack");
			mGraph.addNode("LeftRight", "swRBacktrack");
			mGraph.addNode("Motion", "swRFromBacktrack");

			mGraph.setStart("noMotion");

			mGraph.addConnection("noMotion", 1, "coarse");
			mGraph.addConnection("coarse", 0, "stVertical");
			mGraph.addConnection("coarse", 1, "stHorizontal");

			mGraph.addConnection("stHorizontal", 0, "swL");
			mGraph.addConnection("stHorizontal", 1, "swR");

			// Swipe left sub-cycle
			mGraph.addConnection("swL", 0, "swL");
			mGraph.addConnection("swL", 1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack", 1, "swLBacktrack");
			mGraph.addConnection("swLBacktrack", 0, "swLFromBacktrack");
			mGraph.addConnection("swLBacktrack", 1, "swLBacktrack");
			mGraph.addConnection("swLFromBacktrack", 1, "swL");

			// Swipe right sub-cycle
			mGraph.addConnection("swR", 1, "swR");
			mGraph.addConnection("swR", 0, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack", 1, "swRBacktrack");
			mGraph.addConnection("swRBacktrack", 1, "swRFromBacktrack");
			mGraph.addConnection("swRBacktrack", 0, "swRBacktrack");
			mGraph.addConnection("swRFromBacktrack", 1, "swR");
		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame frame = c.frame(0);
//			std::cout << "Frame " << (frame.timestamp() / 100000) << std::endl;
			if (frame.hands().count() == 1) {
				mMainHand = (*frame.hands().begin());
				mHandVelocity = mMainHand.palmVelocity();
				Leap::Vector xyvel = mHandVelocity;
				xyvel.z = 0.0f;
				mXYHandSpeed = xyvel.magnitude();
			} else {
				mMainHand = Leap::Hand::invalid();
				mHandVelocity = Leap::Vector::zero();
				mXYHandSpeed = 0.0f;
			}

			mGraph.updateWithPrint(frame);
		}

		void run() {
			while (true)
				SDL_Delay(100);
		}
};

int main(int argc, char **argv) {
	Engine e;
	Leap::Controller controller(e);
	e.run();
	return 0;
}

