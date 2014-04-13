/*
	Learning Leap Motion SDK
	1/27/2014
*/

#include <iostream>
#include <stdexcept>
#include <math.h>

#if defined(_WIN32)
#include <windows.h> // Needed in gl.h
#endif

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <Leap.h>

#if defined(_WIN32)
#undef main // Windows doesn't like the SDL_main thing
#endif

#define PI 3.1415926535

struct Vector2D {
	double x, y;

	Vector2D(double nx, double ny) : x(nx), y(ny)
		{ }
};

class Everything : public Leap::Listener {
	public:
		Everything() {
			mRunning = false;
			mWindow = 0;
			mConnected = false;

			mX = 0;
			mY = 0;
		}

		void run() {
			mWindow = SDL_CreateWindow("Learning Leap",
					SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					800, 600, SDL_WINDOW_OPENGL);
			if (!mWindow)
				throw std::runtime_error("Failed to create SDL window");

			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

			mGLContext = SDL_GL_CreateContext(mWindow);
			if (!mGLContext)
				throw std::runtime_error("Failed to create OpenGL context");

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glOrtho(0, 800, 600, 0, -1, 1);

			SDL_Event evt;
			Uint32    framestart;

			mRunning = true;
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
								default:
									break;
							}
							break;
						default:
							break;
					}
				}

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				if (mConnected) {
					glPushMatrix();
					glTranslated(mX, mY, 0.0);
					glRotated(mPalmAngle, 0.0, 0.0, -1.0);

					glBegin(GL_QUADS);
						glColor4d(1.0, 1.0, 1.0, 1.0);
						glVertex3d(-5.0,  5.0, 0.0);
						glVertex3d( 5.0,  5.0, 0.0);
						glVertex3d( 5.0, -5.0, 0.0);
						glVertex3d(-5.0, -5.0, 0.0);
					glEnd();
					glBegin(GL_LINES);
						glVertex3d(0.0,  5.0, 0.0);
						glVertex3d(0.0, 10.0, 0.0);
						glVertex3d(-30.0, 0.0, 0.0);
						glVertex3d( 30.0, 0.0, 0.0);
					glEnd();

					glPopMatrix();
				}

				SDL_GL_SwapWindow(mWindow);

				while (SDL_GetTicks() - framestart < 1000 / 60)
					SDL_Delay(1000 / 60 - (SDL_GetTicks() - framestart));
			}

			SDL_DestroyWindow(mWindow);
			mWindow = 0;
		}

		/* Leap callbacks */

		virtual void onConnect(const Leap::Controller &c) {
			mConnected = true;
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame current = c.frame(0);

			if (current.hands().count() > 0) {
				Leap::Vector norm = current.hands()[0].palmNormal();
				std::cout << "hand 0 palm normal: ("
						<< norm.x << ", "
						<< norm.y << ", "
						<< norm.z << ")"
						<< std::endl;
				Leap::Vector pos = current.hands()[0].palmPosition();
				mX = 400 + (int)(6.0f * pos.x);
				mY = 800 - (int)(4.0f * pos.y);
				mPalmAngle = 180 * asin(norm.x) / PI;
			} else {
				mX = 0;
				mY = 0;
			}

			if (current.hands().count() > 1) {
				//mRunning = false;
			}
		}

	private:
		bool mRunning;

		SDL_Window    *mWindow;
		SDL_GLContext mGLContext;

		bool mConnected;
		int mX, mY;
		double mPalmAngle; // Angle about "screen normal" in degrees
};

int main(int argc, char **argv) {
	Everything cadacosa;
	Leap::Controller controller(cadacosa);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL initialization failed" << std::endl;
		return -1;
	}

	cadacosa.run();

	SDL_Quit();

	return 0;
}
