#ifndef IHTTPCLIENT_H
#define IHTTPCLIENT_H

#include <string>
#include <httplib.h>


class IHttpClient
{
public:
    virtual ~IHttpClient() = default;
    virtual httplib::Result Post(const std::string& path, const std::string& body, const std::string &contentType) = 0;
    virtual httplib::Result Get(const std::string& path) = 0;
};

#endif // IHTTPCLIENT_H
