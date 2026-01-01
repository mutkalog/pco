#ifndef SYSCALLS_MOCK_H
#define SYSCALLS_MOCK_H

#include <gmock/gmock.h>
#include "core/interfaces/isystemcalls.h"


class MockSystemCalls : public ISystemCalls
{
public:
    MOCK_METHOD(int, chmod, (const char* path, mode_t mode), (override));
    MOCK_METHOD(int, posix_spawn, (pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attr, char *const argv[],
                char *const env[]), (override));
    MOCK_METHOD(int, waitpid, (pid_t pid, int* status, int options), (override));
    MOCK_METHOD(int, sleep, (unsigned int secs), (override));
};

#endif // SYSCALLS_MOCK_H
