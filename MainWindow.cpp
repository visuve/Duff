#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "HashCalculator.hpp"
#include "ResultModel.hpp"

#include <QActionGroup>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QTreeWidgetItem>

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow()),
	_model(new ResultModel(this))
{
	ui->setupUi(this);

	ui->actionOpen->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenDirectoryDialog);

	ui->actionExit->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);

	ui->actionAbout->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

	ui->actionLicenses->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarMenuButton));
	connect(ui->actionLicenses, &QAction::triggered, [this]()
	{
		QMessageBox::aboutQt(this, "Duff");
	});

	ui->treeViewResults->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->treeViewResults, &QTreeWidget::customContextMenuRequested, this, &MainWindow::createFileContextMenu);

	auto algorithmGroup = new QActionGroup(this);
	algorithmGroup->addAction(ui->actionMD5);
	algorithmGroup->addAction(ui->actionSHA_1);
	algorithmGroup->addAction(ui->actionSHA_256);
	algorithmGroup->addAction(ui->actionSHA_512);
	algorithmGroup->setExclusive(true);
	ui->actionSHA_256->setChecked(true);

	connect(ui->actionMD5, &QAction::triggered, [this]() { _algorithm = QCryptographicHash::Algorithm::Md5; });
	connect(ui->actionSHA_1, &QAction::triggered, [this]() { _algorithm = QCryptographicHash::Algorithm::Sha1; });
	connect(ui->actionSHA_256, &QAction::triggered, [this]() { _algorithm = QCryptographicHash::Algorithm::Sha256; });
	connect(ui->actionSHA_512, &QAction::triggered, [this]() { _algorithm = QCryptographicHash::Algorithm::Sha512; });

	connect(ui->pushButtonFindDuplicates, &QPushButton::clicked, this, &MainWindow::onFindDuplicates);
	connect(ui->pushButtonDeleteSelected, &QPushButton::clicked, this, &MainWindow::deleteSelected);

	ui->treeViewResults->setModel(_model);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onOpenDirectoryDialog()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, true);

	if (dialog.exec() == QFileDialog::Accepted)
	{
		_model->clear();
		const QString directory = dialog.selectedFiles().first();
		ui->lineEditSelectedDirectory->setText(QDir::toNativeSeparators(directory));
	}
}

void MainWindow::onFindDuplicates()
{
	const QString selectedDirectory = ui->lineEditSelectedDirectory->text();

	if (selectedDirectory.isEmpty())
	{
		onOpenDirectoryDialog();
		onFindDuplicates();
	}

	if (!QDir().exists(selectedDirectory))
	{
		QMessageBox::warning(this, "Selected directory", '"' + selectedDirectory + '"' + " does not appear to exist!");
		onOpenDirectoryDialog();
		return;
	}

	populateTree(ui->lineEditSelectedDirectory->text());
}

void MainWindow::onDuplicateFound(const QString& hashString, const QString& filePath)
{
	qDebug() << hashString << filePath;
	ui->statusBar->showMessage(QTime::currentTime().toString() + " Found duplicate: " + filePath + " -> " + hashString);
	_model->addPath(hashString, filePath);
}

void MainWindow::onFinished()
{
	if (_model->rowCount() <= 0)
	{
		QMessageBox::information(
			this,
			"No duplicate files",
			"No duplicate files were found.\n");
	}

	ui->statusBar->showMessage(QTime::currentTime().toString() + " Finished processing.\n ");
	ui->menuAlgorithm->setEnabled(true);
}

void MainWindow::deleteSelected()
{
	QStringList filePaths = _model->selectedPaths();

	if (filePaths.empty() ||
		QMessageBox::question(
			this,
			"Confirm delete?",
			"Are you sure you want to delete the following files:\n" + filePaths.join('\n')) !=
		QMessageBox::StandardButton::Yes)
	{
		return;
	}

	for (const QString& filePath : filePaths)
	{
		if (!QFile::remove(filePath))
		{
			if (QFile::exists(filePath))
			{
				QMessageBox::warning(this, "Failed to remove file", "Failed to remove:\n\n" + filePath + "\n");
				continue;
			}

			QMessageBox::warning(this, "Failed to remove file", filePath + "\n\ndoes not exist anymore!\n");
		}

		_model->removePath(filePath);
	}
}

void MainWindow::populateTree(const QString& directory)
{
	_model->clear();
	ui->menuAlgorithm->setEnabled(false);

	auto hashCalculator = new HashCalculator(this, directory, _algorithm);
	connect(hashCalculator, &HashCalculator::processing, [this](const QString& filePath)
	{
		ui->statusBar->showMessage(QTime::currentTime().toString() + " Processing: " + filePath);
	});

	connect(hashCalculator, &HashCalculator::duplicateFound, this, &MainWindow::onDuplicateFound);
	connect(hashCalculator, &HashCalculator::finished, this, &MainWindow::onFinished);
	connect(hashCalculator, &HashCalculator::finished, hashCalculator, &QObject::deleteLater);

	hashCalculator->start();
}

void MainWindow::createFileContextMenu(const QPoint& pos)
{
	const QModelIndex selection = ui->treeViewResults->indexAt(pos);

	if (!selection.isValid())
	{
		qDebug() << "Invalid selection";
		return;
	}

	const QVariant variant = _model->data(selection, Qt::DisplayRole);

	if (!variant.isValid())
	{
		qDebug() << "Invalid variant";
		return;
	}

	const QString filePath = variant.toString();

	if (filePath.isEmpty())
	{
		qDebug() << "File path is empty / no file selected";
		return;
	}

	auto openFileAction = new QAction("Open file", this);
	connect(openFileAction, &QAction::triggered, std::bind(&MainWindow::openFileWithDefaultAssociation, this, filePath));

	auto openParentDirAction = new QAction("Open parent directory", this);
	connect(openParentDirAction, &QAction::triggered, std::bind(&MainWindow::openParentDirectory, this, filePath));

	auto removeFileAction = new QAction("Delete file", this);
	connect(removeFileAction, &QAction::triggered, std::bind(&MainWindow::removeFile, this, filePath));

	QMenu menu(this);
	menu.addActions({ openFileAction, openParentDirAction, removeFileAction });
	menu.exec(ui->treeViewResults->mapToGlobal(pos));
}

void MainWindow::openFileWithDefaultAssociation(const QString& filePath)
{
	const QUrl url = QUrl::fromLocalFile(filePath);

	if (!QDesktopServices::openUrl(url))
	{
		QMessageBox::warning(this, "Failed to open", "Failed to open file:\n\n" + filePath + "\n");
	}
}

void MainWindow::openParentDirectory(const QString& filePath)
{
	const QFileInfo fileInfo(filePath);
	const QUrl url = QUrl::fromLocalFile(fileInfo.dir().path());

	if (!QDesktopServices::openUrl(url))
	{
		QMessageBox::warning(this, "Failed to open", "Failed to open directory:\n\n" + filePath + "\n");
	}
}

bool MainWindow::removeFile(const QString& filePath)
{
	if (QMessageBox::question(
		this,
		"Remove file?", "Are you sure you want to remove:\n\n" + filePath + "\n\nThe file will be permanently deleted.\n",
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No) != QMessageBox::Yes)
	{
		return false;
	}

	if (!QFile::remove(filePath))
	{
		if (QFile::exists(filePath))
		{
			QMessageBox::warning(this, "Failed to remove file", "Failed to remove:\n\n" + filePath + "\n");
			return false;
		}

		QMessageBox::warning(this, "Failed to remove file", filePath + "\n\ndoes not exist anymore!\n");
	}

	_model->removePath(filePath);
	return true;
}

void MainWindow::onAbout()
{
	QStringList text;
	text << "Duff - Duplicate File Finder version 0.1.";
	text << "";
	text << "Duff is yet another duplicate file finder.";
	text << "";
	text << "Duff calculates checksums of files withing given folder."
		"The paths of the files containing a non-unique checksum are displayed in the main view.";
	text << "";
	text << "Duff is open source and written in Qt (C++) see Licenses for more details.";
	text << "";

	QMessageBox::about(this, "Duff", text.join('\n'));
}
