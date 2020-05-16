#include "logger.h"
#include <QDebug>
#include <QMutex>

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

QMutex	m_mutex;
// Вывод логов в файл
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

Logger::Logger()
{
}

Logger::~Logger()
{
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    m_mutex.lock();
    std::cout << msgType[type] << msg.toStdString() << std::endl;
    if(logStream && logStream->device())
    {
        *logStream << msgType[type] << QDateTime::currentDateTime().toString("hh:mm:ss.zzz ") << msg << endl;
    }
    m_mutex.unlock();
}
void Logger::installLog()
{
    logFile = new QFile("stats_dxhook.log");
    if(logFile->open(QIODevice::WriteOnly | QIODevice::Unbuffered))

    logStream = new QTextStream(logFile);
    logStream->setCodec("utf-8");

    // Запись заголовка с информацией о запуске
    if(logStream && logStream->device())
    {
        *logStream << endl << TextDescription << endl;
        *logStream << QString("Markers: (II) informational, (WW) warning,") << endl;
        *logStream << QString("(EE) error, (FF) fatal error.") << endl;
        *logStream << getOSInfo() << endl;
        *logStream << QString("Runned at %1.").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) << endl << endl;
    }

    qInstallMessageHandler(customMessageHandler);

    qDebug("Success opening log file");
}

void Logger::clearLog()
{
    logFile->resize(log_size);
    logFile->seek(log_size);
}

void Logger::updateSize(){
    log_size = logFile->size();
    qDebug() << "log size:" << log_size;
}

void Logger::finishLog()
{
    qDebug("Success closing log file");

    delete logStream;
    logStream = 0;
    delete logFile;
    logFile = 0;

    qInstallMessageHandler(0);
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
        case QSysInfo::WV_WINDOWS7: return infoStr.arg("Windows 7");
        case QSysInfo::WV_WINDOWS8: return infoStr.arg("Windows 8");
        case QSysInfo::WV_WINDOWS8_1 : return infoStr.arg("Windows 8.1");
        case QSysInfo::WV_WINDOWS10 : return infoStr.arg("Windows 10");
        default: return infoStr.arg("Windows");
    }
    #endif // Q_OS_WIN
}
