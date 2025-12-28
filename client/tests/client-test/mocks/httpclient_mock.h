#ifndef HTTPCLIENT_MOCK_H
#define HTTPCLIENT_MOCK_H

#include <gmock/gmock.h>
#include "core/interfaces/ihttpclient.h"


class MockHttpClient : public IHttpClient
{
public:
    MOCK_METHOD(httplib::Result, Post, (const std::string& path, const std::string& body, const std::string &contentType), (override));
    MOCK_METHOD(httplib::Result, Get, (const std::string& path), (override));
};
#endif // HTTPCLIENT_MOCK_H
