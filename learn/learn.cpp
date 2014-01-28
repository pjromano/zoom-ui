/*
	Learning Leap Motion SDK
	1/24/2014
*/

#include <iostream>
#include <unistd.h>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <leap/Leap.h>

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
					800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
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
					glBegin(GL_QUADS);
						glColor4d(1.0, 1.0, 1.0, 1.0);
						glVertex3d(mX - 5, mY - 5, 0.0);
						glVertex3d(mX + 5, mY - 5, 0.0);
						glVertex3d(mX + 5, mY + 5, 0.0);
						glVertex3d(mX - 5, mY + 5, 0.0);
					glEnd();
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
				std::cout << "hand 0 palm angle: ("
						<< norm.x << ", " << norm.y << ", " << norm.z << ")"
						<< std::endl;
				Leap::Vector pos = current.hands()[0].palmPosition();
				mX = 400 + 6 * pos.x;
				mY = 800 - 4 * pos.y;
			} else {
				mX = 0;
				mY = 0;
			}

			if (current.hands().count() > 1)
				mRunning = false;
		}

	private:
		bool mRunning;

		SDL_Window    *mWindow;
		SDL_GLContext mGLContext;

		bool mConnected;
		int  mX, mY;
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

