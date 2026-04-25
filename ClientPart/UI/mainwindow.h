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

    void newMessage(int sender, const std::string& text);

signals:
    void sendMessageRequest(const int& to, const std::string& text);

private:
    Ui::MainWindows *ui;

    Handler* handler;
    AppState* state;

    int selectedUserId = -1;
    std::unordered_map<int, std::vector<std::string>> chats;

    void updateUsers(const std::unordered_map<int, std::string>& users);
};
