#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"

class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox() = default;

    enum Responses : int { OK = 0, FAIL = -1 };
    enum Comands   : int { RUN = 0, ABORT = -1 };
    enum FDNames   : int { PARENT, CHILD, TOTAL };

    void run(const UpdateContext& context) {prepare(context); launch(context); }
protected:
    virtual void prepare(const UpdateContext& context) override;
    virtual void launch(const UpdateContext& context) override;
    virtual void cleanup(const UpdateContext& context) override { }

private:
    void copyDependencies(const UpdateContext& context);
    void createRootfs(const UpdateContext& context);
    pid_t containerPid_;
    int socketsFds_[TOTAL];

    void socketReport(int sockFd, int cmd, const std::string& logMessage);
    int  socketRead(int sockFd);

};

inline void LinuxSandbox::socketReport(int sockFd, int cmd, const std::string &logMessage)
{
    std::cout << logMessage << std::endl;
    if (write(socketsFds_[sockFd], &cmd, sizeof(int)) == -1)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "write() failed");
    exit(errno);
}

inline int LinuxSandbox::socketRead(int sockFd)
{
    std::array<char, 128> buf{};
    if (read(socketsFds_[sockFd], buf.data(), buf.size()) <= 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "read() failed");
    try
    {
        int cmd = std::stoi(std::string(buf.data()), nullptr, 10);
        return cmd;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        throw;
    }
}

#endif // LINUXSANDBOX_H
