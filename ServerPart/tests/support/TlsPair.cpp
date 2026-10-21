#include "TlsPair.h"

#include <openssl/err.h>
#include <stdexcept>

#include "PacketIO_Server.h"

namespace {

constexpr size_t kBioBufferSize = 4 * 1024 * 1024;

[[noreturn]] void fail(const std::string& what) {
    ERR_print_errors_fp(stderr);
    throw std::runtime_error("TlsPair: " + what);
}

}

TlsPair::TlsPair() {
    serverCtx_ = SSL_CTX_new(TLS_server_method());
    clientCtx_ = SSL_CTX_new(TLS_client_method());
    if (!serverCtx_ || !clientCtx_)
        fail("SSL_CTX_new");

    if (SSL_CTX_use_certificate_file(serverCtx_, TEST_CERT_FILE, SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(serverCtx_, TEST_KEY_FILE, SSL_FILETYPE_PEM) != 1)
        fail("не удалось загрузить тестовый сертификат");

    SSL_CTX_set_verify(clientCtx_, SSL_VERIFY_NONE, nullptr);

    server_ = SSL_new(serverCtx_);
    client_ = SSL_new(clientCtx_);

    BIO* serverBio = nullptr;
    if (BIO_new_bio_pair(&serverBio, kBioBufferSize, &clientBio_, kBioBufferSize) != 1)
        fail("BIO_new_bio_pair");

    SSL_set_bio(server_, serverBio, serverBio);
    SSL_set_bio(client_, clientBio_, clientBio_);
    SSL_set_accept_state(server_);
    SSL_set_connect_state(client_);

    bool serverDone = false;
    bool clientDone = false;
    for (int i = 0; i < 100 && !(serverDone && clientDone); ++i) {
        if (!clientDone) {
            const int r = SSL_do_handshake(client_);
            clientDone = r == 1;
            if (r != 1 && SSL_get_error(client_, r) != SSL_ERROR_WANT_READ)
                fail("рукопожатие клиента");
        }
        if (!serverDone) {
            const int r = SSL_do_handshake(server_);
            serverDone = r == 1;
            if (r != 1 && SSL_get_error(server_, r) != SSL_ERROR_WANT_READ)
                fail("рукопожатие сервера");
        }
    }
    if (!serverDone || !clientDone)
        fail("рукопожатие не завершилось");
}

TlsPair::~TlsPair() {
    if (ownsServer_ && server_)
        SSL_free(server_);
    if (client_)
        SSL_free(client_);
    SSL_CTX_free(serverCtx_);
    SSL_CTX_free(clientCtx_);
}

SSL* TlsPair::takeServer() {
    ownsServer_ = false;
    return server_;
}

bool TlsPair::clientHasData() const {
    return SSL_pending(client_) > 0 || BIO_ctrl_pending(clientBio_) > 0;
}

std::vector<std::string> TlsPair::clientReadPackets() {
    std::vector<std::string> packets;
    while (clientHasData()) {
        std::string data;
        if (!PacketIO_Server::recvPacket(client_, data))
            break;
        packets.push_back(std::move(data));
    }
    return packets;
}

bool TlsPair::clientSendPacket(const std::string& data) {
    return PacketIO_Server::sendPacket(client_, data);
}

void TlsPair::clientWriteRaw(const std::string& bytes) {
    if (SSL_write(client_, bytes.data(), static_cast<int>(bytes.size())) != static_cast<int>(bytes.size()))
        fail("SSL_write");
}

std::string TlsPair::clientReadRaw(const size_t max) {
    std::string buffer(max, '\0');
    const int n = SSL_read(client_, buffer.data(), static_cast<int>(max));
    buffer.resize(n > 0 ? static_cast<size_t>(n) : 0);
    return buffer;
}

void TlsPair::dropClient() {
    SSL_free(client_);
    client_ = nullptr;
    clientBio_ = nullptr;
}
