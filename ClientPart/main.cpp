#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QMetaObject>
#include <memory>

#include "Router.h"
#include "TcpClient.h"
#include "Handler.h"
#include "Appcontroller.h"
#include "DialogManager.h"
#include "LoginViewModel.h"
#include "SearchViewModel.h"
#include "Logging.h"

void colorfulMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    const char* reset   = "\033[0m";
    const char* red     = "\033[31m";
    const char* yellow  = "\033[33m";
    const char* green   = "\033[32m";
    const char* cyan    = "\033[36m";
    const char* gray    = "\033[90m";
    const char* bold    = "\033[1m";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    const char* levelColor = reset;
    const char* levelText = "";
    bool isBold = false;

    switch (type) {
    case QtDebugMsg:
        levelColor = cyan;
        levelText = "DEBUG  ";
        break;
    case QtInfoMsg:
        levelColor = green;
        levelText = "INFO   ";
        break;
    case QtWarningMsg:
        levelColor = yellow;
        levelText = "WARNING";
        break;
    case QtCriticalMsg:
        levelColor = red;
        levelText = "CRITICL";
        isBold = true;
        break;
    case QtFatalMsg:
        levelColor = red;
        levelText = "FATAL  ";
        isBold = true;
        break;
    }

    QString category = context.category ? context.category : "default";

    std::cerr << gray << "[" << timestamp.toStdString() << "] " << reset
              << levelColor << (isBold ? bold : "") << levelText << reset << " "
              << gray << "[" << category.toStdString() << "] " << reset
              << msg.toStdString() << std::endl;

    if (type == QtFatalMsg) {
        abort();
    }
}

#ifdef Q_OS_ANDROID
void startAndroidBackgroundService() {
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        qWarning() << "[Android] Не удалось получить контекст";
        return;
    }

    QJniObject classLoader = context.callObjectMethod(
        "getClassLoader", "()Ljava/lang/ClassLoader;"
        );

    QJniObject serviceName = QJniObject::fromString(
        "org.qtproject.example.BackgroundService"
        );

    QJniObject serviceClass = classLoader.callObjectMethod(
        "loadClass",
        "(Ljava/lang/String;)Ljava/lang/Class;",
        serviceName.object<jstring>()
        );

    if (!serviceClass.isValid()) {
        qWarning() << "[Android] Класс BackgroundService не найден через ClassLoader!";
        return;
    }

    QJniObject intent("android/content/Intent",
                      "(Landroid/content/Context;Ljava/lang/Class;)V",
                      context.object<jobject>(),
                      serviceClass.object<jobject>());

    if (!intent.isValid()) {
        qWarning() << "[Android] Не удалось создать Intent";
        return;
    }

    context.callObjectMethod("startForegroundService",
                             "(Landroid/content/Intent;)Landroid/content/ComponentName;",
                             intent.object<jobject>());

    qInfo() << "[Android] Фоновый сервис успешно запущен";
}
#else
void startAndroidBackgroundService() {}
#endif


int main(int argc, char *argv[]) {
    qInstallMessageHandler(colorfulMessageHandler);

    QLoggingCategory::setFilterRules(
        "app.*.debug=true\n"
        "app.*.info=true\n"
        "app.*.warning=true\n"
        "app.*.critical=true\n"
        );

    qCInfo(logMain) << "Запуск приложения";

    QGuiApplication app(argc, argv);

    qCInfo(logMain) << "Создание компонентов приложения";
    auto state = std::make_unique<AppState>();
    auto router = std::make_unique<Router>();
    auto handler = std::make_unique<Handler>();
    auto client = std::make_unique<TCPClient>(state->getServerHost(), state->getServerPort(), router.get());
    auto appController = std::make_unique<AppController>(router.get(), state.get(), handler.get(), client.get());
    auto dialogManager = std::make_unique<DialogManager>(handler.get(), state.get());

    qCInfo(logMain) << "Настройка обработчика входящих сообщений";
    client->onMessage = [h = handler.get()](const std::string& msg) {
        QMetaObject::invokeMethod(h, [h, msg]() {
            h->handleMessage(msg);
        }, Qt::QueuedConnection);
    };

    qCInfo(logMain) << "Инициализация сетевого клиента";
    client->start();

    qCInfo(logMain) << "Создание ViewModel компонентов";
    auto loginVM = std::make_unique<LoginViewModel>(router.get(), handler.get(), appController.get(), &app);
    auto searchVM = std::make_unique<SearchViewModel>(router.get());
    QObject::connect(handler.get(), &Handler::S_UserSearch, searchVM.get(), &SearchViewModel::onUserSearchResults);

    qCInfo(logMain) << "Запуск фонового сервиса (Android)";
    startAndroidBackgroundService();

    qCInfo(logMain) << "Инициализация QML движка";
    QQmlApplicationEngine engine;

    qCInfo(logMain) << "Регистрация объектов в QML контексте";
    engine.rootContext()->setContextProperty("appController", appController.get());
    engine.rootContext()->setContextProperty("loginViewModel", loginVM.get());
    engine.rootContext()->setContextProperty("searchViewModel", searchVM.get());
    engine.rootContext()->setContextProperty("appState", state.get());

    qCInfo(logMain) << "Загрузка главного QML файла";
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCWarning(logMain) << "Не удалось загрузить QML интерфейс";
        return -1;
    }

    qCInfo(logMain) << "QML интерфейс успешно загружен";

    qCInfo(logMain) << "Запуск менеджера диалогов";
    dialogManager->start();

    qCInfo(logMain) << "Приложение готово к работе";

    int result = app.exec();

    qCInfo(logMain) << "Завершение работы приложения";

    return result;
}