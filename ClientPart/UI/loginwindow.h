#pragma once

#include <QDialog>
#include <optional>
#include <QLineEdit>

#include "MessageRouter.h"
#include "Validator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LoginWindow;
}
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent_, MessageRouter* router_);
    ~LoginWindow();

    void loginSuccess();
    void loginFailed(const std::string& reason);
    void registerSuccess();
    void registerFailed(const std::string& reason);

signals:
    void loginRequest(const std::string& login, const std::string& password);
    void registerRequest(const std::string& login, const std::string& password);

private:
    Ui::LoginWindow *ui;

    MessageRouter* router;

    void tryLogin();
    void tryRegister();
    void showError(const QString& msg);

    void validateLoginForm();
    void validateRegisterForm();
    void updateFieldStyle(QLineEdit* field, bool isValid, bool isEmpty);
};
