#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTcp)      //TCP-соединение, сокеты
Q_DECLARE_LOGGING_CATEGORY(logRouter)   //Формирование и отправка пакетов
Q_DECLARE_LOGGING_CATEGORY(logPacket)   //Детали парсинга пакетов

Q_DECLARE_LOGGING_CATEGORY(logHandler)  //Обработка входящих сообщений
Q_DECLARE_LOGGING_CATEGORY(logState)    //AppState (токены, пользователи)
Q_DECLARE_LOGGING_CATEGORY(logAuth)     //Логин, регистрация, сессии
Q_DECLARE_LOGGING_CATEGORY(logDialog)   //Диалоги, история, поиск
Q_DECLARE_LOGGING_CATEGORY(logFiles)    //Передача файлов, локальный кэш

Q_DECLARE_LOGGING_CATEGORY(logUi)       //События интерфейса
Q_DECLARE_LOGGING_CATEGORY(logMain)     //События запуска