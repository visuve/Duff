#include "mainwindow.h"

#include <QApplication>
#include <QTime>

#include <iostream>

char qtMsgTypeToChar(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
        return 'D';
    case QtInfoMsg:
        return 'I';
    case QtWarningMsg:
        return 'W';
    case QtCriticalMsg:
        return 'C';
    case QtFatalMsg:
        return 'F';
    }

    return '?';
}

std::ostream& qtMsgTypeToStreamType(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
    case QtInfoMsg:
        return std::cout;
    case QtWarningMsg:
        return std::clog;
    case QtCriticalMsg:
    case QtFatalMsg:
        return std::cerr;
    }

    return std::cerr;
}

void duffMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    const QString time = QTime::currentTime().toString();

    if (context.function && !message.isEmpty())
    {
        qtMsgTypeToStreamType(type)
                << time.toStdString() << " ["
                << qtMsgTypeToChar(type) << "] "
                << context.function << ':'
                << context.line << ": "
                << message.toStdString() << std::endl;
    }

    if (context.function && message.isEmpty())
    {
        qtMsgTypeToStreamType(type)
                << time.toStdString() << " ["
                << qtMsgTypeToChar(type) << "] "
                << context.function << ':'
                << context.line << std::endl;
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(duffMessageHandler);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
