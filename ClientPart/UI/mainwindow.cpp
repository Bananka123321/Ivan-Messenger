#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent, AppState* state, DialogManager* manager)
    : QMainWindow(parent), ui(new Ui::MainWindows), state_(state), manager_(manager) {
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
        Message localMsg{
            -1,
            state_->getCurrentUserId(),
            selectedUserId,
            text.toStdString(),
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
        };

        manager_->addMessage(selectedUserId, localMsg);

        emit sendMessageRequest(selectedUserId, text.toStdString());

        ui->inputField->clear();
    });

    connect(ui->userWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        int id = item->data(Qt::UserRole).toInt();
        openDialog(id);
    });

    connect(manager, &DialogManager::messagesUpdated, this, [this](int peer_id){
        if(peer_id == selectedUserId) refreshCurrentChat();
    });

    connect(manager, &DialogManager::findUsers, this, [this](const std::vector<User>& users){
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
        auto it = searchMap.find(text.toStdString());
        if(it != searchMap.end()) openDialog(it->second);
    });

    connect(manager, &DialogManager::historyLoaded, this, [this](int peer_id, const std::vector<Message>& messages){
        if (peer_id == selectedUserId)
            prependMessagesToView(messages);
    });

    // QScrollBar* sb = ui->chatView->verticalScrollBar();  //future
    // connect(sb, &QScrollBar::valueChanged, this, [this, sb](int value){
    //     if (value == sb->minimum() && selectedUserId != -1) {
    //         loadMoreHistory();
    //     }
    // });

    connect(manager, &DialogManager::dialogsUpdated, this, &MainWindow::refreshDialogs);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshCurrentChat() {
    ui->chatView->clear();
    if(selectedUserId == -1) return;

    std::string peerName = state_->getUsername(selectedUserId);

    const auto* messages = manager_->getMessages(selectedUserId);
    if(!messages) return;

    for (const auto& msg : *messages) {
        if(msg.senderId == selectedUserId)
            ui->chatView->append(QString::fromStdString("[" + peerName + "] " + msg.text));
        else
            ui->chatView->append(QString::fromStdString("[YOU] " + msg.text));
    }

    ui->chatView->verticalScrollBar()->setValue(ui->chatView->verticalScrollBar()->maximum());
}

void MainWindow::prependMessagesToView(const std::vector<Message>& messages) {
    if(messages.empty()) return;

    auto* scrollbar = ui->chatView->verticalScrollBar();
    int oldMax = scrollbar->maximum();
    int oldValue = scrollbar->value();
    bool wasAtBottom = (oldValue >= oldMax - 10);

    QTextCursor cursor = ui->chatView->textCursor();
    cursor.movePosition(QTextCursor::Start);

    std::string peerName = state_->getUsername(selectedUserId);

    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        const auto& msg = *it;
        QString line;
        if(msg.senderId == state_->getCurrentUserId())
            line = QString::fromStdString("[YOU] " + msg.text);
        else
            line = QString::fromStdString("[" + peerName + "] " + msg.text);

        cursor.insertText(line + "\n");
    }

    int newMax = scrollbar->maximum();
    if(wasAtBottom) scrollbar->setValue(newMax);
    else scrollbar->setValue(oldValue + newMax - oldMax);
}

void MainWindow::openDialog(int id) {
    selectedUserId = id;
    ui->chatView->clear();
    ui->user->setText(QString::fromStdString(state_->getUsername(id)));
    ui->inputField->setFocus();

    emit loadHistoryRequest(id, std::numeric_limits<int>::max());
}

void MainWindow::appendMessageToView(const Message& msg) {
    QString html = QString::fromStdString((msg.senderId == state_->getCurrentUserId() ? "[YOU] " : "[" + state_->getUsername(msg.senderId) + "] ") + msg.text);
    ui->chatView->append(html);
    ui->chatView->verticalScrollBar()->setValue(ui->chatView->verticalScrollBar()->maximum());
}

void MainWindow::refreshDialogs() {
    ui->userWidget->clear();
    auto dialogs = manager_->getDialogs();
    for(const auto& dialog : dialogs) {
        QString displayText = QString::fromStdString(dialog.username + ": " + dialog.last_msg_text);

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, dialog.peer_id);

        ui->userWidget->addItem(item);
    }

    if(selectedUserId != -1) {
        auto items = ui->userWidget->findItems(QString(), Qt::MatchContains);
        for(auto* item : items) {
            if(item->data(Qt::UserRole).toInt() == selectedUserId) {
                ui->userWidget->setCurrentItem(item);
                break;
            }
        }
    }
}
