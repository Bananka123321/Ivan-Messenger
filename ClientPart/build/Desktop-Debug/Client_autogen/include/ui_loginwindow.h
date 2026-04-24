/********************************************************************************
** Form generated from reading UI file 'loginwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINWINDOW_H
#define UI_LOGINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoginWindow
{
public:
    QGridLayout *gridLayout;
    QStackedWidget *stackedWidget;
    QWidget *Registr;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer_3;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QLabel *label_2;
    QLineEdit *loginRegField;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QLineEdit *passwordRegField;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_4;
    QLineEdit *passwordRepeateField;
    QPushButton *okButton_2;
    QLabel *signInLink;
    QSpacerItem *verticalSpacer_4;
    QWidget *Login;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_5;
    QLineEdit *loginField;
    QLineEdit *passwordField;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *rememberBox;
    QLabel *label_6;
    QPushButton *okButton;
    QLabel *registrLink;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QDialog *LoginWindow)
    {
        if (LoginWindow->objectName().isEmpty())
            LoginWindow->setObjectName("LoginWindow");
        LoginWindow->resize(1101, 620);
        LoginWindow->setMinimumSize(QSize(500, 250));
        gridLayout = new QGridLayout(LoginWindow);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setHorizontalSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(LoginWindow);
        stackedWidget->setObjectName("stackedWidget");
        Registr = new QWidget();
        Registr->setObjectName("Registr");
        Registr->setMaximumSize(QSize(400, 16777215));
        verticalLayout_3 = new QVBoxLayout(Registr);
        verticalLayout_3->setSpacing(20);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_3);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(20);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_2 = new QLabel(Registr);
        label_2->setObjectName("label_2");
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(label_2);

        loginRegField = new QLineEdit(Registr);
        loginRegField->setObjectName("loginRegField");
        loginRegField->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(loginRegField);

        horizontalLayout->setStretch(0, 1);
        horizontalLayout->setStretch(1, 2);

        verticalLayout_2->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_3 = new QLabel(Registr);
        label_3->setObjectName("label_3");
        sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(label_3);

        passwordRegField = new QLineEdit(Registr);
        passwordRegField->setObjectName("passwordRegField");
        passwordRegField->setFrame(true);
        passwordRegField->setEchoMode(QLineEdit::Password);
        passwordRegField->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(passwordRegField);

        horizontalLayout_2->setStretch(0, 1);
        horizontalLayout_2->setStretch(1, 2);

        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_4 = new QLabel(Registr);
        label_4->setObjectName("label_4");
        sizePolicy.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy);
        label_4->setWordWrap(true);

        horizontalLayout_3->addWidget(label_4);

        passwordRepeateField = new QLineEdit(Registr);
        passwordRepeateField->setObjectName("passwordRepeateField");
        passwordRepeateField->setEchoMode(QLineEdit::Password);
        passwordRepeateField->setAlignment(Qt::AlignCenter);

        horizontalLayout_3->addWidget(passwordRepeateField);

        horizontalLayout_3->setStretch(0, 1);
        horizontalLayout_3->setStretch(1, 2);

        verticalLayout_2->addLayout(horizontalLayout_3);


        verticalLayout_3->addLayout(verticalLayout_2);

        okButton_2 = new QPushButton(Registr);
        okButton_2->setObjectName("okButton_2");

        verticalLayout_3->addWidget(okButton_2);

        signInLink = new QLabel(Registr);
        signInLink->setObjectName("signInLink");
        signInLink->setCursor(QCursor(Qt::PointingHandCursor));
        signInLink->setTextFormat(Qt::RichText);
        signInLink->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);
        signInLink->setTextInteractionFlags(Qt::TextBrowserInteraction);

        verticalLayout_3->addWidget(signInLink);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_4);

        verticalLayout_3->setStretch(0, 2);
        verticalLayout_3->setStretch(1, 1);
        verticalLayout_3->setStretch(2, 1);
        verticalLayout_3->setStretch(3, 1);
        verticalLayout_3->setStretch(4, 1);
        stackedWidget->addWidget(Registr);
        Login = new QWidget();
        Login->setObjectName("Login");
        Login->setMaximumSize(QSize(400, 16777215));
        verticalLayout = new QVBoxLayout(Login);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(20);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_5->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalLayout_5->setContentsMargins(20, 0, 20, 0);
        loginField = new QLineEdit(Login);
        loginField->setObjectName("loginField");
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(loginField->sizePolicy().hasHeightForWidth());
        loginField->setSizePolicy(sizePolicy1);
        loginField->setInputMethodHints(Qt::ImhHiddenText);
        loginField->setAlignment(Qt::AlignCenter);

        verticalLayout_5->addWidget(loginField);

        passwordField = new QLineEdit(Login);
        passwordField->setObjectName("passwordField");
        passwordField->setEchoMode(QLineEdit::Password);
        passwordField->setAlignment(Qt::AlignCenter);

        verticalLayout_5->addWidget(passwordField);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_4->setContentsMargins(-1, -1, 0, -1);
        rememberBox = new QCheckBox(Login);
        rememberBox->setObjectName("rememberBox");

        horizontalLayout_4->addWidget(rememberBox);

        label_6 = new QLabel(Login);
        label_6->setObjectName("label_6");
        label_6->setCursor(QCursor(Qt::PointingHandCursor));
        label_6->setTextFormat(Qt::RichText);
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        label_6->setTextInteractionFlags(Qt::TextBrowserInteraction);

        horizontalLayout_4->addWidget(label_6);


        verticalLayout_5->addLayout(horizontalLayout_4);

        okButton = new QPushButton(Login);
        okButton->setObjectName("okButton");
        okButton->setFlat(false);

        verticalLayout_5->addWidget(okButton);

        registrLink = new QLabel(Login);
        registrLink->setObjectName("registrLink");
        QFont font;
        font.setPointSize(12);
        font.setUnderline(false);
        font.setStrikeOut(false);
        font.setKerning(true);
        registrLink->setFont(font);
        registrLink->setCursor(QCursor(Qt::PointingHandCursor));
        registrLink->setTextFormat(Qt::RichText);
        registrLink->setScaledContents(false);
        registrLink->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);
        registrLink->setTextInteractionFlags(Qt::TextBrowserInteraction);

        verticalLayout_5->addWidget(registrLink);


        verticalLayout->addLayout(verticalLayout_5);

        verticalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        stackedWidget->addWidget(Login);

        gridLayout->addWidget(stackedWidget, 0, 1, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 0, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 0, 2, 1, 1);


        retranslateUi(LoginWindow);

        stackedWidget->setCurrentIndex(1);
        okButton->setDefault(true);


        QMetaObject::connectSlotsByName(LoginWindow);
    } // setupUi

    void retranslateUi(QDialog *LoginWindow)
    {
        LoginWindow->setWindowTitle(QCoreApplication::translate("LoginWindow", "Dialog", nullptr));
        label_2->setText(QCoreApplication::translate("LoginWindow", "Login", nullptr));
        loginRegField->setPlaceholderText(QCoreApplication::translate("LoginWindow", "Login", nullptr));
        label_3->setText(QCoreApplication::translate("LoginWindow", "Password", nullptr));
        passwordRegField->setPlaceholderText(QCoreApplication::translate("LoginWindow", "Password", nullptr));
        label_4->setText(QCoreApplication::translate("LoginWindow", "Re-type Password", nullptr));
        passwordRepeateField->setPlaceholderText(QCoreApplication::translate("LoginWindow", "Re-type Password", nullptr));
        okButton_2->setText(QCoreApplication::translate("LoginWindow", "Accept", nullptr));
        signInLink->setText(QCoreApplication::translate("LoginWindow", "<html><head/><body><p>Already have an account? <a href=\"login\"><span style=\" text-decoration: underline; color:#007af4;\">Sign in</span></a></p></body></html>", nullptr));
        loginField->setPlaceholderText(QCoreApplication::translate("LoginWindow", "Login", nullptr));
        passwordField->setPlaceholderText(QCoreApplication::translate("LoginWindow", "Password", nullptr));
        rememberBox->setText(QCoreApplication::translate("LoginWindow", "Remember me", nullptr));
        label_6->setText(QCoreApplication::translate("LoginWindow", "Forgot password?", nullptr));
        okButton->setText(QCoreApplication::translate("LoginWindow", "Accept", nullptr));
        registrLink->setText(QCoreApplication::translate("LoginWindow", "<html><head/><body><p>Don't have account? <a href=\"registr\"><span style=\" text-decoration: underline; color:#007af4;\">Register</span></a></p></body></html>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoginWindow: public Ui_LoginWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINWINDOW_H
