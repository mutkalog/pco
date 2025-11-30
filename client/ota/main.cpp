#include "statemachine.h"
// #include <sys/mount.h>


int main()
{
    // int rc = umount2("/sys/fs/cgroup/pco", MNT_FORCE | MNT_DETACH);
    while (true)
    {
        StateMachine::instance().run();
    }
}
