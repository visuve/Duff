#pragma once

#include <QObject>
#include <QThread>
#include <QCryptographicHash>
#include <QMap>
#include <QStringList>
#include <QMetaType>
#include <atomic>

using HashToFilePaths = std::map<QString, QStringList>;
Q_DECLARE_METATYPE(HashToFilePaths);

class HashCalculator : public QThread
{
    Q_OBJECT

public:
    HashCalculator(QObject* parent, const QString& directory, QCryptographicHash::Algorithm algorithm);
    ~HashCalculator();

signals:
    void processing(const QString& filePath);
    void processed(const QString& filePath, const QString& hashString);
    void completed(const HashToFilePaths& fileHashes);

private:
    void run() override;

    const QString _directory;
    const QCryptographicHash::Algorithm _algorithm;
    std::atomic<bool> _keepRunning = true;
};
