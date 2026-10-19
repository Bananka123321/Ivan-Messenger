#include "Handler.h"
#include "Logging.h"

Handler::Handler() {
    qCDebug(logHandler) << "Инициализация Handler. Регистрация обработчиков сообщений";

    handlers_["loginResponse"] = [this] (const nlohmann::json& j) {
        onLoginResponse(j["success"], j["user_id"], j["username"], j["connectToken"], j["sessionToken"], j.value("error", ""));
    };

    handlers_["registerResponse"] = [this] (const nlohmann::json& j) {
        onRegisterResponse(j["success"], j["user_id"], j["username"], j["connectToken"], j["sessionToken"], j.value("error", ""));
    };

    handlers_["sendMessage"] = [this] (const nlohmann::json& j) {
        onSendMessage(j["data"]);
    };

    handlers_["searchUserResponse"] = [this] (const nlohmann::json& j) {
        onSearchUserResponse(j["result"]);
    };

    handlers_["error"] = [this] (const nlohmann::json& j) {
        onErrorMessage(j["message"]);
    };

    handlers_["historyResponse"] = [this] (const nlohmann::json& j) {
        onHistoryResponse(j["success"], j["peer_id"], j["messages"], j.value("error", ""));
    };

    handlers_["getDialogsResponse"] = [this] (const nlohmann::json& j) {
        onGetDialogsResponse(j["success"], j["dialogs"], j.value("error", ""));
    };

    handlers_["resumeConnectionResponse"] = [this] (const nlohmann::json& j) {
        onResumeConnectionResponse(j["success"]);
    };

    handlers_["resumeSessionResponse"] = [this] (const nlohmann::json& j) {
        onResumeSessionResponse(j["success"], j.value("token", ""));
    };

    handlers_["openDialogResponse"] = [this](const nlohmann::json& j) {
        if (!j.value("success", false)) {
            onOpenDialogResponse(false, 0, User{}, {}, j.value("error", "Unknown error"));
            return;
        }
        onOpenDialogResponse(true, j.at("dialog_id").get<int64_t>(), j.at("peer").get<User>(), j.at("messages").get<std::vector<Message>>(), "");
    };
}

void Handler::handleMessage(std::string_view msg) {
    qCDebug(logHandler) << "Получено сырое сообщение, размер:" << msg.size() << "байт";

    try {
        auto j = nlohmann::json::parse(msg);
        std::string type = j.value("type", "unknown");

        qCDebug(logHandler) << "Успешно распарсен JSON. Тип сообщения:" << QString::fromStdString(type);

        if (handlers_.contains(type)) {
            handlers_[type](j);
        } else {
            qCWarning(logHandler) << "Неизвестный тип сообщения:" << QString::fromStdString(type);
        }
    } catch (const std::exception& e) {
        qCCritical(logHandler) << "Критическая ошибка парсинга JSON:" << e.what();
    }
}

//=============================================================================================

void Handler::onLoginResponse(const bool success, const int user_id, const std::string& login, const std::string& connectionToken, const std::string& sessionToken, const std::string& reason) {
    if (success) {
        qCInfo(logAuth) << "Ответ сервера: Успешный вход. userId=" << user_id << "login=" << QString::fromStdString(login);
        emit S_loginSuccess(login, user_id, connectionToken, sessionToken);
    } else {
        qCWarning(logAuth) << "Ответ сервера: Ошибка входа. Причина:" << QString::fromStdString(reason);
        emit S_loginFailed(reason);
    }
}

void Handler::onRegisterResponse(const bool success, const int user_id, const std::string& login, const std::string& connectionToken, const std::string& sessionToken, const std::string& reason) {
    if (success) {
        qCInfo(logAuth) << "Ответ сервера: Успешная регистрация. userId=" << user_id << "login=" << QString::fromStdString(login);
        emit S_registerSuccess(login, user_id, connectionToken, sessionToken);
    } else {
        qCWarning(logAuth) << "Ответ сервера: Ошибка регистрации. Причина:" << QString::fromStdString(reason);
        emit S_registerFailed(reason);
    }
}

void Handler::onSendMessage(const Message& msg) {
    qCDebug(logHandler) << "Получено сообщение от сервера. dialog_id=" << msg.dialog_id << "msg_id=" << msg.id;
    emit S_Message(msg);
}

void Handler::onSearchUserResponse(const std::vector<User>& users) {
    qCInfo(logDialog) << "Ответ сервера: Результаты поиска пользователей. Найдено:" << users.size();
    emit S_UserSearch(users);
}

void Handler::onErrorMessage(const std::string& text) {
    qCWarning(logHandler) << "Получено сообщение об ошибке от сервера:" << QString::fromStdString(text);
}

void Handler::onHistoryResponse(const bool success, const int64_t dialog_id, const std::vector<Message>& messages, const std::string& reason) {
    if(success) {
        qCInfo(logDialog) << "Ответ сервера: История диалога загружена. dialog_id=" << dialog_id << "сообщений:" << messages.size();
        emit S_HistoryLoaded(dialog_id, messages);
    } else {
        qCWarning(logDialog) << "Ответ сервера: Ошибка загрузки истории. dialog_id=" << dialog_id << "Причина:" << QString::fromStdString(reason);
        onErrorMessage(reason);
    }
}

void Handler::onGetDialogsResponse(const bool success, const std::vector<MetaDialog_Client>& dialogs, const std::string& reason) {
    if(success) {
        qCInfo(logDialog) << "Ответ сервера: Список диалогов загружен. Количество:" << dialogs.size();
        emit S_DialogsLoaded(dialogs);
    } else {
        qCWarning(logDialog) << "Ответ сервера: Ошибка загрузки списка диалогов. Причина:" << QString::fromStdString(reason);
        onErrorMessage(reason);
    }
}

void Handler::onResumeConnectionResponse(const bool success) {
    if(success) {
        qCInfo(logAuth) << "Ответ сервера: Соединение успешно восстановлено";
        emit S_ResumeConnectionSuccess();
    } else {
        qCWarning(logAuth) << "Ответ сервера: Отклонен запрос на восстановление соединения";
    }
}

void Handler::onResumeSessionResponse(const bool success, const std::string& token) {
    if (success) {
        qCInfo(logAuth) << "Ответ сервера: Сессия успешно восстановлена";
    } else {
        qCWarning(logAuth) << "Ответ сервера: Ошибка восстановления сессии";
    }
    emit S_ResumeSessionResponse(success, token);
}

void Handler::onOpenDialogResponse(bool success, int64_t dialog_id, const User& peer, const std::vector<Message>& messages, const std::string& error) {
    if (success) {
        emit S_DialogOpened(true, dialog_id, peer, messages, "");
    } else {
        emit S_DialogOpened(false, 0, User{}, {}, error);
    }
}