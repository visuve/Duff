#pragma once

#include <QObject>
#include <QThread>
#include <QCryptographicHash>

class HashCalculator : public QThread
{
    Q_OBJECT

public:
    HashCalculator(QObject* parent, const QString& directory, QCryptographicHash::Algorithm algorithm);
    ~HashCalculator();

signals:
    void processing(const QString& filePath);
    void processed(const QString& hashString, const QString& filePath);

private:
    void run() override;

    const QString _directory;
    const QCryptographicHash::Algorithm _algorithm;
};
