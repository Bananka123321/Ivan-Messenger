#include "loginwindow.h"
#include "ui_loginwindow.h"


LoginWindow::LoginWindow(QWidget *parent_, MessageRouter* router_)
    : QDialog(parent_), ui(new Ui::LoginWindow), router(router_) {
    ui->setupUi(this);

    connect(ui->okButton, &QPushButton::clicked, this, [this]() {
        tryLogin();
    });

    connect(ui->okButton_2, &QPushButton::clicked, this, [this]() {
        tryRegister();
    });

    connect(ui->signInLink, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "login")
            ui->stackedWidget->setCurrentIndex(1);
    });

    connect(ui->registrLink, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "registr")
            ui->stackedWidget->setCurrentIndex(0);
    });

    connect(ui->loginField, &QLineEdit::textChanged, this, &LoginWindow::validateLoginForm);
    connect(ui->passwordField, &QLineEdit::textChanged, this, &LoginWindow::validateLoginForm);
    connect(ui->loginRegField, &QLineEdit::textChanged, this, &LoginWindow::validateRegisterForm);
    connect(ui->passwordRegField, &QLineEdit::textChanged, this, &LoginWindow::validateRegisterForm);
    connect(ui->passwordRepeateField, &QLineEdit::textChanged, this, &LoginWindow::validateRegisterForm);
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

void LoginWindow::tryLogin() {
    std::string login = ui->loginField->text().toStdString();
    std::string password = ui->passwordField->text().toStdString();

    if(auto err = Validator::username(login)) {
        showError(QString::fromStdString(*err));
        return;
    }

    if(auto err = Validator::password(password)) {
        showError(QString::fromStdString(*err));
        return;
    }

    emit loginRequest(login, password);
}

void LoginWindow::tryRegister() {
    std::string login = ui->loginRegField->text().toStdString();
    std::string password = ui->passwordRegField->text().toStdString();

    if (password != ui->passwordRepeateField->text().toStdString()) {
        showError(QString::fromStdString("Passwords not equals\n"));
        return;
    }

    if(auto err = Validator::username(login)) {
        showError(QString::fromStdString(*err));
        return;
    }

    if(auto err = Validator::password(password)) {
        showError(QString::fromStdString(*err));
        return;
    }

    emit registerRequest(login, password);
}

void LoginWindow::validateLoginForm() {
    bool loginOk = Validator::username(ui->loginField->text().toStdString()) == std::nullopt;
    bool passwordOk = Validator::password(ui->passwordField->text().toStdString()) == std::nullopt;

    updateFieldStyle(ui->loginField, loginOk, ui->loginField->text().isEmpty());
    updateFieldStyle(ui->passwordField, passwordOk, ui->passwordField->text().isEmpty());
}

void LoginWindow::validateRegisterForm() {
    bool loginOk = Validator::username(ui->loginRegField->text().toStdString()) == std::nullopt;
    bool password1Ok = Validator::password(ui->passwordRegField->text().toStdString()) == std::nullopt;
    bool password2Ok = Validator::password(ui->passwordRepeateField->text().toStdString()) == std::nullopt;
    bool equal = ui->passwordRegField->text() == ui->passwordRepeateField->text();

    updateFieldStyle(ui->loginRegField, loginOk, ui->loginRegField->text().isEmpty());
    updateFieldStyle(ui->passwordRegField, password1Ok, ui->passwordRegField->text().isEmpty());
    updateFieldStyle(ui->passwordRepeateField, password1Ok && equal, ui->passwordRepeateField->text().isEmpty());
}

void LoginWindow::updateFieldStyle(QLineEdit* field, bool isValid, bool isEmpty) {
    if(isEmpty) {
        field->setStyleSheet("");
    } else if (isValid) {
        field->setStyleSheet("QLineEdit { border: 2px solid #4caf50; }");
    } else {
        field->setStyleSheet("QLineEdit { border: 2px solid #f44336; }");
    }
}

void LoginWindow::showError(const QString& error) {

}
