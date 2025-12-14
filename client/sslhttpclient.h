#ifndef SSLHTTPCLIENT_H
#define SSLHTTPCLIENT_H

#include "interfaces/ihttpclient.h"
#include <httplib.h>

class SslHttpClient : public IHttpClient
{
public:
    SslHttpClient(const std::string &host, int port,
                  const std::string &clientCert, const std::string &clientKey, const std::string &caCert);

    virtual httplib::Result Post(const std::string& path, const std::string& body, const std::string &contentType) override;
    virtual httplib::Result Get(const std::string& path) override;

private:
    httplib::SSLClient client_;
};

inline SslHttpClient::SslHttpClient(const std::string &host,
                                    int port,
                                    const std::string &clientCert,
                                    const std::string &clientKey,
                                    const std::string &caCert)
    : client_(host, port, clientCert, clientKey)
{
    client_.set_ca_cert_path(caCert);
    client_.enable_server_certificate_verification(true);
}

inline httplib::Result SslHttpClient::Post(const std::string &path,
                                             const std::string &body,
                                             const std::string &contentType)
{
    return client_.Post(path, body, contentType);
}

inline httplib::Result SslHttpClient::Get(const std::string &path)
{
    return client_.Get(path);
}

#endif // SSLHTTPCLIENT_H
