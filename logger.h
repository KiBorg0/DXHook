#ifndef LOGGER_H
#define LOGGER_H

#include "version.h"
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

    const QString TextDescription = QObject::tr(
    "%1 %2\n"
    "Built on " __DATE__ " at " __TIME__ ".\n"
    "Based on Qt %3.\n"
    "Copyright %4. All rights reserved.\n"
    "See also %5\n")
    .arg(QLatin1String(VER_PRODUCTNAME_STR), QLatin1String(VER_FILEVERSION_STR),
    QLatin1String(QT_VERSION_STR), QLatin1String(VER_COMPANYNAME_STR), QLatin1String(VER_COMPANYDOMAIN_STR)
    );

    // Вывод логов в файл
//    void customMessageHandler(QtMsgType type, const char* msg);
    // Создание файла для логов
    void installLog();
    // очистка файла лога
    void clearLog();
    // Закрытие файла логов
    void finishLog();
    // Информация об ОС
    QString getOSInfo();
    void updateSize();

private:
    int log_size;
};

#endif // LOGGER_H
