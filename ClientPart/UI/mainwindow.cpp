#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent_, AppState* state_, DialogManager* manager_)
    : QMainWindow(parent_), ui(new Ui::MainWindows), state(state_), manager(manager_) {
    ui->setupUi(this);

    model = new QStringListModel(this);

    completer = new QCompleter(model, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);

    ui->searchLineEdit->setCompleter(completer);

    QTimer* searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, [this, searchTimer](const QString& text) {
        if (text.isEmpty()) return;

        searchTimer->start(300);
    });

    connect(searchTimer, &QTimer::timeout, this, [this]() {
        emit searchUser(ui->searchLineEdit->text().toStdString());
    });

    connect(ui->sendButton, &QPushButton::clicked, this, [this]() {
        QString text = ui->inputField->text();
        if (text.isEmpty() || selectedUserId == -1) return;

        int myId = state->getCurrentUserId();
        Message localMsg{myId, selectedUserId, text.QString::toStdString()};

        emit sendMessageRequest(selectedUserId, text.toStdString());

        ui->inputField->clear();
        // ui->chatView->append("[YOU] " + text);
        // ui->chatView->verticalScrollBar()->setValue(ui->chatView->verticalScrollBar()->maximum());

        manager->addMessage(localMsg);
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

    connect(manager, &DialogManager::findUsers, this, [this](const std::vector<protocol::User>& users){
        QStringList list;
        searchMap.clear();

        for (const auto& u : users) {
            QString name = QString::fromStdString(u.username);
            list << name;
            searchMap[u.username] = u.user_id;
        }

        model->setStringList(list);

        completer->complete();
    });

    connect(completer, QOverload<const QString&>::of(&QCompleter::activated), this, [this](const QString& text){
        std::string username = text.toStdString();

        if(searchMap.find(username) == searchMap.end()) return;

        int id = searchMap[username];

        openDialog(id);
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

    const Dialog* dialog = manager->getDialog(selectedUserId);
    if(!dialog) return;

    const auto& messages = dialog->messages;
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

            item->setText(name + "] " + last);

            QListWidgetItem* taken = ui->userWidget->takeItem(i);
            ui->userWidget->insertItem(0, taken);

            return;
        }
    }

    const Dialog* dialog = manager->getDialog(peerId);
    if(!dialog) return;

    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(state->getUsername(peerId) + "]\t" + dialog->lastMessage));
    item->setData(Qt::UserRole, peerId);
    ui->userWidget->insertItem(0, item);
}

void MainWindow::openDialog(int id) {
    selectedUserId = id;
    ui->user->setText(QString::fromStdString(state->getUsername(selectedUserId)));

    renderMessages(selectedUserId);
}

void MainWindow::renderMessages(int userId) {
    ui->chatView->clear();

    const Dialog* dialog = manager->getDialog(userId);
    if (!dialog) {
        ui->chatView->setPlainText("go chatting with - " + QString::fromStdString(state->getUsername(userId)));
        ui->inputField->setFocus();
        return;
    }

    for (const auto& msg : dialog->messages) {
        if (msg.senderId == userId)
            ui->chatView->append(QString::fromStdString("[" + state->getUsername(userId) + "] " + msg.text));
        else
            ui->chatView->append(QString::fromStdString("[YOU] " + msg.text));
    }
}
