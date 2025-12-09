#include "statemachine.h"


int main()
{
    while (true)
    {
        StateMachine::instance().run();
    }
}
