/*
	Philip Romano
	3/13/2014
	Gestures as a state machine
*/

#include <iostream>
#include <Leap.h>
#include <SDL2/SDL.h>

/**
	Abstract base class
*/
class GestureNode {
	public:
		virtual ~GestureNode() = 0;

		/**
			Run code associated with this type of GestureNode. If the state
			is to change after execution, then the appropriate GestureNode
			state is returned. Else, null is returned indicating to stay
			in this state.
		*/
		virtual GestureNode *getNextState(const Leap::Frame &f) = 0;
};

class GestureState : public GestureNode {
	
};

class Engine : public Leap::Listener {
	public:
		Engine() {

		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			std::cout << "Frame" << std::endl;
		}

		void run() {
			while (true)
				SDL_Delay(100);
		}

	private:
		int x;
};

int main(int argc, char **argv) {
	Engine e;
	Leap::Controller controller(e);
	e.run();
	return 0;
}

