#ifndef LOGGER_H
#define LOGGER_H


#define APP_NAME "Soulstorm Stats Reader"
#define APP_VERSION "1.0"
#define ORG_NAME "New"
#define ORG_DOMAIN "loa92@mail.ru"


#include <QTextCodec>
#include <QDateTime>
#include <windows.h>

#include <iostream>
#include <QString>
#include <QFile>
#include <QTextStream>

class Logger
{
public:
    Logger();
    ~Logger();

    //#define __DATE__
    //#define __TIME__

    const QString TextDescription = QObject::tr(
    "%1 %2\n"
    "Built on " __DATE__ " at " __TIME__ ".\n"
    "Based on Qt %3.\n"
    "Copyright %4. All rights reserved.\n"
    "See also %5\n")
    .arg(QLatin1String(APP_NAME), QLatin1String(APP_VERSION),
    QLatin1String(QT_VERSION_STR), QLatin1String(ORG_NAME), QLatin1String(ORG_DOMAIN)
    );

//    // Вывод логов в файл
//    void customMessageHandler(QtMsgType type, const char* msg);
    // Создание файла для логов
    void installLog();
    // Закрытие файла логов
    void finishLog();
    // Информация об ОС
    QString getOSInfo();
};

#endif // LOGGER_H
