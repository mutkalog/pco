#ifndef SYSTEMCALLS_H
#define SYSTEMCALLS_H

#include "interfaces/isystemcalls.h"

#include <sys/wait.h>
#include <sys/stat.h>

class SystemCalls : public ISystemCalls
{
public:
    int chmod(const char* path, mode_t mode) override
    {
        return ::chmod(path, mode);
    }

    int posix_spawn(pid_t *pid, const char *path,
                    const posix_spawn_file_actions_t *file_actions,
                    const posix_spawnattr_t *attr, char *const argv[],
                    char *const env[]) override
    {
        return ::posix_spawn(pid, path, file_actions, attr, argv, env);
    }

    int waitpid(pid_t pid, int* status, int options) override
    {
        return ::waitpid(pid, status, options);
    }

    int sleep(unsigned int secs) override
    {
        return ::sleep(secs);
    }
};

#endif // SYSTEMCALLS_H
