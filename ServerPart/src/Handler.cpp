#include "../include/Handler.h"

Handler::Handler(SessionManager& sm) : sessionManager(sm), dispatcher(sessionManager), userManager(Config::getDB().getConnectionStr()), msgManager(Config::getDB().getConnectionStr()), diaManager(Config::getDB().getConnectionStr()) {
    handlers["loginRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        loginRequest(client, j);
    };

    handlers["registerRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        registerRequest(client, j);
    };

    handlers["privateMessage"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        privateMessage(client, j);
    };

    handlers["searchUserRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        searchUserRequest(client, j);
    };

    handlers["historyRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        historyRequest(client, j);
    };

    handlers["getDialogsRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        getDialogsRequest(client, j);
    };

    handlers["ping"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        ping(client, j);
    };

    handlers["resumeConnectionRequest"] = [this](const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
        resumeConnectionRequest(client, j);
    };
}

Handler::~Handler() = default;

void Handler::handleMessage(const std::shared_ptr<ClientSession>& client, std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        sessionManager.updateActivity(client);
        if (const std::string type = j["type"]; handlers.contains(type)) {
            handlers[type](client, j);
        }
        else
            std::cerr << "Unknown message type " << type << std::endl;    

    } catch (const std::exception& e) {
        std::cerr << "JSON parse ERROR " << e.what() << std::endl;
    }
}

static std::mt19937& getGlobalRNG() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

std::string Handler::generateToken() {
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
    if(std::string error; !Validator::valid_string_field(j, "username", Validator::username, error) || !Validator::valid_string_field(j, "password", Validator::password, error)) {
        dispatcher.sendTo(client, protocol::loginResponse(false, -1, "", error));
        return;
    }

    auto [success, user_id, error] = userManager.loginUser(j["username"], j["password"]);
    if (!success) {
        dispatcher.sendTo(client, protocol::loginResponse(success, user_id, j["username"], "", error));
        return;
    }

    authSuccess(client, user_id, j["username"]);
}

void Handler::registerRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if(std::string error; !Validator::valid_string_field(j, "username", Validator::username, error) || !Validator::valid_string_field(j, "password", Validator::password, error)) {
        dispatcher.sendTo(client, protocol::registerResponse(false, -1, "", error));
        return;
    }
    
    auto [success, user_id, error] = userManager.registerUser(j["username"],j["password"]);
    if (!success) {
        dispatcher.sendTo(client, protocol::loginResponse(success, user_id, j["username"], "", error));
        return;
    }

    authSuccess(client, user_id, j["username"]);
}

void Handler::authSuccess(const std::shared_ptr<ClientSession>& client, const int& id, const std::string& username) {
    client->setUser(id, username);
    client->set_is_authentificated(true);
    const std::string token = generateToken();
    userManager.createSession(id, token);
    dispatcher.sendTo(client, protocol::loginResponse(true, id, username, token, ""));
    sessionManager.add(client);
}

void Handler::privateMessage(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if (!client->get_is_authentificate()) {
        std::cerr << "SORRY, not auth\n";
        return;
    }
    
    std::string error;
    if(!Validator::valid_string_field(j, "text", Validator::message, error)) {
        dispatcher.sendTo(client, protocol::errorMessage(error));
        return;
    }

    if(!Validator::valid_int_field(j, "to", 1, std::numeric_limits<long int>::max(), error)) {
        dispatcher.sendTo(client, protocol::errorMessage(error));
        return;
    }

    std::string cleanText = Validator::sanitize(j["text"].get<std::string_view>());
    if (cleanText.empty()) {
        dispatcher.sendTo(client, protocol::errorMessage("Message is empty after sanitization"));
        return;
    }

    const int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const int msg_id = msgManager.saveMessage(client->getUserId(), j["to"], cleanText, timestamp);
    
    diaManager.insertDialog(client->getUserId(), j["to"], msg_id, cleanText, timestamp);
    diaManager.insertDialog(j["to"], client->getUserId(), msg_id, cleanText, timestamp);

    const auto receiver = sessionManager.getByUserId(j["to"]);
    if (!receiver) return;

    nlohmann::json out = j;
    out["text"] = cleanText;
    if (!receiver->send(out.dump())) {
        std::cerr << "Failed to send message to user " << j["to"] << '\n';
    }
}

void Handler::setDisconnectHandler(const std::function<void(std::shared_ptr<ClientSession>)> &cb) {
    dispatcher.onDisconnect = cb;
}

void Handler::searchUserRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if(std::string error; !Validator::valid_string_field(j, "username", Validator::search, error)) {
        dispatcher.sendTo(client, protocol::errorMessage(error));
        return;
    }

    const auto users = userManager.searchUsers(j["username"]);

    dispatcher.sendTo(client, protocol::searchUserResponse(users));
}

void Handler::historyRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if(!client->get_is_authentificate()) return;
    const auto history = msgManager.getHistory(client->getUserId(), j["peer_id"], j["last_msg_id"], j["limit"]);
    dispatcher.sendTo(client, protocol::historyResponse(!history.empty(), j["peer_id"], history));
}

void Handler::getDialogsRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    if(!client->get_is_authentificate()) return;

    const auto dialogs = diaManager.getUserDialogs(client->getUserId());
    std::vector<MetaDialog> metas;
    for (const auto&[peer_id, username, last_msg_text, last_msg_timestamp, last_activity_time] : dialogs)
        metas.push_back({.peer_id = peer_id, .username = username, .last_msg_text = last_msg_text, .last_msg_timestamp = last_msg_timestamp, .last_activity_time = last_activity_time});

    dispatcher.sendTo(client, protocol::getDialogsResponse(!metas.empty(), metas));
}

void Handler::ping(const std::shared_ptr<ClientSession> &client, const nlohmann::json& j) const {
    sessionManager.updateActivity(client);
}

void Handler::resumeConnectionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j) {
    const std::string token = j.value("token", "");
    if (token.empty()) {
        std::cerr << "token empty\n";
        dispatcher.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    const auto user_id = userManager.getUserIdByToken(token);
    if(!user_id.has_value()) {
        dispatcher.sendTo(client, protocol::resumeConnectionResponse(false));
        return;
    }

    const int id = user_id.value();
    client->setUser(id, "hz");
    client->set_is_authentificated(true);
    sessionManager.add(client);
    dispatcher.sendTo(client, protocol::resumeConnectionResponse(true));
}