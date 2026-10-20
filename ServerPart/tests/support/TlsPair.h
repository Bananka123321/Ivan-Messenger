#pragma once

#include <openssl/ssl.h>
#include <string>
#include <vector>

class TlsPair {
public:
    TlsPair();
    ~TlsPair();

    TlsPair(const TlsPair&) = delete;
    TlsPair& operator=(const TlsPair&) = delete;

    [[nodiscard]] SSL* server() const { return server_; }
    [[nodiscard]] SSL* client() const { return client_; }

    SSL* takeServer();

    [[nodiscard]] bool clientHasData() const;

    std::vector<std::string> clientReadPackets();

    bool clientSendPacket(const std::string& data);

    void clientWriteRaw(const std::string& bytes);

    std::string clientReadRaw(size_t max);

private:
    SSL_CTX* serverCtx_ = nullptr;
    SSL_CTX* clientCtx_ = nullptr;
    SSL* server_ = nullptr;
    SSL* client_ = nullptr;
    BIO* clientBio_ = nullptr;
    bool ownsServer_ = true;
};
