#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, AppState* state, Handler* handler)
    : QMainWindow(parent), ui(new Ui::MainWindows), handler(handler), state(state) {
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, [this]() {
        QString text = ui->inputField->text();
        if (text.isEmpty() || selectedUser.empty()) return;

        chats[selectedUser].push_back("[You] " + text.toStdString());

        emit sendMessageRequest(selectedUser, text.toStdString());

        ui->chatView->append(QString::fromStdString("[You] " + text.toStdString()));
        ui->inputField->clear();
    });

    connect(ui->userWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        selectedUser = item->text().toStdString();
        ui->user->setText(item->text());
        ui->chatView->clear();
        for (const auto& msg : chats[selectedUser])
            ui->chatView->append(QString::fromStdString(msg));
    });

    connect(state, &AppState::usersChanged, this, [this](const std::vector<std::string>& users) {
        updateUsers(users);
    });

    updateUsers(state->getUsers());
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updateUsers(const std::vector<std::string>& users) {
    ui->userWidget->clear();

    for (auto& user : users) {
        if (user == state->getCurrentUser()) continue;
        ui->userWidget->addItem(QString::fromStdString(user));
    }
}

void MainWindow::newMessage(const std::string& sender, const std::string& text) {
    chats[sender].push_back("[" + sender + "] " + text);

    if (selectedUser == sender)
        ui->chatView->append(QString::fromStdString("[" + sender + "] " + text));
}
