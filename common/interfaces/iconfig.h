#ifndef ICONFIG_H
#define ICONFIG_H

#include <filesystem>


namespace fs = std::filesystem;

class IConfig {
public:
    virtual ~IConfig() = default;

    virtual fs::path certPath()   const = 0;
    virtual fs::path keyPath()    const = 0;
    virtual fs::path caCertPath() const = 0;
};

#endif // ICONFIG_H
