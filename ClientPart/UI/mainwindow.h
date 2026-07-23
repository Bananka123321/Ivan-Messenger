#pragma once

#include <QMainWindow>
#include <unordered_map>
#include <QListWidget>
#include <QCompleter>
#include <QStringListModel>
#include <QDateTime>

#include "Handler.h"
#include "AppState.h"
#include "DialogManager.h"
#include "Validator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindows;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent, AppState* state, DialogManager* manager);
    ~MainWindow();

    // void newMessage(int sender, const std::string& text);

signals:
    void sendMessageRequest(const int& to, const std::string& text);
    void searchUser(const std::string& text);
    void loadHistoryRequest(int peer_id, int last_msg_id);

private:
    Ui::MainWindows *ui;

    QCompleter* completer;
    QStringListModel* model;

    AppState* state_;
    DialogManager* manager_;

    int selectedUserId = -1;

    void refreshDialogs();
    void refreshCurrentChat();
    void openDialog(int id);

    std::unordered_map<std::string, int> searchMap;

    void appendMessageToView(const Message& msg);
    void prependMessagesToView(const std::vector<Message>& messages);
    // void loadMoreHistory();

    QString formatTime(int64_t timestamp);
    QString buildMessageHtml(const Message& msg);
};
