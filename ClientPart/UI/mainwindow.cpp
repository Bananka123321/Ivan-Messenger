#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent_, AppState* state_, DialogManager* manager_)
    : QMainWindow(parent_), ui(new Ui::MainWindows), state(state_), manager(manager_) {
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, [this]() {
        QString text = ui->inputField->text();
        if (text.isEmpty() || selectedUserId == -1) return;

        emit sendMessageRequest(selectedUserId, text.toStdString());
        ui->inputField->clear();
    });

    connect(ui->userWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        QVariant data = item->data(Qt::UserRole);

        selectedUserId = data.toInt();
        std::string name = state->getUsername(selectedUserId);

        ui->user->setText(QString::fromStdString(name));
        ui->chatView->clear();

        const auto& messages = manager->getDialog(selectedUserId)->messages;
        for (const auto& msg : messages) {
            if(msg.senderId == selectedUserId)
                ui->chatView->append(QString::fromStdString("[" + name + "] " + msg.text));
            else
                ui->chatView->append(QString::fromStdString("[YOU] " + msg.text));
        }
    });

    connect(manager, &DialogManager::dialogsUpdated, this, &MainWindow::refreshDialogs);

    connect(manager, &DialogManager::dialogUpdated, this, &MainWindow::updateSingleDialog);

    connect(manager, &DialogManager::dialogUpdated, this, [this](int peerId) {
        if(peerId != selectedUserId) return;
        refreshCurrentChat();
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshDialogs() {
    ui->userWidget->clear();

    const auto& dialogs = manager->getDialogs();


    for (const auto& [peerId, dialog] : dialogs) {
        QString name = QString::fromStdString(state->getUsername(peerId));
        QString last = QString::fromStdString(dialog.lastMessage);

        QListWidgetItem* item = new QListWidgetItem(name + "]\t" + last);

        item->setData(Qt::UserRole, peerId);
        ui->userWidget->addItem(item);
    }
}

void MainWindow::refreshCurrentChat() {
    ui->chatView->clear();

    std::string name = state->getUsername(selectedUserId);

    const auto& messages = manager->getDialog(selectedUserId)->messages;
    for (const auto& msg : messages) {
        if(msg.senderId == selectedUserId)
            ui->chatView->append(QString::fromStdString("[" + name + "] " + msg.text));
        else
            ui->chatView->append(QString::fromStdString("[YOU] " + msg.text));
    }
}

void MainWindow::updateSingleDialog(int peerId) {
    for (int i = 0; i < ui->userWidget->count(); i++) {
        QListWidgetItem* item = ui->userWidget->item(i);

        if (item->data(Qt::UserRole).toInt() == peerId) {
            const Dialog* dialog = manager->getDialog(peerId);
            if (!dialog) return;

            QString name = QString::fromStdString(state->getUsername(peerId));
            QString last = QString::fromStdString(dialog->lastMessage);

            item->setText(name + " | " + last);

            QListWidgetItem* taken = ui->userWidget->takeItem(i);
            ui->userWidget->insertItem(0, taken);

            return;
        }
    }

    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(state->getUsername(peerId) + "]\t" + manager->getDialog(peerId)->lastMessage));
    item->setData(Qt::UserRole, peerId);
    ui->userWidget->insertItem(0, item);
}
