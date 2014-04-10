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
#include <GL/gl.h>
#include <GL/glu.h>
#include <boost/shared_ptr.hpp>

#include "gesturestategraph.h"
#include "gesturenode.h"

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

	private:
		std::string mMessage;
};

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
						new Node_LeftRightLimbo(this)));

			mGraph.addNode("Motion",          "noMotion");
			mGraph.addNode("CoarseDirection", "coarse");
			mGraph.addNode("LeftRight",       "stHorizontal");
			mGraph.addNode("UpDown",          "stVertical");

			mGraph.addNode("LeftRight", "swL");
			mGraph.addNode("LeftRight", "swR");
			mGraph.addNode("LRLimbo",   "swLToBacktrack");
			mGraph.addNode("LeftRight", "swLBacktrack");
			mGraph.addNode("LRLimbo",   "swLFromBacktrack");
			mGraph.addNode("LRLimbo",   "swRToBacktrack");
			mGraph.addNode("LeftRight", "swRBacktrack");
			mGraph.addNode("LRLimbo",   "swRFromBacktrack");

			mGraph.setStart("noMotion");

			mGraph.addConnection("noMotion", 1, "coarse");
			mGraph.addConnection("coarse", 0, "stVertical");
			mGraph.addConnection("coarse", 1, "stHorizontal");

			mGraph.addConnection("stHorizontal", 0, "swL");
			mGraph.addConnection("stHorizontal", 1, "swR");

			// Swipe left sub-cycle
			mGraph.addConnection("swL", 0, "swL");
			mGraph.addConnection("swL", 1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack", 1, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack", 2, "swLToBacktrack");
			mGraph.addConnection("swLToBacktrack", 3, "swLBacktrack");
			mGraph.addConnection("swLBacktrack", 0, "swLFromBacktrack");
			mGraph.addConnection("swLBacktrack", 1, "swLBacktrack");
			mGraph.addConnection("swLFromBacktrack", 1, "swLFromBacktrack");
			mGraph.addConnection("swLFromBacktrack", 2, "swL");
			mGraph.addConnection("swLFromBacktrack", 3, "swLFromBacktrack");

			// Swipe right sub-cycle
			mGraph.addConnection("swR", 1, "swR");
			mGraph.addConnection("swR", 0, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack", 1, "swRToBacktrack");
			mGraph.addConnection("swRToBacktrack", 2, "swRBacktrack");
			mGraph.addConnection("swRToBacktrack", 3, "swRToBacktrack");
			mGraph.addConnection("swRBacktrack", 1, "swRFromBacktrack");
			mGraph.addConnection("swRBacktrack", 0, "swRBacktrack");
			mGraph.addConnection("swRFromBacktrack", 1, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 2, "swRFromBacktrack");
			mGraph.addConnection("swRFromBacktrack", 3, "swR");
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
			mRunning = true;

			initializeHandVelocity();
			mSelection = 0;
			initializeGraphics();
			runLoop();
			cleanUp();
		}

	private:
		GestureStateGraph mGraph;

		Leap::Hand   mMainHand;
		Leap::Vector mHandVelocity;
		float        mXYHandSpeed;
		int          mSelection;

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

			mWindow = SDL_CreateWindow("Swipe Experiment",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				800, 600, SDL_WINDOW_OPENGL);// | SDL_WINDOW_FULLSCREEN_DESKTOP);
			if (!mWindow)
				throw EngineException("SDL_CreateWindow failed");

			SDL_ShowCursor(SDL_FALSE);

			mGLContext = SDL_GL_CreateContext(mWindow);
			if (!mGLContext)
				throw EngineException("SDL_GL_CreateContext failed");

			SDL_GetWindowSize(mWindow, &mScreenWidth, &mScreenHeight);

			// Set up OpenGL

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glClearDepth(1.0);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

			glMatrixMode(GL_PROJECTION);
			glOrtho(0, mScreenWidth, mScreenHeight, 0, -100, 100);
			//glLoadIdentity();
			//gluPerspective(70.0, mScreenWidth / mScreenHeight, 10, 10000);
		}

		void initializeHandVelocity() {
			mNumSmoothing = 5;
			mAverageVelocityCurrentIndex = 0;
			mAverageVelocityBuffer.clear();
			for (int i = 0; i < mNumSmoothing; ++i) {
				mAverageVelocityBuffer.push_back(
						Leap::Vector(0.0f, 0.0f, 0.0f));
			}
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

				renderFrame();

				SDL_GL_SwapWindow(mWindow);

				while (SDL_GetTicks() - framestart < 1000 / 60)
					SDL_Delay(1000 / 60 - (SDL_GetTicks() - framestart));
			}
		}

		/**
		  All the GL rendering calls are done here.
		*/
		void renderFrame() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

