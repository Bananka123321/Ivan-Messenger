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

    connect(manager, &DialogManager::dialogsUpdated, this, &MainWindow::refreshDialogs);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshCurrentChat() {
    ui->chatView->setUpdatesEnabled(false);
    ui->chatView->clear();

    if (selectedUserId == -1) {
        ui->chatView->setUpdatesEnabled(true);
        return;
    }

    const auto* messages = manager_->getMessages(selectedUserId);
    if (!messages) {
        ui->chatView->setUpdatesEnabled(true);
        return;
    }

    QString fullHtml;
    for (const auto& msg : *messages) {
        fullHtml += buildMessageHtml(msg);
    }

    ui->chatView->setHtml(fullHtml);
    ui->chatView->setUpdatesEnabled(true);

    QScrollBar* sb = ui->chatView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::prependMessagesToView(const std::vector<Message>& messages) {
    if(messages.empty()) return;

    auto* scrollbar = ui->chatView->verticalScrollBar();
    int previousMaximum = scrollbar->maximum();
    int previousValue = scrollbar->value();

    ui->chatView->setUpdatesEnabled(false);

    QTextCursor cursor(ui->chatView->document());
    cursor.movePosition(QTextCursor::Start);

    QString fullHtml;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        fullHtml += buildMessageHtml(*it);
    }

    cursor.insertHtml(fullHtml);
    ui->chatView->setUpdatesEnabled(true);

    int delta = scrollbar->maximum() - previousMaximum;
    scrollbar->setValue(previousValue + delta);
}
void MainWindow::openDialog(int id) {
    selectedUserId = id;
    ui->chatView->clear();
    ui->user->setText(QString::fromStdString(state_->getUsername(id)));
    ui->inputField->setFocus();

    refreshCurrentChat();
    emit loadHistoryRequest(id, std::numeric_limits<int>::max());
}

void MainWindow::appendMessageToView(const Message& msg) {
    QString html = buildMessageHtml(msg);
    QTextCursor cursor(ui->chatView->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);

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

QString MainWindow::formatTime(int64_t timestamp) {
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dt.toString("HH:mm");
}

QString MainWindow::buildMessageHtml(const Message& msg) {
    bool isMe = (msg.senderId == state_->getCurrentUserId());

    QString align     = isMe ? "right" : "left";
    QString bgColor   = isMe ? "#89b4fa" : "#313244";
    QString textColor = isMe ? "#11111b" : "#cdd6f4";
    QString timeColor = isMe ? "#45475a" : "#a6adc8";
    QString timeStr   = formatTime(msg.timestamp);

    QString escapedText = QString::fromStdString(msg.text).toHtmlEscaped();
    escapedText.replace("\n", "<br>");

    return QString(
               "<table width='100%' style='margin: 4px 0px; border: none;'>"
               "  <tr>"
               "    <td align='%1' style='border: none; padding: 0px;'>"
               "      <table style='background-color: %2; border-radius: 12px; margin: 0px; border: none;' cellpadding='8' cellspacing='0'>"
               "        <tr>"
               "          <td style='border: none; padding: 6px 10px 4px 10px; max-width: 450px;'>"
               "            <span style='color: %3; font-size: 13px; font-family: \"Segoe UI\", sans-serif;'>%4</span>"
               "          </td>"
               "        </tr>"
               "        <tr>"
               "          <td align='right' style='border: none; padding: 0px 10px 4px 10px;'>"
               "            <span style='color: %5; font-size: 9px; font-family: \"Segoe UI\", sans-serif;'>%6</span>"
               "          </td>"
               "        </tr>"
               "      </table>"
               "    </td>"
               "  </tr>"
               "</table>"
               ).arg(align, bgColor, textColor, escapedText, timeColor, timeStr);
}
