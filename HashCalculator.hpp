#pragma once

#include <QObject>
#include <QThread>
#include <QCryptographicHash>

class HashCalculator : public QThread
{
	Q_OBJECT

public:
	enum class ErrorType : char
	{
		Open = 'o',
		Empty = 'e',
		Read = 'r'
	};

	HashCalculator(QObject* parent);
	~HashCalculator();

	void setDirectory(const QString& directory);
	void setAlgorithm(QCryptographicHash::Algorithm algorithm);

signals:
	void processing(const QString& filePath, qint64 bytesRead, qint64 bytesLeft);
	void duplicateFound(const QString& hashString, const QString& filePath);
	void failure(const QString& filePath, ErrorType error);

private:
	bool keepRunning() const;
	QByteArray calculateHash(const QString& filePath);
	void run() override;

	QString _directory;
	QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
};

Q_DECLARE_METATYPE(HashCalculator::ErrorType)
