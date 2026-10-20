#include "../include/Handler.h"
#include "../include/Logger.h"

Handler::Handler(SessionManager& sm)
    : userManager_(Config::getDB().getConnectionStr()),
      messageManager_(Config::getDB().getConnectionStr()),
      dialogManager_(Config::getDB().getConnectionStr()),
      devicesSessionManager_(Config::getDB().getConnectionStr()),
      temporaryTokenManager_(Config::getDB().getConnectionStr()),
      sessionManager_(sm),
      dispatcher_(sessionManager_)
{
    LOG_INFO(HANDLER, "Инициализация Message Handler и регистрация обработчиков");

    handlers_["loginRequest"] = [this](const auto& client, const auto& j) {
        loginRequest(client, j);
    };

    handlers_["registerRequest"] = [this](const auto& client, const auto& j) {
        registerRequest(client, j);
    };

    handlers_["sendMessage"] = [this](const auto& client, const auto& j) {
        sendMessage(client, j);
    };

    handlers_["searchUserRequest"] = [this](const auto& client, const auto& j) {
        searchUserRequest(client, j);
    };

    handlers_["historyRequest"] = [this](const auto& client, const auto& j) {
        historyRequest(client, j);
    };

    handlers_["getDialogsRequest"] = [this](const auto& client, const auto& j) {
        getDialogsRequest(client, j);
    };

    handlers_["ping"] = [](const auto& client, const auto& j) {
        ping(client, j);
    };

    handlers_["resumeConnectionRequest"] = [this](const auto& client, const auto& j) {
        resumeConnectionRequest(client, j);
    };

    handlers_["resumeSessionRequest"] = [this](const auto& client, const auto& j) {
        resumeSessionRequest(client, j);
    };

    handlers_["openDialogRequest"] = [this](const auto& client, const auto& j) {
        openDialogRequest(client, j);
    };
}

void Handler::handleMessage(const std::shared_ptr<ClientSession>& client, std::string_view msg) {
    try {
        const auto j = nlohmann::json::parse(msg);
        SessionManager::updateActivity(client);

        if (const std::string type = j.value("type", "unknown"); handlers_.contains(type)) {
            LOG_DEBUG(HANDLER, "Обработка запроса типа '", type, "' от userId=", client->getUserId());
            handlers_[type](client, j);
        } else {
            LOG_WARNING(HANDLER, "Получен неизвестный тип сообщения: '", type, "' от userId=", client->getUserId());
        }
    } catch (const nlohmann::json::parse_error& e) {
        LOG_WARNING(HANDLER, "Ошибка парсинга JSON от userId=", client->getUserId(), " позиция=", e.byte, " ошибка=", e.what());
    } catch (const std::exception& e) {
        LOG_CRITICAL(HANDLER, "Необработанное исключение при обработке сообщения от userId=", client->getUserId(), " ошибка=", e.what());
    }
}

static std::mt19937& getGlobalRNG() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

std::string Handler::generateConnectToken() {
    static std::mutex rng_mutex;
    std::lock_guard<std::mutex> lock(rng_mutex);

    auto& gen = getGlobalRNG();
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    std::stringstream ss;
    for(int i = 0; i < 4; i++)
        ss << std::hex << std::setw(8) << std::setfill('0') << dis(gen);
    return ss.str();
}

//============================================================================================================================

void Handler::loginRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    std::string username = j.value("username", "");

    if(std::string error; !Validator::valid_string_field(j, "username", Validator::username, error) ||
                          !Validator::valid_string_field(j, "password", Validator::password, error)) {
        LOG_WARNING(HANDLER, "Ошибка валидации при входе. username='", username, "' ошибка: ", error);
        dispatcher_.sendTo(client, protocol::loginResponse(false, -1, "", "", "", error));
        return;
    }

    auto [success, user_id, error] = userManager_.loginUser(username, j["password"]);
    if (!success) {
        LOG_WARNING(HANDLER, "Неудачная попытка входа. username='", username, "' причина: ", error);
        dispatcher_.sendTo(client, protocol::loginResponse(false, user_id, username, "", "", error));
        return;
    }

    const auto sessionToken = devicesSessionManager_.createToken(user_id, "", "");
    const auto connectToken = authSuccess(client, user_id, username);

    if (!sessionToken.has_value()) {
        LOG_CRITICAL(HANDLER, "Критическая ошибка: не удалось создать токен сессии для userId=", user_id);
        dispatcher_.sendTo(client, protocol::loginResponse(false, user_id, username, "", "", "Internal server error"));
        return;
    }

    LOG_INFO(HANDLER, "Успешный вход пользователя. userId=", user_id, " username='", username, "'");
    dispatcher_.sendTo(client, protocol::loginResponse(true, user_id, username, connectToken, sessionToken.value(), ""));
}

void Handler::registerRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    std::string username = j.value("username", "");

    if(std::string error; !Validator::valid_string_field(j, "username", Validator::username, error) ||
                          !Validator::valid_string_field(j, "password", Validator::password, error)) {
        LOG_WARNING(HANDLER, "Ошибка валидации при регистрации. username='", username, "' ошибка: ", error);
        dispatcher_.sendTo(client, protocol::registerResponse(false, -1, "", "", "", error));
        return;
    }

    auto [success, user_id, error] = userManager_.registerUser(username, j["password"]);
    if (!success) {
        LOG_WARNING(HANDLER, "Неудачная попытка регистрации. username='", username, "' причина: ", error);
        dispatcher_.sendTo(client, protocol::registerResponse(false, user_id, username, "", "", error));
        return;
    }

    const auto sessionToken = devicesSessionManager_.createToken(user_id, "", "");
    const auto connectToken = authSuccess(client, user_id, username);

    if (!sessionToken.has_value()) {
        LOG_CRITICAL(HANDLER, "Критическая ошибка: не удалось создать токен сессии при регистрации userId=", user_id);
        dispatcher_.sendTo(client, protocol::registerResponse(false, user_id, username, "", "", "Internal server error"));
        return;
    }

    LOG_INFO(HANDLER, "Успешная регистрация пользователя. userId=", user_id, " username='", username, "'");
    dispatcher_.sendTo(client, protocol::registerResponse(true, user_id, username, connectToken, sessionToken.value(), ""));
}

std::string Handler::authSuccess(const std::shared_ptr<ClientSession>& client, const int id, const std::string& username) {
    client->setUser(id, username);
    client->setIsAuthenticated(true);
    sessionManager_.add(client);

    const std::string connectToken = generateConnectToken();
    client->setTempToken(connectToken);
    temporaryTokenManager_.createSession(id, connectToken);

    LOG_INFO(HANDLER, "Сессия авторизована. socket_fd=", client->getSocket(),
             " userId=", id, " username='", username, "' token=", connectToken.substr(0, 8), "...");

    return connectToken;
}

void Handler::sendMessage(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    int clientUserId = client->getUserId();

    if (!client->getIsAuthenticated()) {
        LOG_WARNING(HANDLER, "Попытка отправки сообщения без авторизации. socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::errorMessage("Not authenticated"));
        return;
    }

    Message msg;
    try {
        msg = j.at("data").get<Message>();
    } catch ([[maybe_unused]] const std::exception& e) {
        LOG_WARNING(HANDLER, "Неверный формат сообщения от userId=", clientUserId);
        dispatcher_.sendTo(client, protocol::errorMessage("Invalid message format"));
        return;
    }

    if (msg.sender_id != clientUserId) {
        LOG_WARNING(HANDLER, "Попытка подмены sender_id! Заявлено: ", msg.sender_id, " реально: ", clientUserId, " userId=", clientUserId);
        dispatcher_.sendTo(client, protocol::errorMessage("Forbidden: sender_id mismatch"));
        return;
    }

    msg.sender_id = clientUserId;

    if(const auto error = Validator::validateMessage(msg); error.has_value()) {
        LOG_WARNING(HANDLER, "Ошибка валидации сообщения от userId=", clientUserId, " ошибка: ", error.value());
        dispatcher_.sendTo(client, protocol::errorMessage(error.value()));
        return;
    }

    if (std::holds_alternative<TextContent>(msg.payload)) {
        auto&[text] = std::get<TextContent>(msg.payload);
        std::string cleanText = Validator::sanitize(text);
        if (cleanText.empty()) {
            LOG_WARNING(HANDLER, "Попытка отправки пустого сообщения от userId=", clientUserId);
            dispatcher_.sendTo(client, protocol::errorMessage("Message is empty"));
            return;
        }
        text = std::move(cleanText);
    }

    msg.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    const auto saved_id = messageManager_.saveMessage(msg);
    if (!saved_id.has_value()) {
        LOG_CRITICAL(HANDLER, "Критическая ошибка БД: не удалось сохранить сообщение. dialogId=", msg.dialog_id);
        dispatcher_.sendTo(client, protocol::errorMessage("Database error"));
        return;
    }

    msg.id = saved_id.value();
    dialogManager_.updateLastMessage(msg);

    const auto& participants = dialogManager_.getDialogParticipants(msg.dialog_id);
    int sentCount = 0;
    for (const auto participant : participants) {
        if (participant == msg.sender_id) {
            continue;
        }

        if (const auto& receiver = sessionManager_.getByUserId(participant)) {
            if (receiver->send(protocol::sendMessage(msg))) {
                sentCount++;
            } else {
                LOG_WARNING(HANDLER, "Не удалось доставить сообщение получателю. msgId=", msg.id, " receiverUserId=", participant);
            }
        }
    }

    LOG_INFO(HANDLER, "Сообщение успешно обработано и отправлено. msgId=", msg.id, " dialogId=", msg.dialog_id, " получателей=", sentCount);
}

void Handler::setDisconnectHandler(const std::function<void(std::shared_ptr<ClientSession>)> &cb) {
    dispatcher_.onDisconnect = cb;
}

void Handler::searchUserRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if (!client->getIsAuthenticated()) {
        LOG_WARNING(HANDLER, "Попытка поиска без авторизации. socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::errorMessage("Not authenticated"));
        return;
    }

    std::string query = j.value("username", "");
    if(std::string error; !Validator::valid_string_field(j, "username", Validator::search, error)) {
        LOG_WARNING(HANDLER, "Ошибка валидации поиска от userId=", client->getUserId(), " запрос='", query, "'");
        dispatcher_.sendTo(client, protocol::errorMessage(error));
        return;
    }

    const auto users = userManager_.searchUsers(query);
    LOG_DEBUG(HANDLER, "Поиск пользователей. userId=", client->getUserId(), " запрос='", query, "' найдено=", users.size());

    dispatcher_.sendTo(client, protocol::searchUserResponse(users));
}

void Handler::historyRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    int clientUserId = client->getUserId();

    if (!client->getIsAuthenticated()) {
        LOG_WARNING(HANDLER, "Попытка запроса истории без авторизации. socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::errorMessage("Not authenticated"));
        return;
    }

    if (!j.contains("dialog_id") || !j.contains("last_msg_id") || !j.contains("limit")) {
        LOG_WARNING(HANDLER, "Отсутствуют обязательные поля в historyRequest от userId=", clientUserId);
        dispatcher_.sendTo(client, protocol::errorMessage("Missing required fields"));
        return;
    }

    int64_t dialog_id;
    int64_t last_msg_id;
    int limit;

    try {
        dialog_id = j["dialog_id"].get<int64_t>();
        last_msg_id = j["last_msg_id"].get<int64_t>();
        limit = j["limit"].get<int>();
    } catch (const std::exception& e) {
        LOG_WARNING(HANDLER, "Неверный тип параметров в historyRequest от userId=", clientUserId, " ошибка=", e.what());
        dispatcher_.sendTo(client, protocol::errorMessage("Invalid parameter types"));
        return;
    }

    if (limit <= 0 || limit > 100) {
        LOG_WARNING(HANDLER, "Недопустимый limit в historyRequest от userId=", clientUserId, " limit=", limit);
        dispatcher_.sendTo(client, protocol::errorMessage("Limit must be between 1 and 100"));
        return;
    }

    if (auto participants = dialogManager_.getDialogParticipants(dialog_id); std::ranges::find(participants, clientUserId) == participants.end()) {
        LOG_WARNING(HANDLER, "Попытка доступа к чужому диалогу! userId=", clientUserId, " dialogId=", dialog_id);
        dispatcher_.sendTo(client, protocol::errorMessage("You are not a participant of this dialog"));
        return;
    }

    const auto history = messageManager_.getHistory(dialog_id, last_msg_id, limit);
    LOG_DEBUG(HANDLER, "История запрошена. userId=", clientUserId, " dialogId=", dialog_id,
              " возвращено=", history.size(), " из запрошенных ", limit);

    dispatcher_.sendTo(client, protocol::historyResponse(!history.empty(), dialog_id, history));
}

void Handler::getDialogsRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if(!client->getIsAuthenticated()) {
        LOG_WARNING(HANDLER, "Попытка запроса диалогов без авторизации. socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::errorMessage("Not authenticated"));
        return;
    }

    const auto dialogs = dialogManager_.getUserDialogs(client->getUserId());
    LOG_DEBUG(HANDLER, "Запрос списка диалогов. userId=", client->getUserId(), " найдено=", dialogs.size());

    dispatcher_.sendTo(client, protocol::getDialogsResponse(!dialogs.empty(), dialogs));
}

void Handler::ping(const std::shared_ptr<ClientSession> &client, const nlohmann::json& j) {
    SessionManager::updateActivity(client);
    LOG_DEBUG(HANDLER, "Ping получен от userId=", client->getUserId());
}

void Handler::resumeConnectionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    const std::string token = j.value("token", "");

    if (token.empty()) {
        LOG_WARNING(HANDLER, "resumeConnection: пустой токен от socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    if (!temporaryTokenManager_.isValid(token)) {
        LOG_WARNING(HANDLER, "resumeConnection: невалидный токен=", token.substr(0, 8), "... от socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    const auto user_id = temporaryTokenManager_.getUserIdByToken(token);
    if(!user_id.has_value()) {
        LOG_CRITICAL(HANDLER, "resumeConnection: токен валиден, но userId не найден в БД! token=", token.substr(0, 8), "...");
        dispatcher_.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    const auto username = userManager_.getUsername(user_id.value());
    if (!username.has_value()) {
        LOG_CRITICAL(HANDLER, "resumeConnection: userId=", user_id.value(), " найден, но username не найден в БД!");
        dispatcher_.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    client->setUser(user_id.value(), username.value());
    client->setIsAuthenticated(true);
    sessionManager_.add(client);

    LOG_INFO(HANDLER, "Успешное восстановление соединения. userId=", user_id.value(), " username='", username.value(), "' token=", token.substr(0, 8), "...");

    dispatcher_.sendTo(client, protocol::resumeConnectionResponse(true));
}

void Handler::resumeSessionRequest(const std::shared_ptr<ClientSession> &client, const nlohmann::json &j) {
    const std::string token = j.value("token", "");

    if (token.empty()) {
        LOG_WARNING(HANDLER, "resumeSession: пустой токен от socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::resumeSessionResponse(false));
        return;
    }

    if (!devicesSessionManager_.isValid(token)) {
        LOG_WARNING(HANDLER, "resumeSession: невалидный токен сессии=", token.substr(0, 8), "... от socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::resumeSessionResponse(false));
        return;
    }

    const auto user_id = devicesSessionManager_.getUserIdByToken(token);
    if (!user_id.has_value()) {
        LOG_CRITICAL(HANDLER, "resumeSession: токен сессии валиден, но userId не найден в БД! token=", token.substr(0, 8), "...");
        dispatcher_.sendTo(client, protocol::resumeSessionResponse(false));
        return;
    }

    const auto username = userManager_.getUsername(user_id.value());
    if (!username.has_value()) {
        LOG_CRITICAL(HANDLER, "resumeSession: userId=", user_id.value(), " найден, но username не найден в БД!");
        dispatcher_.sendTo(client, protocol::resumeSessionResponse(false));
        return;
    }

    devicesSessionManager_.updateActivity(token);
    const auto tempToken = authSuccess(client, user_id.value(), username.value());

    LOG_INFO(HANDLER, "Успешное восстановление сессии. userId=", user_id.value(), " username='", username.value(), "' новый connectToken=", tempToken.substr(0, 8), "...");

    dispatcher_.sendTo(client, protocol::resumeSessionResponse(true, tempToken));
}

void Handler::openDialogRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if (!client->getIsAuthenticated()) {
        LOG_WARNING(HANDLER, "Попытка открытия диалога без авторизации. socket_fd=", client->getSocket());
        dispatcher_.sendTo(client, protocol::openDialogResponse(false, 0, User{}, {}, "Not authenticated"));
        return;
    }

    const int userId = client->getUserId();

    int targetId = 0;
    try {
        targetId = j.at("target_user_id").get<int>();
    } catch (const std::exception& e) {
        LOG_WARNING(HANDLER, "Неверный target_user_id в openDialogRequest от userId=", userId, " ошибка=", e.what());
        dispatcher_.sendTo(client, protocol::openDialogResponse(false, 0, User{}, {}, "Invalid target_user_id"));
        return;
    }

    if (targetId == userId) {
        dispatcher_.sendTo(client, protocol::openDialogResponse(false, 0, User{}, {}, "Cannot open dialog with yourself"));
        return;
    }

    const auto peerName = userManager_.getUsername(targetId);
    if (!peerName.has_value()) {
        LOG_WARNING(HANDLER, "openDialog: пользователь не найден. userId=", userId, " targetId=", targetId);
        dispatcher_.sendTo(client, protocol::openDialogResponse(false, 0, User{}, {}, "User not found"));
        return;
    }

    const auto dialogId = dialogManager_.findOrCreatePrivateDialog(userId, targetId);
    if (!dialogId.has_value()) {
        dispatcher_.sendTo(client, protocol::openDialogResponse(false, 0, User{}, {}, "Database error"));
        return;
    }

    auto history = messageManager_.getHistory(*dialogId, std::numeric_limits<int64_t>::max(), 50);
    std::ranges::reverse(history);

    LOG_INFO(HANDLER, "Диалог открыт. userId=", userId, " targetId=", targetId, " dialogId=", *dialogId, " сообщений=", history.size());

    dispatcher_.sendTo(client, protocol::openDialogResponse(
        true, *dialogId, User{.username = *peerName, .user_id = targetId}, history));
}