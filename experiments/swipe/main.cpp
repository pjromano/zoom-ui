/*
	Philip Romano
	3/2/2014
*/

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <gl/GL.h>
#include <gl/GLU.h>
#include <SDL2/SDL.h>
#include <Leap.h>

#ifdef _WIN32
#undef main // Windows doesn't like the SDL_main thing
#endif

#define PI 3.1415926535

class EngineException : public std::exception {
	public:
		EngineException(const std::string &message) : mMessage(message) { }

		std::string getMessage() {
			return mMessage;
		}

	private:
		std::string mMessage;
};

class Engine : public Leap::Listener {
	public:
		Engine() {
			mWindow = 0;
			mScreenWidth = 0;
			mScreenHeight = 0;
			mRunning = false;
		}

		/**
			Run the engine. Throws EngineException on failure.
		*/
		void run() {
			initialize();

			mRunning = true;
			runLoop();

			cleanUp();
		}

		/*
			Leap callbacks
		*/

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame current = c.frame(0);

			if (!current.hand(mMainHandIndex).isValid())
				mMainHandIndex = -1;
			
			if (current.hands().count() > 0) {
				// Grab hold of a hand, if not already
				if (mMainHandIndex == -1)
					mMainHandIndex = current.hands().rightmost().id();

				Leap::Hand hand = current.hand(mMainHandIndex);
				Leap::FingerList fingers = hand.fingers();
				mCurrentVelocity = Leap::Vector(0.0f, 0.0f, 0.0f);
				if (fingers.count() > 0) {
					for (Leap::FingerList::const_iterator it = fingers.begin();
						it != fingers.end(); ++it) {
						mCurrentVelocity += (*it).tipVelocity();
					}
					mCurrentVelocity /= (float)fingers.count();
				}
				mCurrentVelocity += hand.palmVelocity();
			} else {
				mCurrentVelocity = Leap::Vector(0.0f, 0.0f, 0.0f);
			}
		}

	private:
		bool mRunning;

		SDL_Window    *mWindow;
		SDL_GLContext mGLContext;

		int mScreenWidth,
			mScreenHeight;

		Leap::Controller mController;

		// Application mechanics
		int    mSelection,
			   mNumItems;
		double mListPosition, mListVelocity,
		       mItemSeparation;
		int32_t mMainHandIndex;
		
		Leap::Vector mCurrentVelocity;
		int mNumSmoothing, mAverageVelocityCurrentIndex;
		std::vector<Leap::Vector> mAverageVelocityBuffer;
		Leap::Vector mAverageVelocity;

		void initialize() {
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

			// Initialize values for application mechanics
			mSelection = 0;
			mNumItems = 9;
			mItemSeparation = 200.0;
			mListPosition = 0.0;
			mListVelocity = 0.0;

			mMainHandIndex = -1;

			mNumSmoothing = 2;
			mAverageVelocityCurrentIndex = 0;
			for (int i = 0; i < mNumSmoothing; ++i) {
				mAverageVelocityBuffer.push_back(
						Leap::Vector(0.0f, 0.0f, 0.0f));
			}

			mController.addListener(*this);
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
									mListPosition = 0.0;
									mListVelocity = 0.0;
									mCurrentVelocity =
									mAverageVelocity =
											Leap::Vector(0.0f, 0.0f, 0.0f);
									mAverageVelocityBuffer.assign(mNumSmoothing,
											Leap::Vector(0.0f, 0.0f, 0.0f));
									mSelection = 0;
									break;

								case SDLK_RIGHT:
									mListPosition += 50.0;
									break;
							}
							break;
					}
				}

				updateLogic();
				renderFrame();

				SDL_GL_SwapWindow(mWindow);

				while (SDL_GetTicks() - framestart < 1000 / 60)
					SDL_Delay(1000 / 60 - (SDL_GetTicks() - framestart));
			}
		}

		void updateLogic() {
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

			Leap::Vector velocity = mAverageVelocity;
			mListVelocity += velocity.x / 300.0;
			
			// Friction
			if (mListVelocity > 1.0)
				mListVelocity -= 1.0;
			else if (mListVelocity < -1.0)
				mListVelocity += 1.0;
			else
				mListVelocity = 0.0;

			// Gravitate towards selection
			//if (mSelection * mItemSeparation + mListPosition != 0.0)
			if (abs(mListVelocity) < 3.0)
				mListVelocity -= (mSelection * mItemSeparation + mListPosition) / 10.0;

			mListPosition += mListVelocity;

			// List moves rightwards; selection moves leftwards
			if (mListPosition > -mSelection * mItemSeparation + mItemSeparation / 2.0)
				mSelection -= 1;
			// Vice-versa
			else if (mListPosition < -mSelection * mItemSeparation - mItemSeparation / 2.0)
				mSelection += 1;
		}

		/**
			All the GL rendering calls are done here.
		*/
		void renderFrame() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			//glTranslated(0.0, 0.0, -500.0);
			
			glMatrixMode(GL_MODELVIEW);

			glPushMatrix(); // Origin
			glTranslated(mScreenWidth / 2, mScreenHeight / 2, 0.0);

			glBegin(GL_LINES);
				glColor4d(1.0, 1.0, 1.0, 0.3);
				glVertex3d(-mScreenWidth / 2 + 0.5, 0.5, 0.0);
				glVertex3d(mScreenWidth / 2 + 0.5, 0.5, 0.0);
				glVertex3d(0.5, -mScreenHeight / 2 + 0.5, 0.0);
				glVertex3d(0.5, mScreenHeight / 2 + 0.5, 0.0);
			glEnd();

			glPushMatrix(); // All Items
			glTranslated(mListPosition, 0.0, 0.0);

			glColor4d(0.5, 0.8, 1.0, 1.0);
			for (int i = -(mNumItems / 2); i < (mNumItems + 1) / 2; ++i) {
				glPushMatrix(); // Item
				glTranslated(i * mItemSeparation, 0.0, 0.0);
				if (i == mSelection) {
					glBegin(GL_QUADS);
						glVertex3d(-80.0, -100.0, 1.0);
						glVertex3d(80.0, -100.0, 1.0);
						glVertex3d(80.0, 100.0, 1.0);
						glVertex3d(-80.0, 100.0, 1.0);
					glEnd();
				} else {
					glBegin(GL_QUADS);
						glVertex3d(-60.0, -80.0, 1.0);
						glVertex3d(60.0, -80.0, 1.0);
						glVertex3d(60.0, 80.0, 1.0);
						glVertex3d(-60.0, 80.0, 1.0);
					glEnd();
				}
				glPopMatrix(); // Item
			}

			glPopMatrix(); // All Items
			glPopMatrix(); // Origin

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
		}

		void cleanUp() {
			SDL_GL_DeleteContext(mGLContext);
			SDL_DestroyWindow(mWindow);
			SDL_Quit();
		}
};

int main(int argc, char **argv) {
	try {
		Engine engine;
		engine.run();
		return 0;
	}
	catch (EngineException &e) {
		std::cout << "Exception: " << e.getMessage() << std::endl;
		return -1;
	}
}
