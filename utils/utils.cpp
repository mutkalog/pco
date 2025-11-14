#include "utils.h"
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>


std::vector<uint8_t> SSLUtils::decodeBase64(const std::string& input)
{
    std::vector<uint8_t> output(input.size());

    BIO* b64Filter = BIO_new(BIO_f_base64());
    BIO* bio       = BIO_new_mem_buf(input.data(), input.size());

    bio = BIO_push(b64Filter, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    int len = BIO_read(bio, output.data(), input.size());

    len > 0
        ? output.resize(len)
        : output.clear();

    BIO_free_all(bio);

    return output;
}

bool SSLUtils::verifySignature(const std::string &manifest, const std::vector<uint8_t> &signature, const std::string &pubkeyFilePath)
{
    bool result = false;
    FILE* fp = fopen(pubkeyFilePath.c_str(), "r");
    if (fp == nullptr)
        return false;

    EVP_PKEY* pubkey = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);

    fclose(fp);

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr)
        return false;

    if (EVP_DigestVerifyInit(context, nullptr, EVP_sha256(), nullptr, pubkey) == 1 &&
        EVP_DigestVerifyUpdate(context, manifest.data(), manifest.size()) == 1)
    {
        if (EVP_DigestVerifyFinal(context, signature.data(), signature.size()) == 1)
            result = true;
    }

    EVP_MD_CTX_free(context);
    EVP_PKEY_free(pubkey);

    return result;
}
