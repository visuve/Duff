#include "hashcalculator.h"
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QMap>
#include <QStringList>

HashCalculator::HashCalculator(QObject* parent, const QString &directory, QCryptographicHash::Algorithm algorithm) :
    QThread(parent),
    _directory(directory),
    _algorithm(algorithm)
{
    connect(this, &QThread::terminate, []()
    {
        qDebug() << "Terminate requested";
    });

    connect(this, &QThread::quit, []()
    {
        qDebug() << "Quit requested";
    });
}

HashCalculator::~HashCalculator()
{
    qDebug() << "Destroying...";
    requestInterruption();
    wait();
    qDebug() << "Destroyed.";
}

void HashCalculator::run()
{
    QDirIterator it(_directory, QDir::Files, QDirIterator::Subdirectories);
    QMap<QString, QStringList> fileHashes;

    while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext())
    {
        const QString path = it.next();
        QFile file(path);

        if (!file.open(QFile::ReadOnly))
        {
            qWarning() << "Failed to open" << path;
            continue;
        }

        emit processing(path);

        QCryptographicHash hash(_algorithm);

        if (!hash.addData(&file))
        {
            qWarning() << "Failed to process: " << path;
            continue;
        }

        const QString hashString = hash.result().toHex();
        fileHashes[hashString].append(path);
        int size = fileHashes[hashString].size();

        if (size == 2)
        {
            emit duplicateFound(hashString, fileHashes[hashString].first());
            emit duplicateFound(hashString, path);
        }

        if (size > 2)
        {
            emit duplicateFound(hashString, path);
        }
    }
}
