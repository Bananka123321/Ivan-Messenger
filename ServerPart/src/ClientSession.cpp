#include "../include/ClientSession.h"
#include "../include/Logger.h"

#include <cerrno>
#include <cstring>
#include <openssl/err.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int SERVE_POLL_INTERVAL_MS = 1000;
constexpr size_t READ_CHUNK_SIZE = 16 * 1024;

}

ClientSession::ClientSession(const int sock, SSL* ssl)
    : socket_(sock), user_id_(0), ssl_(ssl), isAuthenticated_(false),
      last_activity_time_(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()),
      wakeFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {
    if (ssl_) {
        SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);
    }
    if (wakeFd_ < 0) {
        LOG_CRITICAL(SESSION, "Не удалось создать eventfd для сессии. socket_fd=", socket_, " ошибка=", strerror(errno));
    }
    LOG_INFO(SESSION, "Создана новая сессия. socket_fd=", socket_);
}

ClientSession::~ClientSession() {
    LOG_INFO(SESSION, "Уничтожение сессии. userId=", user_id_, " username=", username_);

    if (ssl_) {
        if (!broken_ && !closed_.load()) {
            SSL_shutdown(ssl_);
        }
        SSL_free(ssl_);
        ssl_ = nullptr;
    }

    if (socket_ >= 0) {
        ::close(socket_);
    }

    if (wakeFd_ >= 0) {
        ::close(wakeFd_);
    }
}

int ClientSession::getSocket() const {
    return socket_;
}

const std::string& ClientSession::getUsername() const {
    return username_;
}

int ClientSession::getUserId() const {
    return user_id_;
}

void ClientSession::setUser(const int new_id, const std::string& new_username) {
    LOG_INFO(AUTH, "Авторизация сессии. socket_fd=", socket_, " userId=", new_id, " username=", new_username);

    username_ = new_username;
    user_id_ = new_id;
}

bool ClientSession::getIsAuthenticated() const {
    return isAuthenticated_;
}

void ClientSession::setIsAuthenticated(const bool value) {
    if (isAuthenticated_ != value) {
        LOG_INFO(AUTH, "Изменение статуса авторизации. userId=", user_id_, " новое значение=", (value ? "true" : "false"));
        isAuthenticated_ = value;
    }
}

int64_t ClientSession::getLastActivity() const {
    return last_activity_time_.load(std::memory_order_relaxed);
}

void ClientSession::setLastActivity(const int64_t newTimestamp) {
    last_activity_time_.store(newTimestamp);
}

bool ClientSession::getConnected() const {
    return connected_.load(std::memory_order_relaxed);
}

void ClientSession::setConnected(const bool newState) {
    if (const bool oldState = connected_.load(std::memory_order_relaxed); oldState != newState) {
        LOG_INFO(SESSION, "Изменение состояния подключения. userId=", user_id_, " новое состояние=", (newState ? "connected" : "disconnected"));
        connected_.store(newState);
    }
}

std::string ClientSession::getTempToken() const {
    return tempToken_;
}

void ClientSession::setTempToken(const std::string& newToken) {
    LOG_DEBUG(AUTH, "Установка временного токена. userId=", user_id_);
    tempToken_ = newToken;
}

bool ClientSession::send(const std::string& message) {
    if (closed_.load()) {
        LOG_WARNING(NETWORK, "Попытка отправки в закрытую сессию. userId=", user_id_, " размер=", message.size());
        return false;
    }

    std::string framed = PacketIO_Server::frame(message);
    bool overflow = false;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (queuedBytes_ + framed.size() > MAX_OUTGOING_QUEUE_BYTES) {
            overflow = true;
        } else {
            queuedBytes_ += framed.size();
            outgoing_.push_back(std::move(framed));
        }
    }

    if (overflow) {
        LOG_WARNING(NETWORK, "Очередь отправки переполнена, клиент не успевает читать. userId=", user_id_);
        close();
        return false;
    }

    wake();
    LOG_DEBUG(NETWORK, "Пакет поставлен в очередь отправки. userId=", user_id_, " размер=", message.size());
    return true;
}

bool ClientSession::flush() {
    if (broken_) {
        return false;
    }

    while (true) {
        if (writeOffset_ == writeBuffer_.size()) {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (outgoing_.empty()) {
                return true;
            }
            writeBuffer_ = std::move(outgoing_.front());
            outgoing_.pop_front();
            queuedBytes_ -= writeBuffer_.size();
            writeOffset_ = 0;
        }

        const int sent = SSL_write(ssl_, writeBuffer_.data() + writeOffset_, static_cast<int>(writeBuffer_.size() - writeOffset_));
        if (sent > 0) {
            writeOffset_ += sent;
            continue;
        }

        const int err = SSL_get_error(ssl_, sent);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            return true;
        }

        markBroken("SSL_write", err);
        return false;
    }
}

bool ClientSession::receive(std::string& message) {
    while (true) {
        const auto extracted = PacketIO_Server::extractPacket(readBuffer_, message);
        if (extracted == PacketIO_Server::Extract::Packet) {
            LOG_DEBUG(NETWORK, "Получено данных от клиента. userId=", user_id_, " размер=", message.size());
            return true;
        }
        if (extracted == PacketIO_Server::Extract::Invalid) {
            LOG_WARNING(NETWORK, "Некорректный размер пакета от клиента. userId=", user_id_);
            return false;
        }

        const auto result = readSome();
        if (result == ReadResult::Data) {
            continue;
        }
        if (result == ReadResult::WouldBlock && socket_ >= 0 && waitSocket(readWantsWrite_ ? POLLOUT : POLLIN, -1)) {
            continue;
        }

        LOG_WARNING(NETWORK, "Ошибка получения данных от клиента. userId=", user_id_);
        return false;
    }
}

bool ClientSession::handshake(const int timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (true) {
        const int result = SSL_accept(ssl_);
        if (result == 1) {
            return true;
        }

        const int err = SSL_get_error(ssl_, result);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            markBroken("SSL_accept", err);
            return false;
        }

        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0 || !waitSocket(err == SSL_ERROR_WANT_READ ? POLLIN : POLLOUT, static_cast<int>(left))) {
            LOG_WARNING(NETWORK, "Таймаут TLS-рукопожатия. socket_fd=", socket_);
            broken_ = true;
            return false;
        }
    }
}

void ClientSession::serve(const std::function<void(const std::string&)>& onPacket, const std::atomic<bool>& running) {
    while (running.load() && !closed_.load()) {
        const short socketEvents = POLLIN | (hasPendingOutput() || readWantsWrite_ ? POLLOUT : 0);
        pollfd fds[2] = {
            {socket_, socketEvents, 0},
            {wakeFd_, POLLIN, 0},
        };

        if (poll(fds, 2, SERVE_POLL_INTERVAL_MS) < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_WARNING(NETWORK, "Ошибка poll. userId=", user_id_, " ошибка=", strerror(errno));
            return;
        }

        if (fds[1].revents & POLLIN) {
            drainWake();
        }

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR) || (readWantsWrite_ && fds[0].revents & POLLOUT)) {
            ReadResult result;
            while ((result = readSome()) == ReadResult::Data) {
                if (!dispatchPackets(onPacket)) {
                    return;
                }
            }
            if (result == ReadResult::Failed) {
                return;
            }
        }

        if (!flush()) {
            return;
        }
    }
}

bool ClientSession::close() {
    if (closed_.exchange(true)) {
        return false;
    }

    LOG_INFO(SESSION, "Закрытие сессии. userId=", user_id_, " socket_fd=", socket_);

    if (socket_ >= 0) {
        shutdown(socket_, SHUT_RDWR);
    }
    wake();
    return true;
}

bool ClientSession::isClosed() const {
    return closed_.load();
}

bool ClientSession::hasPendingOutput() {
    if (writeOffset_ < writeBuffer_.size()) {
        return true;
    }
    std::lock_guard<std::mutex> lock(queueMutex_);
    return !outgoing_.empty();
}

ClientSession::ReadResult ClientSession::readSome() {
    char chunk[READ_CHUNK_SIZE];
    const int bytes = SSL_read(ssl_, chunk, sizeof(chunk));
    if (bytes > 0) {
        readBuffer_.append(chunk, bytes);
        readWantsWrite_ = false;
        return ReadResult::Data;
    }

    const int err = SSL_get_error(ssl_, bytes);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        readWantsWrite_ = err == SSL_ERROR_WANT_WRITE;
        return ReadResult::WouldBlock;
    }

    if (err == SSL_ERROR_ZERO_RETURN) {
        LOG_INFO(NETWORK, "Клиент закрыл соединение. userId=", user_id_);
        return ReadResult::Failed;
    }

    markBroken("SSL_read", err);
    return ReadResult::Failed;
}

bool ClientSession::dispatchPackets(const std::function<void(const std::string&)>& onPacket) {
    std::string packet;
    while (true) {
        const auto extracted = PacketIO_Server::extractPacket(readBuffer_, packet);
        if (extracted == PacketIO_Server::Extract::Incomplete) {
            return true;
        }
        if (extracted == PacketIO_Server::Extract::Invalid) {
            LOG_WARNING(NETWORK, "Некорректный размер пакета от клиента. userId=", user_id_);
            return false;
        }

        LOG_DEBUG(NETWORK, "Получено данных от клиента. userId=", user_id_, " размер=", packet.size());
        onPacket(packet);
    }
}

bool ClientSession::waitSocket(const short events, const int timeoutMs) const {
    pollfd fd{socket_, events, 0};
    while (true) {
        const int ready = poll(&fd, 1, timeoutMs);
        if (ready > 0) {
            return true;
        }
        if (ready == 0 || errno != EINTR) {
            return false;
        }
    }
}

void ClientSession::wake() const {
    if (wakeFd_ < 0) {
        return;
    }
    constexpr uint64_t one = 1;
    [[maybe_unused]] const auto written = write(wakeFd_, &one, sizeof(one));
}

void ClientSession::drainWake() const {
    uint64_t value;
    [[maybe_unused]] const auto bytes = read(wakeFd_, &value, sizeof(value));
}

void ClientSession::markBroken(const char* operation, const int result) {
    broken_ = true;
    LOG_WARNING(NETWORK, "Ошибка ", operation, ". userId=", user_id_, " socket_fd=", socket_, " код=", result, " errno=", strerror(errno));
    ERR_print_errors_fp(stderr);
}
