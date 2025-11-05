#include "statemachine.h"
#include "updater.h"
#include <chrono>
#include <iostream>
#include <thread>


constexpr int g_checkIntervalMillis = 10e3;

int main()
{
    int pollInterval = g_checkIntervalMillis;
    Updater updater;

    for (;; std::this_thread::sleep_for(std::chrono::microseconds(pollInterval))) {
        auto state = StateMachine::instance().state();
        switch (state) {
        case StateMachine::IDLE:
            if (pollInterval == g_checkIntervalMillis) {
                StateMachine::instance().proceed();
                pollInterval = 0;
                std::cout << "gogogo\n";
            } else {
                pollInterval = g_checkIntervalMillis;
                std::cout << "sleep\n";
            }
            break;
        case StateMachine::CHECKING:
            updater.getManifest();
            exit(0);
            // StateMachine::instance().reset();

            break;
        case StateMachine::DOWNLOADING:
            break;
        case StateMachine::VERIFYING:
            break;
        case StateMachine::TESTING:
            break;
        case StateMachine::COMMITING:
            break;

        default: break;
        }
    }


}
