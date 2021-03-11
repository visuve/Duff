#include "HashCalculator.hpp"
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QMap>
#include <QStringList>

HashCalculator::HashCalculator(QObject* parent, const QString& directory, QCryptographicHash::Algorithm algorithm) :
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

QByteArray HashCalculator::calculateHash(const QString& filePath)
{
	QFile file(filePath);

	if (!file.open(QFile::ReadOnly))
	{
		qWarning() << "Failed to open" << filePath;
		return {};
	}

	emit processing(filePath);

	QCryptographicHash hash(_algorithm);

	if (!hash.addData(&file))
	{
		qWarning() << "Failed to process: " << filePath;
		return {};
	}

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

		fileHashes[fileHash] << (path);
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
