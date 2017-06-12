#include "logger.h"
#include <QDebug>

// Типы сообщений
static const char* msgType[] =
{
    "(II) ", // Info
    "(WW) ", // Warning
    "(EE) ", // Error
    "(FF) " // Fatal error
};

// Данные для ведения логов
static QTextStream* logStream;
static QFile* logFile;
// Вывод логов в файл
void customMessageHandler(QtMsgType type, const char* msg);

Logger::Logger()
{

}

Logger::~Logger()
{

}


void customMessageHandler(QtMsgType type, const char* msg)
{
    std::cout << msgType[type] << msg << std::endl;
    if(logStream && logStream->device())
    {
        *logStream << msgType[type] << QDateTime::currentDateTime().toString("hh:mm:ss.zzz ") << QString::fromLocal8Bit(msg) << endl;
    }
}

void Logger::installLog()
{
    logFile = new QFile("stats.log");
    if(logFile->open(QIODevice::WriteOnly | QIODevice::Unbuffered))
    logStream = new QTextStream(logFile);

//    #ifdef Q_WS_WIN
//    logStream->setCodec("Windows-1251");
    logStream->setCodec("utf-8");
    // Под остальными ОС - utf8
//    #else
//    logStream->setCodec("utf-8");
//    #endif

    // Запись заголовка с информацией о запуске
    if(logStream && logStream->device())
    {
        *logStream << endl << TextDescription << endl;
        *logStream << QString("Markers: (II) informational, (WW) warning,") << endl;
        *logStream << QString("(EE) error, (FF) fatal error.") << endl;
        *logStream << getOSInfo() << endl;
        *logStream << QString("Runned at %1.").arg(QDateTime::currentDateTime().toString()) << endl << endl;
    }

    qInstallMsgHandler(customMessageHandler);

    qDebug("Success opening log file");
}

void Logger::finishLog()
{
    qDebug("Success closing log file");

    delete logStream;
    logStream = 0;
    delete logFile;
    logFile = 0;

    qInstallMsgHandler(0);
}

QString Logger::getOSInfo()
{
    QString infoStr("Current Operating System: %1");
    #ifdef Q_OS_WIN
    switch(QSysInfo::windowsVersion())
    {
        case QSysInfo::WV_NT: return infoStr.arg("Windows NT");
        case QSysInfo::WV_2000: return infoStr.arg("Windows 2000");
        case QSysInfo::WV_XP: return infoStr.arg("Windows XP");
        case QSysInfo::WV_2003: return infoStr.arg("Windows 2003");
        case QSysInfo::WV_VISTA: return infoStr.arg("Windows Vista");
        case QSysInfo::WV_WINDOWS7: return infoStr.arg("Windows Seven");
        case QSysInfo::WV_WINDOWS8: return infoStr.arg("Windows 8");
        case QSysInfo::WV_WINDOWS8_1 : return infoStr.arg("Windows 8.1");
        default: return infoStr.arg("Windows");
    }
    #endif // Q_OS_WIN

    #ifdef Q_OS_MAC
    switch(QSysInfo::MacintoshVersion())
    {
        case QSysInfo::MV_CHEETAH: return infoStr.arg("Mac OS X 1.0 Cheetah");
        case QSysInfo::MV_PUMA: return infoStr.arg("Mac OS X 1.1 Puma");
        case QSysInfo::MV_JAGUAR: return infoStr.arg("Mac OS X 1.2 Jaguar");
        case QSysInfo::MV_PANTHER: return infoStr.arg("Mac OS X 1.3 Panther");
        case QSysInfo::MV_TIGER: return infoStr.arg("Mac OS X 1.4 Tiger");
        case QSysInfo::MV_LEOPARD: return infoStr.arg("Mac OS X 1.5 Leopard");
        case QSysInfo::MV_SNOWLEOPARD: return infoStr.arg("Mac OS X 1.6 Snow Leopard");
        default: return infoStr.arg("Mac OS X");
    }
    #endif // Q_OS_MAC

    #ifdef Q_OS_LINUX
    #ifdef LINUX_OS_VERSION
    return infoStr.arg(LINUX_OS_VERSION);
    #else
    return infoStr.arg("Linux");
    #endif // LINUX_OS_VERSION
    #endif // Q_OS_LINUX
}
