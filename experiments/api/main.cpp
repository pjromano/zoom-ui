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


class Engine : public Leap::Listener {
	public:
		Engine() {

		}

		~Engine()
			{ }

		virtual void onConnect(const Leap::Controller &c) {
			c.enableGesture(Leap::Gesture::TYPE_SWIPE);
			c.enableGesture(Leap::Gesture::TYPE_CIRCLE);
			std::cout << c.config().setFloat("Gesture.Swipe.MinLength", 1.0f)
					<< std::endl
					<< c.config().setFloat("Gesture.Swipe.MinVelocity", 10.0f)
					<< std::endl
					<< "save: " << c.config().save()
					<< std::endl;
			std::cout << "Connected" << std::endl;
		}

		virtual void onFrame(const Leap::Controller &c) {
			Leap::Frame frame = c.frame(0);
			Leap::GestureList gestures = frame.gestures();
			for (Leap::GestureList::const_iterator it = gestures.begin();
					it != gestures.end(); ++it) {
				switch ((*it).state()) {
					case Leap::Gesture::STATE_START:
						switch ((*it).type()) {
							case Leap::Gesture::TYPE_SWIPE: {
								Leap::SwipeGesture g
									= (Leap::SwipeGesture)(*it);
								std::cout << "Swipe " << g.id() << std::endl;
							}	break;
							case Leap::Gesture::TYPE_CIRCLE: {
								Leap::CircleGesture g
									= (Leap::CircleGesture)(*it);
								std::cout << "Circle " << g.id() << std::endl;
							}	break;
						}
						break;
				}
			}
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

