#pragma once

#include <QMainWindow>
#include <unordered_map>
#include "Handler.h"
#include "AppState.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindows;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, AppState* state, Handler* ctrl);
    ~MainWindow();

    void newMessage(const std::string& sender, const std::string& text);

signals:
    void sendMessageRequest(const std::string& to, const std::string& text);

private:
    Ui::MainWindows *ui;

    Handler* handler;
    AppState* state;

    std::string selectedUser;
    std::unordered_map<std::string, std::vector<std::string>> chats;

    void updateUsers(const std::vector<std::string>& users);
};
