#include "HashCalculator.hpp"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QMap>
#include <QStringList>
#include <array>

HashCalculator::HashCalculator(QObject* parent) :
	QThread(parent)
{
	qDebug();
}

HashCalculator::~HashCalculator()
{
	qDebug() << "Destroying...";
	requestInterruption();
	wait();
	qDebug() << "Destroyed.";
}

void HashCalculator::setDirectory(const QString& directory)
{
	_directory = directory;
}

void HashCalculator::setAlgorithm(QCryptographicHash::Algorithm algorithm)
{
	_algorithm = algorithm;
}

QByteArray HashCalculator::calculateHash(const QString& filePath)
{
	QFile file(filePath);

	if (!file.open(QFile::ReadOnly))
	{
		emit failure(filePath);
		return {};
	}

	QCryptographicHash hash(_algorithm);
	constexpr int BufferSize = 0x10000; // 64K
	thread_local std::array<char, BufferSize> buffer = {};
	qint64 bytesReadTotal = 0;
	const qint64 bytesLeft = file.size();

	if (bytesLeft <= 0)
	{
		emit failure(filePath);
		return {};
	}

	emit processing(filePath, bytesReadTotal, bytesLeft);

	do
	{
		if (QThread::currentThread()->isInterruptionRequested())
		{
			return {};
		}

		qint64 bytesRead = file.read(buffer.data(), BufferSize);

		if (bytesRead < 0)
		{
			emit failure(filePath);
			return {};
		}

		bytesReadTotal += bytesRead;
		hash.addData(buffer.data(), bytesRead);
		emit processing(filePath, bytesReadTotal, bytesLeft);
	}
	while (bytesReadTotal < bytesLeft);

	return hash.result().toHex();
}

void HashCalculator::run()
{
	QDirIterator it(_directory, QDir::Files, QDirIterator::Subdirectories);
	QMap<QString, QStringList> fileHashes;

	while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext())
	{
		const QString path = QDir::toNativeSeparators(it.next());
		const QByteArray fileHash = calculateHash(path);

		if (fileHash.isEmpty())
		{
			continue;
		}

		fileHashes[fileHash] << path;
		int size = fileHashes[fileHash].size();

		if (size == 2)
		{
			emit duplicateFound(fileHash, fileHashes[fileHash].first());
			emit duplicateFound(fileHash, path);
		}

		if (size > 2)
		{
			emit duplicateFound(fileHash, path);
		}
	}
}
