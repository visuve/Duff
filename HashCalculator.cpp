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
		qWarning() << "Failed to open" << filePath;
		return {};
	}

	QCryptographicHash hash(_algorithm);
	constexpr int BufferSize = 0x10000; // 64K
	thread_local std::array<char, BufferSize> buffer = {};
	qint64 bytesRead = 0;

	emit processing(filePath);

	do
	{
		if (QThread::currentThread()->isInterruptionRequested())
		{
			return {};
		}

		bytesRead = file.read(buffer.data(), BufferSize);

		if (bytesRead < 0)
		{
			qWarning() << "Failed to process: " << filePath;
			return {};
		}

		hash.addData(buffer.data(), bytesRead);
	}
	while (bytesRead > 0);

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
