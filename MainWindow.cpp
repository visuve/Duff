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
	_hashCalculator(new HashCalculator(this)),
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

	connect(ui->actionMD5, &QAction::triggered,
			std::bind(&HashCalculator::setAlgorithm, _hashCalculator, QCryptographicHash::Algorithm::Md5));
	connect(ui->actionSHA_1, &QAction::triggered,
			std::bind(&HashCalculator::setAlgorithm, _hashCalculator, QCryptographicHash::Algorithm::Sha1));
	connect(ui->actionSHA_256, &QAction::triggered,
			std::bind(&HashCalculator::setAlgorithm, _hashCalculator, QCryptographicHash::Algorithm::Sha256));
	connect(ui->actionSHA_512, &QAction::triggered,
			std::bind(&HashCalculator::setAlgorithm, _hashCalculator, QCryptographicHash::Algorithm::Sha512));

	ui->actionSHA_256->setChecked(true);

	ui->treeViewResults->setModel(_model);

	connect(_hashCalculator, &HashCalculator::processing, [this](const QString& filePath)
	{
		ui->statusBar->showMessage(QTime::currentTime().toString() + " Processing: " + filePath);
	});

	connect(_hashCalculator, &HashCalculator::duplicateFound, this, &MainWindow::onDuplicateFound);
	connect(_hashCalculator, &HashCalculator::finished, this, &MainWindow::onFinished);

	const QStringList args = QCoreApplication::arguments();

	if (args.count() == 2 && QDir().exists(args[1]))
	{
		const QString& path = args[1];
		ui->lineEditSelectedDirectory->setText(path);
		populateTree(path);
	}

	connect(ui->lineEditSelectedDirectory, &QLineEdit::textChanged, [this](const QString& text)
	{
		const QFileInfo info(text);
		QPalette palette;

		if (info.isDir())
		{
			emit inputReady();
		}
		else
		{
			palette.setColor(text.isEmpty() ? QPalette::Window : QPalette::Text, Qt::red);
			emit inputIncomplete();
		}

		ui->lineEditSelectedDirectory->setPalette(palette);
	});

	auto emptyState = new QState();
	emptyState->assignProperty(ui->pushButtonFindDuplicates, "enabled", false);
	emptyState->assignProperty(ui->lineEditSelectedDirectory, "enabled", true);
	emptyState->setObjectName("empty");

	auto readyState = new QState();
	readyState->assignProperty(ui->pushButtonFindDuplicates, "text", "Find duplicates");
	readyState->assignProperty(ui->pushButtonFindDuplicates, "enabled", true);
	readyState->assignProperty(ui->lineEditSelectedDirectory, "enabled", true);
	readyState->setObjectName("ready");

	auto runningState = new QState();
	runningState->assignProperty(ui->pushButtonFindDuplicates, "text", "Cancel");
	runningState->assignProperty(ui->lineEditSelectedDirectory, "enabled", false);
	runningState->setObjectName("running");

	emptyState->addTransition(this, &MainWindow::inputReady, readyState);
	readyState->addTransition(this, &MainWindow::inputIncomplete, emptyState);
	readyState->addTransition(ui->pushButtonFindDuplicates, &QAbstractButton::clicked, runningState);
	runningState->addTransition(_hashCalculator, &HashCalculator::finished, readyState);
	runningState->addTransition(ui->pushButtonFindDuplicates, &QAbstractButton::clicked, readyState);

	connect(readyState, &QState::entered, [this]()
	{
		_hashCalculator->requestInterruption();
	});

	connect(runningState, &QState::entered, this, &MainWindow::onFindDuplicates);

	_machine.addState(emptyState);
	_machine.addState(readyState);
	_machine.addState(runningState);

	_machine.setInitialState(emptyState);
	_machine.start();

	connect(ui->pushButtonDeleteSelected, &QPushButton::clicked, this, &MainWindow::deleteSelected);
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

	if (!QDir(selectedDirectory).exists())
	{
		QMessageBox::warning(this, "Selected directory", '"' + selectedDirectory + '"' + " does not appear to exist!");
		onOpenDirectoryDialog();
		return;
	}

	populateTree(selectedDirectory);
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
	ui->treeViewResults->expandAll();
}

void MainWindow::deleteSelected()
{
	const QStringList filePaths = _model->selectedPaths();

	if (filePaths.empty())
	{
		QMessageBox::warning(this, "Failed to remove file", "Nothing selected!\n");
		return;
	}

	if (QMessageBox::question(
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
	_hashCalculator->setDirectory(directory);
	_hashCalculator->start();
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
