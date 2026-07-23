#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent, AppState* state, DialogManager* manager)
    : QMainWindow(parent), ui(new Ui::MainWindows), state_(state), manager_(manager) {
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, [this]() {
        QString text = ui->inputField->text();
        if (text.isEmpty() || selectedUserId == -1) return;

        std::string cleanText = Validator::sanitize(text.toStdString());

        Message localMsg{
            -1,
            state_->getCurrentUserId(),
            selectedUserId,
            cleanText,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
        };

        manager_->addMessage(selectedUserId, localMsg);

        emit sendMessageRequest(selectedUserId, cleanText);

        ui->inputField->clear();
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::appendMessageToView(const Message& msg) {
    QString html = buildMessageHtml(msg);
    QTextCursor cursor(ui->chatView->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);

    ui->chatView->verticalScrollBar()->setValue(ui->chatView->verticalScrollBar()->maximum());
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
