#ifndef ISYSTEMCALLS_H
#define ISYSTEMCALLS_H

#include <sys/types.h>
#include <spawn.h>

class ISystemCalls
{
public:
    virtual ~ISystemCalls() = default;
    virtual int chmod(const char* path, mode_t mode) = 0;
    virtual int posix_spawn(pid_t *pid, const char *path,
                            const posix_spawn_file_actions_t *file_actions,
                            const posix_spawnattr_t *attr, char *const argv[],
                            char *const env[]) = 0;
    virtual int waitpid(pid_t pid, int* status, int options) = 0;
    virtual int sleep(unsigned int secs) = 0;
};
#endif // ISYSTEMCALLS_H
