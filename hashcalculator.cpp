#include "hashcalculator.h"
#include <QDir>
#include <QDirIterator>
#include <QTime>
#include <QDebug>
#include <QCryptographicHash>

HashCalculator::HashCalculator(QObject* parent, const QString &directory, QCryptographicHash::Algorithm algorithm) :
    QThread(parent),
    _directory(directory),
    _algorithm(algorithm)
{
    connect(this, &QThread::terminate, [this]()
    {
        _keepRunning = false;
    });

    qRegisterMetaType<HashToFilePaths>("LOL");
}

HashCalculator::~HashCalculator()
{
    _keepRunning = false;
    this->wait();
}

void HashCalculator::run()
{
    QDirIterator it(_directory, QDir::Files, QDirIterator::Subdirectories);
    HashToFilePaths fileHashes;

    while (it.hasNext() && _keepRunning)
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

        if (hash.addData(&file))
        {
            const QString hashString = hash.result().toHex();
            fileHashes[hashString].append(path);
            emit processed(path, hashString);
        }
    }

    emit completed(fileHashes);
}
