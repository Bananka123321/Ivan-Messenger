#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, AppState* state, Handler* handler)
    : QMainWindow(parent), ui(new Ui::MainWindows), handler(handler), state(state) {
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, [this]() {
        QString text = ui->inputField->text();
        if (text.isEmpty() || selectedUserId == -1) return;

        chats[selectedUserId].push_back("[You] " + text.toStdString());

        emit sendMessageRequest(selectedUserId, text.toStdString());

        ui->chatView->append(QString::fromStdString("[You] " + text.toStdString()));
        ui->inputField->clear();
    });

    connect(ui->userWidget, &QListWidget::itemClicked, this, [this, state](QListWidgetItem* item) {
        QVariant data = item->data(Qt::UserRole);

        if (!data.isValid()) {
            std::cerr << "ERROR: no user id in item\n";
            return;
        }

        selectedUserId = data.toInt();
        std::string name = state->getUsername(selectedUserId);

        ui->user->setText(QString::fromStdString(name));
        ui->chatView->clear();
        for (const auto& msg : chats[selectedUserId])
            ui->chatView->append(QString::fromStdString(msg));
    });

    connect(state, &AppState::usersChanged, this, [this](const std::unordered_map<int, std::string>& users) {
        updateUsers(users);
    });

    updateUsers(state->getUsers());
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updateUsers(const std::unordered_map<int, std::string>& users) {
    ui->userWidget->clear();

    for (auto& user : users) {
        if (user.first == state->getCurrentUserId()) continue;

        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(user.second));
        item->setData(Qt::UserRole, user.first);

        ui->userWidget->addItem(item);
    }
}

void MainWindow::newMessage(int sender, const std::string& text) {
    std::string name = state->getUsername(sender);
    chats[sender].push_back("[" + name + "] " + text);

    if (selectedUserId == sender)
        ui->chatView->append(QString::fromStdString("[" + name + "] " + text));
}
