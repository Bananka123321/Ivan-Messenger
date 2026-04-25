#include "loginwindow.h"
#include "ui_loginwindow.h"


LoginWindow::LoginWindow(QWidget *parent_, MessageRouter* router_)
    : QDialog(parent_), ui(new Ui::LoginWindow), router(router_) {
    ui->setupUi(this);

    connect(ui->okButton, &QPushButton::clicked, this, [this]() {
        login = ui->loginField->text().toStdString();
        std::string password = ui->passwordField->text().toStdString();

        if (login.empty() || password.empty()) return;

        emit loginRequest(login, password);
    });

    connect(ui->okButton_2, &QPushButton::clicked, this, [this]() {
        login = ui->loginRegField->text().toStdString();
        std::string password = ui->passwordRegField->text().toStdString();

        if (login.empty() || password.empty()) return;
        if (password != ui->passwordRepeateField->text().toStdString()) return;

        emit registerRequest(login, password);
    });


    connect(ui->signInLink, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "login")
            ui->stackedWidget->setCurrentIndex(1);
    });

    connect(ui->registrLink, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "registr")
            ui->stackedWidget->setCurrentIndex(0);
    });
}

LoginWindow::~LoginWindow() {
    delete ui;
}

void LoginWindow::loginSuccess() {
    accept();
}

void LoginWindow::loginFailed(const std::string& reason) {
    std::cerr << reason << '\n';
}

void LoginWindow::registerSuccess() {
    accept();
}

void LoginWindow::registerFailed(const std::string& reason) {
    std::cerr << reason << '\n';
}
