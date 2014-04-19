/*
	Philip Romano
	4/13/2014
	homescreen.cpp

	Test for GestureStateGraph
	Extension of visual.cpp to include other swiping directions, according to
	the design for the Home Screen.
*/

#include <iostream>
#include <string>
#include <math.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Leap.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"

#ifdef _WIN32
#undef main // Windows doesn't like the SDL_main thing
#endif

#define PI 3.1415926535

class EngineException : public std::exception {
	public:
		EngineException(const std::string &message) throw()
				: mMessage(message)
			{ }

		~EngineException() throw()
			{ }

		std::string getMessage() {
			return mMessage;
		}

		const char* what() {
			return mMessage.c_str();
		}

	private:
		std::string mMessage;
};

class Engine : public Leap::Listener {
	public:
		class Node_Motion : public GestureNode {
			private:
				Engine *e;
				double mThreshold;

			public:
				Node_Motion(Engine *creator, double threshold = 300.0) {
					e = creator;
					mThreshold = threshold;
				}

				virtual const std::string& getName() {
					static std::string name("Motion");
					return name;
				}

				/**
				  Determines if there is one hand, and its speed is greater
				  than a threshold value.

				  Slots:
					1 if one hand is present, and the magnitude of velocity is
					    >= threshold
					0 otherwise
				*/
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (e->mMainHand.isValid()
							&& e->mHandVelocity.magnitude() >= mThreshold)
						return 1;
					else {
						e->mListNudge = e->mHandVelocity.x;
						e->mStackNudge = -e->mHandVelocity.y;
						return 0;
					}
				}
		};

		class Node_CoarseDirection : public GestureNode {
			private:
				Engine *e;

			public:
				Node_CoarseDirection(Engine *creator) {
					e = creator;
				}

				virtual const std::string& getName() {
					static std::string name("CoarseDirection");
					return name;
				}

				/**
				  Determines the direction of motion; vertical, horizontal, or
				  depth-wise. The regions are split by the planes y = x,
				  y = -x, y = z, and y = -z.

				  Slots:
					0 if vertical motion   [ |vy| > |vx| && |vy| > |vz| ]
					1 if horizontal motion [ |vx| > |vy| && |vx| > |vz| ]
					2 if depthwise motion  [ otherwise ]
					                       // [ |vz| > |vx| && |vz| > |vy| ]

					3 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
					// TODO Try getting rid of this check... it shouldn't be
					// necessary with the right flow of states
					if (e->mMainHand.isValid()) {
						if (abs(e->mHandVelocity.y) > abs(e->mHandVelocity.x)
						 && abs(e->mHandVelocity.y) > abs(e->mHandVelocity.z))
							return 0;
						else if (
							abs(e->mHandVelocity.x) > abs(e->mHandVelocity.y)
						 && abs(e->mHandVelocity.x) > abs(e->mHandVelocity.z))
							return 1;
						else
							return 2;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 3;
					}
				}
		};

		class Node_LeftRight : public GestureNode {
			private:
				Engine *e;
				std::string mName;
				double mThreshold;

			public:
				/**
				  getName() returns "LeftRight" + integer truncation of
				  threshold
				*/
				Node_LeftRight(Engine *creator, double threshold = 0.0f) {
					e = creator;
					mThreshold = threshold;
					mName = "LeftRight";
					mName.append(
						boost::lexical_cast<std::string>((int)mThreshold));
				}

				virtual const std::string& getName() {
					return mName;
				}

				/**
				  Determines whether the motion is leftwards or rightwards.

				  Slots:
					0 if leftward motion  [ vx <= -threshold ]
					1 if rightward motion [ vx >=  threshold ]
					2 if within threshold [ -threshold < vx < threshold ]

					2 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (e->mMainHand.isValid()) {
						if (e->mHandVelocity.x <= -mThreshold) {
							if (nodeid.compare("swL") == 0)
								e->mListNudge = 2.0 * e->mHandVelocity.x;
							return 0;
						} else if (e->mHandVelocity.x >= mThreshold) {
							if (nodeid.compare("swR") == 0)
								e->mListNudge = 2.0 * e->mHandVelocity.x;
							return 1;
						} else {
							return 2;
						}
					} else {
						std::cout << "BAD!!" << std::endl;
						return 3;
					}
				}

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (nodeid.compare("swL") == 0) {
						e->mSelection++;
						if (e->mSelection >= e->mNumSelections) {
							e->mListPosition -= e->mNumSelections;
							e->mSelection = 0;
						}
					} else if (nodeid.compare("swR") == 0) {
						e->mSelection--;
						if (e->mSelection < 0) {
							e->mListPosition += e->mNumSelections;
							e->mSelection = e->mNumSelections - 1;
						}
					}
				}
		};

		class Node_UpDown : public GestureNode {
			private:
				Engine *e;
				std::string mName;
				double mThreshold;

			public:
				/**
				  getName() returns "UpDown" + integer truncation of
				  threshold
				*/
				Node_UpDown(Engine *creator, double threshold = 0.0) {
					e = creator;
					mThreshold = threshold;
					mName = "UpDown";
					mName.append(
						boost::lexical_cast<std::string>((int)mThreshold));
				}

				virtual const std::string& getName() {
					return mName;
				}

				/**
				  Determines whether the motion is upwards or downwards.

				  Slots:
					0 if upward motion    [ vy <= -threshold ]
					1 if downward motion  [ vy >=  threshold ]
					2 if within threshold [ -threshold < vy < threshold ]

					3 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (e->mMainHand.isValid()) {
						if (e->mHandVelocity.y <= -mThreshold)
							return 0;
						else if (e->mHandVelocity.y >= mThreshold)
							return 1;
						else
							return 2;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 3;
					}
				}

				virtual void onEnter(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (nodeid.compare("swU") == 0) {
						if (e->mCurrentStack < e->mNumStacks - 1)
							e->mCurrentStack++;
					} else if (nodeid.compare("swD") == 0) {
						if (e->mCurrentStack > 0)
							e->mCurrentStack--;
					}
				}
		};

		class Node_ForeBack : public GestureNode {
			private:
				Engine *e;
				std::string mName;
				double mThreshold;

			public:
				/**
				  getName() returns "ForeBack" + integer truncation of
				  threshold
				*/
				Node_ForeBack(Engine *creator, double threshold = 0.0f) {
					e = creator;
					mThreshold = threshold;
					mName = "ForeBack";
					mName.append(
						boost::lexical_cast<std::string>((int)mThreshold));
				}

				virtual const std::string& getName() {
					return mName;
				}

				/**
				  Determines whether the motion is forwards or backwards.

				  Slots:
					0 if foreward motion  [ vz < -threshold ]
					1 if backward motion  [ vz >  threshold ]
					2 if within threshold [ -threshold < vz < threshold ]

					3 if error! ...no hands :(
				*/
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
					if (e->mMainHand.isValid()) {
						if (e->mHandVelocity.z < -mThreshold) {
							e->mTargetZoom += 0.0002 * e->mHandVelocity.z;
							return 0;
						} else if (e->mHandVelocity.z > mThreshold) {
							e->mTargetZoom += 0.0002 * e->mHandVelocity.z;
							return 1;
						} else
							return 2;
					} else {
						std::cout << "BAD!!" << std::endl;
						return 3;
					}
				}

				virtual void onLeave(const Leap::Frame& frame,
						const std::string& nodeid) {
					e->mTargetZoom = 0.0;
				}
		};

		class Node_LeftRightLimbo : public GestureNode {
			private:
				Engine *e;
				uint64_t mTimeStart;
				uint64_t mTimelimit;
				double   mThreshold;

			public:
				/**
				  timer : number of microseconds before timing out (slot 0)
				  threshold : minimum threshold of hand speed to leave state
				      Should be positive (it is absolute-valued anyway)
				*/
				Node_LeftRightLimbo(Engine *creator,
						uint64_t timer = 100000,
						double threshold = 50.0) {
					e = creator;
					mTimeStart = 0;
					mThreshold = abs(threshold);
					mTimelimit = timer;
				}

				virtual const std::string& getName() {
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
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
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
				double   mThreshold;

			public:
				/**
				  timer : number of microseconds before timing out (slot 0)
				  threshold : minimum threshold of hand speed to leave state
				      Should be positive (it is absolute-valued anyway)
				*/
				Node_UpDownLimbo(Engine *creator,
						uint64_t timer = 100000,
						double threshold = 50.0) {
					e = creator;
					mTimeStart = 0;
					mThreshold = abs(threshold);
					mTimelimit = timer;
				}

				virtual const std::string& getName() {
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
				virtual int evaluate(const Leap::Frame& frame,
						const std::string& nodeid) {
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

			bool success = true;

			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_Motion(this, 50.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_CoarseDirection(this)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_LeftRight(this, 200.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_LeftRight(this, 0.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_UpDown(this, 200.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_UpDown(this, 0.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_ForeBack(this, 5.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_ForeBack(this, 10.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_LeftRightLimbo(this, 100000, 100.0)));
			success &= mGraph.createNodeType(boost::shared_ptr<GestureNode>(
						new Node_UpDownLimbo(this, 100000, 100.0)));

			if (!success)
				throw EngineException("Node type creation failed");

			/* Nodes */

			mGraph.addNode("Motion",          "noMotion");
			mGraph.addNode("CoarseDirection", "coarse");
			mGraph.addNode("LeftRight200",    "stHorizontal");
			mGraph.addNode("UpDown200",       "stVertical");
			mGraph.addNode("ForeBack5",       "stDepth");

			mGraph.addNode("LeftRight0", "swL");
			mGraph.addNode("LeftRight0", "swR");
			mGraph.addNode("UpDown0",    "swU");
			mGraph.addNode("UpDown0",    "swD");
			mGraph.addNode("ForeBack10",  "pull");
			mGraph.addNode("ForeBack10",  "push");

			mGraph.addNode("LRLimbo",     "swLToBacktrack");
			mGraph.addNode("LeftRight0",  "swLBacktrack");
			mGraph.addNode("LRLimbo",     "swLFromBacktrack");
			mGraph.addNode("LRLimbo",     "swRToBacktrack");
			mGraph.addNode("LeftRight0",  "swRBacktrack");
			mGraph.addNode("LRLimbo",     "swRFromBacktrack");

			mGraph.addNode("UDLimbo",     "swUToBacktrack");
			mGraph.addNode("UpDown0",     "swUBacktrack");
			mGraph.addNode("UDLimbo",     "swUFromBacktrack");
			mGraph.addNode("UDLimbo",     "swDToBacktrack");
			mGraph.addNode("UpDown0",     "swDBacktrack");
			mGraph.addNode("UDLimbo",     "swDFromBacktrack");

			mGraph.setStart("noMotion");

			/* Connections */

			mGraph.addConnection("noMotion", 1, "coarse");
			mGraph.addConnection("coarse",   0, "stVertical");
			mGraph.addConnection("coarse",   1, "stHorizontal");
			mGraph.addConnection("coarse",   2, "stDepth");

			mGraph.addConnection("stHorizontal", 0, "swL");
			mGraph.addConnection("stHorizontal", 1, "swR");
			mGraph.addConnection("stVertical",   0, "swD");
			mGraph.addConnection("stVertical",   1, "swU");
			mGraph.addConnection("stDepth",      0, "push");
			mGraph.addConnection("stDepth",      1, "pull");

			// Swipe LEFT sub-cycle
			mGraph.addConnection("swL",              0, "swL");
			mGraph.addConnection("swL",              1, "swLToBacktrack");
			mGraph.addConnection("swL",              2, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   2, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack",   3, "swLBacktrack");
			mGraph.addConnection("swLBacktrack",     0, "swLFromBacktrack");
			mGraph.addConnection("swLBacktrack",     1, "swLBacktrack");
			mGraph.addConnection("swLBacktrack",     2, "");
			mGraph.addConnection("swLFromBacktrack", 1, "swLFromBacktrack");
			mGraph.addConnection("swLFromBacktrack", 2, "swL");
			mGraph.addConnection("swLFromBacktrack", 3, "swLFromBacktrack");

			// Swipe RIGHT sub-cycle
			mGraph.addConnection("swR",              0, "swRToBacktrack");
			mGraph.addConnection("swR",              1, "swR");
			mGraph.addConnection("swR",              2, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack",   1, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack",   2, "swRBacktrack");
			mGraph.addConnection("swRToBacktrack",   3, "swRToBacktrack");
			mGraph.addConnection("swRBacktrack",     0, "swRBacktrack");
			mGraph.addConnection("swRBacktrack",     1, "swRFromBacktrack");
			mGraph.addConnection("swRBacktrack",     2, "");
			mGraph.addConnection("swRFromBacktrack", 1, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 2, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 3, "swR");

			// Swipe UP sub-cycle
			mGraph.addConnection("swU",              0, "swUToBacktrack");
			mGraph.addConnection("swU",              1, "swU");
			mGraph.addConnection("swU",              2, "swUToBacktrack");
			mGraph.addConnection("swUToBacktrack",   1, "swUToBacktrack");
			mGraph.addConnection("swUToBacktrack",   2, "swUBacktrack");
			mGraph.addConnection("swUToBacktrack",   3, "swUToBacktrack");
			mGraph.addConnection("swUBacktrack",     0, "swUBacktrack");
			mGraph.addConnection("swUBacktrack",     1, "swUFromBacktrack");
			mGraph.addConnection("swUBacktrack",     2, "");
			mGraph.addConnection("swUFromBacktrack", 1, "swUFromBacktrack");
			mGraph.addConnection("swUFromBacktrack", 2, "swUFromBacktrack");
			mGraph.addConnection("swUFromBacktrack", 3, "swU");

			// Swipe DOWN sub-cycle
			mGraph.addConnection("swD",              0, "swD");
			mGraph.addConnection("swD",              1, "swDToBacktrack");
			mGraph.addConnection("swD",              2, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   1, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   2, "swDToBacktrack");
			mGraph.addConnection("swDToBacktrack",   3, "swDBacktrack");
			mGraph.addConnection("swDBacktrack",     0, "swDFromBacktrack");
			mGraph.addConnection("swDBacktrack",     1, "swDBacktrack");
			mGraph.addConnection("swDBacktrack",     2, "");
			mGraph.addConnection("swDFromBacktrack", 1, "swDFromBacktrack");
			mGraph.addConnection("swDFromBacktrack", 2, "swD");
			mGraph.addConnection("swDFromBacktrack", 3, "swDFromBacktrack");

			// PUSH
			mGraph.addConnection("push", 0, "push");
			mGraph.addConnection("push", 2, "push");

			// PULL
			mGraph.addConnection("pull", 1, "pull");
			mGraph.addConnection("pull", 2, "pull");
		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
			c.setPolicyFlags(Leap::Controller::PolicyFlag::POLICY_BACKGROUND_FRAMES);
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame frame = c.frame(0);

			bool swiping = false;
			if (frame.hands().count() == 1) {
				// Average of fingers + palm
				mMainHand = (*frame.hands().begin());
				
				Leap::FingerList fingers = mMainHand.fingers();
				mCurrentVelocity = Leap::Vector(0.0f, 0.0f, 0.0f);
				if (fingers.count() > 0) {
					for (Leap::FingerList::const_iterator it
							= fingers.begin();
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

				if (mMainHand.fingers().count() >= 3) {
					swiping = true;
					// Stupid temporary fun
					// Maybe keep it?
					mTargetRotate = 30.0 / PI * atan2(mMainHand.palmNormal().y,
							mMainHand.palmNormal().z) + 20.0;
					if (mTargetRotate > 50.0)
						mTargetRotate = 50.0;
					else if (mTargetRotate < -10.0)
						mTargetRotate = -10.0;
				} else {
					if (mHandVelocity.magnitude() > 100.0) {
						SDL_RestoreWindow(mWindow);
						SDL_MaximizeWindow(mWindow);
						SDL_RaiseWindow(mWindow);
					}
				}
			}
			
			if (!swiping) {
				initializeHandVelocity();
				mMainHand = Leap::Hand::invalid();
				mHandVelocity = Leap::Vector::zero();
				mXYHandSpeed = 0.0f;
			}

			mGraph.updateWithPrint(frame);
		}

		void run() {
			mRunning = true;
			
			initializeHandVelocity();
			initializeGraphics();
			initializeApplication();

			runLoop();

			cleanUp();
		}

	private:
		GestureStateGraph mGraph;

		Leap::Hand   mMainHand;
		Leap::Vector mHandVelocity;
		double       mXYHandSpeed;
		int          mSelection,
		             mNumSelections,
					 mCurrentStack,
					 mNumStacks;
		double       mListPosition,
		             mListNudge,
					 mStackPosition,
		             mStackNudge,
		             mZoom,
		             mTargetZoom,
		             mTargetRotate,
					 mRotate;

		Leap::Vector mCurrentVelocity;
		int mNumSmoothing, mAverageVelocityCurrentIndex;
		std::vector<Leap::Vector> mAverageVelocityBuffer;
		Leap::Vector mAverageVelocity;

		SDL_Window    *mWindow;
		SDL_GLContext mGLContext;
		int           mScreenWidth,
		              mScreenHeight;

		bool mRunning;

		void initializeGraphics() {
			if (SDL_Init(SDL_INIT_VIDEO) != 0)
				throw EngineException("SDL_Init failed");

			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

			mWindow = SDL_CreateWindow("Gesture State Machine Experiment",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
			if (!mWindow)
				throw EngineException("SDL_CreateWindow failed");

			SDL_ShowCursor(SDL_FALSE);

			mGLContext = SDL_GL_CreateContext(mWindow);
			if (!mGLContext)
				throw EngineException("SDL_GL_CreateContext failed");

			SDL_GetWindowSize(mWindow, &mScreenWidth, &mScreenHeight);
			std::cout << mScreenWidth << ", " << mScreenHeight << std::endl;

			// Set up OpenGL

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_ALWAYS, 0.0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glClearDepth(1.0);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			//glOrtho(0, mScreenWidth, mScreenHeight, 0, -100, 100);
			gluPerspective(70.0, (double)mScreenWidth / (double)mScreenHeight,
					10.0, 10000.0);
			glViewport(0, 0, mScreenWidth, mScreenHeight);
			glPushMatrix();
		}

		void initializeHandVelocity() {
			mNumSmoothing = 3;
			mAverageVelocityCurrentIndex = 0;
			mAverageVelocityBuffer.clear();
			for (int i = 0; i < mNumSmoothing; ++i) {
				mAverageVelocityBuffer.push_back(
						Leap::Vector(0.0f, 0.0f, 0.0f));
			}
		}

		void initializeApplication() {
			mCurrentStack = 0;
			mNumStacks = 20;
			mStackNudge = 0.0;
			mStackPosition = 0.0;

			mSelection = 0;
			mNumSelections = 20;
			mListNudge = 0.0;
			mListPosition = 0.0;

			mZoom = 0.0;
			mTargetZoom = 0.0;

			mRotate = 0.0;
			mTargetRotate = 0.0;
		}

		void cleanUp() {
			SDL_GL_DeleteContext(mGLContext);
			SDL_DestroyWindow(mWindow);
			SDL_Quit();
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

		void runLoop() {
			Uint32 framestart;
			SDL_Event evt;

			while (mRunning) {

				framestart = SDL_GetTicks();

				while (SDL_PollEvent(&evt)) {
					switch (evt.type) {
						case SDL_QUIT:
							mRunning = false;
							break;
							
						case SDL_KEYDOWN:
							switch (evt.key.keysym.sym) {
								case SDLK_ESCAPE:
									mRunning = false;
									break;

								case SDLK_SPACE:
									mSelection = 0;
									break;
							}
							break;
					}
				}

				updatePosition();
				renderFrame();

				SDL_GL_SwapWindow(mWindow);

				while (SDL_GetTicks() - framestart < 1000 / 60)
					SDL_Delay(1000 / 60 - (SDL_GetTicks() - framestart));
			}
		}

		void updatePosition() {
			// Horizontal position
			double targetposition = (double)mSelection;
			mListPosition += (targetposition - mListPosition) / 20.0f
				- mListNudge * 0.00005f;

			// Stack (vertical position)
			targetposition = (double)mCurrentStack;
			mStackPosition += (targetposition - mStackPosition) / 10.0f;
				//- mStackNudge * 0.00005f;

			// Zooming
			mZoom += (mTargetZoom - mZoom) / 5.0f;
			if (mZoom > 1.0)
				mZoom = 1.0;
			else if (mZoom < -1.0)
				mZoom = -1.0;

			// Rotation
			mRotate += (mTargetRotate - mRotate) / 5.0f;
		}

		/**
		  All the GL rendering calls are done here.
		*/
		void renderFrame() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();

			glTranslated(0.0, -40.0, -300.0);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glPushMatrix(); // Origin

			double arc = 2 * PI / mNumSelections;
			double x, y, z, color;

			for (int stack = 0; stack < mNumStacks; ++stack) {

				if (mStackPosition >(double)stack - 0.999
						&& mStackPosition < (double)stack + 0.999) {

					// Draw the center (active) list
					double factor = 1.0 - abs(mStackPosition - stack);

					glPushMatrix(); // Center list rotation
					glRotated(factor * mRotate * (1.0 - mZoom), -1.0, 0.0, 0.0);

					for (int i = 0; i < mNumSelections; ++i) {

						x = 250.0 * sin(arc * i - (mListPosition * arc))
							* factor;
						y = -stack * 100.0 + mStackPosition * 100.0;
						z = 100.0 * cos(arc * i - (mListPosition * arc))
							* factor;

						color = (z + 100.0) / 200.0;

						if (i == mSelection && stack == mCurrentStack) {
							glColor4d(0.6, 0.8, 1.0, 1.0);
							y += 5.0;
							if (mZoom > 0.0) {
								x *= (1.0 - mZoom);
								y = y * (1.0 - mZoom) + 40.0 * mZoom;
								z += 180.0 * mZoom;
							}
						}
						else
							glColor4d(color * 0.2, color * 0.5, color, 1.0);

						glPushMatrix(); // Box
						glTranslated(x, y, z);
						if (i == mSelection && mZoom < 0.0)
							glScaled(1.0 + mZoom, 1.0 + mZoom, 0.0);

						glBegin(GL_QUADS);
							glVertex3d(30.0, -10.0 - 20.0 * factor, 0.0);
							glVertex3d(30.0, 10.0 + 20.0 * factor, 0.0);
							glVertex3d(-30.0, 10.0 + 20.0 * factor, 0.0);
							glVertex3d(-30.0, -10.0 - 20.0 * factor, 0.0);
						glEnd();

						glPopMatrix(); // Box

						/*
						// Translucency (or translucency effect) is awful
						glPushMatrix(); // Reflection
						glTranslated(x, -y, z - 1.0);
						if (i == mSelection && mZoom < 0.0)
							glScaled(1.0 + mZoom, 1.0 + mZoom, 0.0);

						glBegin(GL_QUADS);
							//glColor4d(color * 0.05, color * 0.1, color * 0.2, 1.0);
							glColor4d(color * 0.2, color * 0.5, color * 0.8, 0.4);
							glVertex3d(-30.0, 30.0, 0.0);
							glVertex3d(30.0, 30.0, 0.0);
							//glColor4d(0.0, 0.0, 0.0, 1.0);
							glColor4d(color * 0.2, color * 0.5, color * 0.8, 0.0);
							glVertex3d(30.0, -30.0, 0.0);
							glVertex3d(-30.0, -30.0, 0.0);
						glEnd();

						glPopMatrix(); // Reflection

						*/
					}

					glPopMatrix(); // Center list rotation

				} else {
					// Draw the non-active lists (just single boxes)
					x = 0.0;
					if (stack < mCurrentStack)
						y = 75.0 - stack * 25.0 + mStackPosition * 25.0;
					else
						y = -75.0 - stack * 25.0 + mStackPosition * 25.0;
					z = 0.0;

					glPushMatrix(); // Box
					glTranslated(x, y, z);

					glBegin(GL_QUADS);
						glColor4d(0.1, 0.25, 0.5, 1.0);
						glVertex3d(30.0, -10.0, 0.0);
						glVertex3d(30.0, 10.0, 0.0);
						glVertex3d(-30.0, 10.0, 0.0);
						glVertex3d(-30.0, -10.0, 0.0);
					glEnd();

					glPopMatrix(); // Box
				}
			}

			glPopMatrix(); // Origin

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
		}
};

int main(int argc, char **argv) {
	try {
		Engine e;
		Leap::Controller controller(e);
		e.run();
		return 0;
	} catch (EngineException& e) {
		std::cout << "Exception: " << e.getMessage() << std::endl;
		return 1;
	} catch (std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
		return 1;
	}
}

