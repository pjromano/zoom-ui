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
					if (e->mMainHand.isValid() && e->mXYHandSpeed >= 300.0)
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
					if (nodeid.compare("swL") == 0) {
						e->mSelection++;
						std::cout << e->mSelection << std::endl;
					} else if (nodeid.compare("swR") == 0) {
						e->mSelection--;
						std::cout << e->mSelection << std::endl;
					}
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
					0 if downward motion [ vy <= 0 ]
					1 if upward motion   [ vy >  0 ]

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

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (nodeid.compare("swU") == 0) {
						e->mLetter++;
						std::cout << e->mLetter << std::endl;
					} else if (nodeid.compare("swD") == 0) {
						e->mLetter--;
						std::cout << e->mLetter << std::endl;
					}
				}
		};

		class Node_LeftRightLimbo : public GestureNode {
			private:
				Engine *e;
				uint64_t mTimeStart;
				uint64_t mTimelimit;
				float    mThreshold;

			public:
				/**
				  timer : number of microseconds before timing out (slot 0)
				  threshold : minimum threshold of hand speed to leave state
				      Should be positive (it is absolute-valued anyway)
				*/
				Node_LeftRightLimbo(Engine *creator,
						uint64_t timer = 100000,
						float threshold = 50.0f) {
					e = creator;
					mTimeStart = 0;
					mThreshold = abs(threshold);
					mTimelimit = timer;
				}

				virtual std::string& getName() {
					static std::string name("LRLimbo");
					return name;
				}

				/**
				  Waits in this state until either a timer runs out, or the
				  speed in the x direction becomes greater than a threshold.

				  Slots:
				    3 if moving rightwards  [ vx >= threshold ]
					2 if moving leftwards   [ vx <= -theshold ]
					1 if negligible movemtn [ -threshold < vx < threshold ]
					0 if the timer runs out, or no hand is present
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					if (e->mMainHand.isValid()
							&& frame.timestamp() - mTimeStart < mTimelimit) {
						if (e->mHandVelocity.x >= mThreshold)
							return 3;
						else if (e->mHandVelocity.x <= -mThreshold)
							return 2;
						else
							return 1;
					} else
						return 0;
				}

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
					mTimeStart = frame.timestamp();
				}
		};

		class Node_UpDownLimbo : public GestureNode {
			private:
				Engine *e;
				uint64_t mTimeStart;
				uint64_t mTimelimit;
				float    mThreshold;

			public:
				/**
				  timer : number of microseconds before timing out (slot 0)
				  threshold : minimum threshold of hand speed to leave state
				      Should be positive (it is absolute-valued anyway)
				*/
				Node_UpDownLimbo(Engine *creator,
						uint64_t timer = 100000,
						float threshold = 50.0f) {
					e = creator;
					mTimeStart = 0;
					mThreshold = abs(threshold);
					mTimelimit = timer;
				}

				virtual std::string& getName() {
					static std::string name("UDLimbo");
					return name;
				}

				/**
				  Waits in this state until either a timer runs out, or the
				  speed in the y direction becomes greater than a threshold.

				  Slots:
				    3 if moving upwards     [ vy >= threshold ]
					2 if moving downwards   [ vy <= -theshold ]
					1 if negligible movemnt [ -threshold < vy < threshold ]
					0 if the timer runs out, or no hand is present
				*/
				virtual int evaluate(const Leap::Frame& frame) {
					if (e->mMainHand.isValid()
							&& frame.timestamp() - mTimeStart < mTimelimit) {
						if (e->mHandVelocity.y >= mThreshold)
							return 3;
						else if (e->mHandVelocity.y <= -mThreshold)
							return 2;
						else
							return 1;
					} else
						return 0;
				}

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
					mTimeStart = frame.timestamp();
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
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_LeftRightLimbo(this, 100000, 50.0f)));
			mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_UpDownLimbo(this, 100000, 50.0f)));

			/* Nodes */

			mGraph.addNode("Motion",          "noMotion");
			mGraph.addNode("CoarseDirection", "coarse");
			mGraph.addNode("LeftRight",       "stHorizontal");
			mGraph.addNode("UpDown",          "stVertical");

			mGraph.addNode("LeftRight", "swL");
			mGraph.addNode("LeftRight", "swR");
			mGraph.addNode("UpDown",    "swU");
			mGraph.addNode("UpDown",    "swD");

			mGraph.addNode("LRLimbo",   "swLToBacktrack");
			mGraph.addNode("LeftRight", "swLBacktrack");
			mGraph.addNode("LRLimbo",   "swLFromBacktrack");
			mGraph.addNode("LRLimbo",   "swRToBacktrack");
			mGraph.addNode("LeftRight", "swRBacktrack");
			mGraph.addNode("LRLimbo",   "swRFromBacktrack");

			mGraph.addNode("UDLimbo",   "swUToBacktrack");
			mGraph.addNode("UpDown",    "swUBacktrack");
			mGraph.addNode("UDLimbo",   "swUFromBacktrack");
			mGraph.addNode("UDLimbo",   "swDToBacktrack");
			mGraph.addNode("UpDown",    "swDBacktrack");
			mGraph.addNode("UDLimbo",   "swDFromBacktrack");

			mGraph.setStart("noMotion");

			/* Connections */

			mGraph.addConnection("noMotion", 1, "coarse");
			mGraph.addConnection("coarse", 0, "stVertical");
			mGraph.addConnection("coarse", 1, "stHorizontal");

			mGraph.addConnection("stHorizontal", 0, "swL");
			mGraph.addConnection("stHorizontal", 1, "swR");
			mGraph.addConnection("stVertical",   0, "swD");
			mGraph.addConnection("stVertical",   1, "swU");

			// Swipe LEFT sub-cycle
			mGraph.addConnection("swL",              0, "swL");
			mGraph.addConnection("swL",              1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   2, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   3, "swLBacktrack");
			mGraph.addConnection("swLBacktrack",     0, "swLFromBacktrack");
			mGraph.addConnection("swLBacktrack",     1, "swLBacktrack");
			mGraph.addConnection("swLFromBacktrack", 1, "swLFromBacktrack");
			mGraph.addConnection("swLFromBacktrack", 2, "swL");
			mGraph.addConnection("swLFromBacktrack", 3, "swLFromBacktrack");

			// Swipe RIGHT sub-cycle
			mGraph.addConnection("swR",              0, "swRToBacktrack");
			mGraph.addConnection("swR",              1, "swR");
			mGraph.addConnection("swRToBacktrack",   1, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack",   2, "swRBacktrack");
			mGraph.addConnection("swRToBacktrack",   3, "swRToBacktrack");
			mGraph.addConnection("swRBacktrack",     0, "swRBacktrack");
			mGraph.addConnection("swRBacktrack",     1, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 1, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 2, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 3, "swR");

			// Swipe UP sub-cycle
			mGraph.addConnection("swU",              0, "swUToBacktrack");
			mGraph.addConnection("swU",              1, "swU");
			mGraph.addConnection("swUToBacktrack",   1, "swUToBacktrack");
			mGraph.addConnection("swUToBacktrack",   2, "swUBacktrack");
			mGraph.addConnection("swUToBacktrack",   3, "swUToBacktrack");
			mGraph.addConnection("swUBacktrack",     0, "swUBacktrack");
			mGraph.addConnection("swUBacktrack",     1, "swUFromBacktrack");
			mGraph.addConnection("swUFromBacktrack", 1, "swUFromBacktrack");
			mGraph.addConnection("swUFromBacktrack", 2, "swUFromBacktrack");
			mGraph.addConnection("swUFromBacktrack", 3, "swU");

			// Swipe DOWN sub-cycle
			mGraph.addConnection("swD",              0, "swD");
			mGraph.addConnection("swD",              1, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   1, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   2, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   3, "swDBacktrack");
			mGraph.addConnection("swDBacktrack",     0, "swDFromBacktrack");
			mGraph.addConnection("swDBacktrack",     1, "swDBacktrack");
			mGraph.addConnection("swDFromBacktrack", 1, "swDFromBacktrack");
			mGraph.addConnection("swDFromBacktrack", 2, "swD");
			mGraph.addConnection("swDFromBacktrack", 3, "swDFromBacktrack");
		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame frame = c.frame(0);

			if (frame.hands().count() == 1) {
				// Average of fingers + palm
				mMainHand = (*frame.hands().begin());
				Leap::FingerList fingers = mMainHand.fingers();
				mCurrentVelocity = Leap::Vector(0.0f, 0.0f, 0.0f);
				if (fingers.count() > 0) {
					for (Leap::FingerList::const_iterator it = fingers.begin();
						it != fingers.end(); ++it) {
						mCurrentVelocity += (*it).tipVelocity();
					}
					mCurrentVelocity /= (float)fingers.count();
				}
				mCurrentVelocity += mMainHand.palmVelocity();
				if (fingers.count() > 0)
					mCurrentVelocity /= 2.0;
				updateHandVelocity();

				Leap::Vector xyvel = mHandVelocity;
				xyvel.z = 0.0f;
				mXYHandSpeed = xyvel.magnitude();

			} else {
				initializeHandVelocity();
				mMainHand = Leap::Hand::invalid();
				mHandVelocity = Leap::Vector::zero();
				mXYHandSpeed = 0.0f;
			}

			mGraph.update(frame);
		}

		void run() {
			initializeHandVelocity();
			mSelection = 0;
			mLetter = 'j';
			while (true)
				SDL_Delay(100);
		}

	private:
		GestureStateGraph mGraph;

		Leap::Hand   mMainHand;
		Leap::Vector mHandVelocity;
		float        mXYHandSpeed;

		int          mSelection;
		char         mLetter;

		Leap::Vector mCurrentVelocity;
		int mNumSmoothing, mAverageVelocityCurrentIndex;
		std::vector<Leap::Vector> mAverageVelocityBuffer;
		Leap::Vector mAverageVelocity;

		void initializeHandVelocity() {
			mNumSmoothing = 5;
			mAverageVelocityCurrentIndex = 0;
			mAverageVelocityBuffer.clear();
			for (int i = 0; i < mNumSmoothing; ++i) {
				mAverageVelocityBuffer.push_back(
						Leap::Vector(0.0f, 0.0f, 0.0f));
			}
		}

		void updateHandVelocity() {
			mAverageVelocityBuffer.at(mAverageVelocityCurrentIndex)
				= mCurrentVelocity;
			++mAverageVelocityCurrentIndex;
			if (mAverageVelocityCurrentIndex >= mNumSmoothing)
				mAverageVelocityCurrentIndex = 0;

			std::vector<Leap::Vector>::iterator it
				= mAverageVelocityBuffer.begin();
			mAverageVelocity = (*it);
			++it;
			while (it != mAverageVelocityBuffer.end()) {
				mAverageVelocity += (*it);
				++it;
			}
			mAverageVelocity /= (float)mNumSmoothing;
			mHandVelocity = mAverageVelocity;
		}
};

int main(int argc, char **argv) {
	try {
		Engine e;
		Leap::Controller controller(e);
		e.run();
		return 0;
	} catch (std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
		return 1;
	}
}

