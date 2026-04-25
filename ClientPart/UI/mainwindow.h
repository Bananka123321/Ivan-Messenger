#pragma once

#include <QMainWindow>
#include <unordered_map>

#include "Handler.h"
#include "AppState.h"
#include "DialogManager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindows;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent_, AppState* state_, DialogManager* mnager_);
    ~MainWindow();

    void newMessage(int sender, const std::string& text);

signals:
    void sendMessageRequest(const int& to, const std::string& text);

private:
    Ui::MainWindows *ui;

    AppState* state;
    DialogManager* manager;

    int selectedUserId = -1;

    void refreshDialogs();
    void refreshCurrentChat();

    void updateSingleDialog(int peerId);
};
