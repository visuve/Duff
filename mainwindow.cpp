#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow())
{
    ui->setupUi(this);

    connect(ui->actionOpen, &QAction::triggered, [this]()
    {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::ShowDirsOnly, true);

        if (dialog.exec() == QFileDialog::Accepted)
        {
            ui->treeWidgetSummary->clear();
            const QString directory = dialog.selectedFiles().first();
            qDebug() << "User selected: " << directory;
            populateTree(directory);
        }
    });

    connect(ui->actionExit, &QAction::triggered, []()
    {
        qDebug() << "Bye!";
        qApp->quit();
    });

    connect(ui->actionAbout, &QAction::triggered, [this]()
    {
        QStringList text;
        text << "Duff - Duplicate File Finder version 0.1.";
        text << "";
        text << "Duff is yet another duplicate file finder.";
        text << "";
        text << "Duff calculates checksums of files withing given folder."
                "The paths of the files containing a non-unique checksum are displayed in the main view.";
        text << "";
        text << "Duff is OpenSource and written in Qt (C++) see Licenses for more details.";
        text << "";

        QMessageBox::about(this, "Duff", text.join('\n'));
    });

    connect(ui->actionLicenses, &QAction::triggered, [this]()
    {
        QMessageBox::aboutQt(this);
    });

    ui->treeWidgetSummary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidgetSummary, &QTreeWidget::customContextMenuRequested, this, &MainWindow::createFileContextMenu);

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onProcessed(const QString& hashString, const QString&filePath)
{
    ui->statusBar->showMessage(QTime::currentTime().toString() + " Found duplicate: " + filePath + " -> " + hashString);

    const QList<QTreeWidgetItem*> hashItems = ui->treeWidgetSummary->findItems(hashString, Qt::MatchFlag::MatchExactly);

    if (hashItems.size() > 1)
    {
        qCritical() << "More than one item with the hash '" << hashString << "' found!";
        return;
    }

    QTreeWidgetItem* hashItem = hashItems.empty() ? new QTreeWidgetItem({hashString}) : hashItems.first();
    auto pathItem = new QTreeWidgetItem();
    pathItem->setText(1, filePath);
    hashItem->addChild(pathItem);

    if (hashItems.empty())
    {
        ui->treeWidgetSummary->insertTopLevelItem(0, hashItem);
    }
}

void MainWindow::onFinished()
{
    if (ui->treeWidgetSummary->topLevelItemCount() <= 0)
    {
        QMessageBox::information(
                    this,
                    "No duplicate files",
                    "No duplicate files were found.\n");
    }
    else
    {
        ui->treeWidgetSummary->resizeColumnToContents(0);
    }

    ui->statusBar->showMessage(QTime::currentTime().toString() + " Finished processing.\n ", 10000);
    ui->menuAlgorithm->setEnabled(true);
}

void MainWindow::populateTree(const QString& directory)
{
    ui->treeWidgetSummary->clear();
    ui->menuAlgorithm->setEnabled(false);

    auto hashCalculator = new HashCalculator(this, directory, _algorithm);
    connect(hashCalculator, &HashCalculator::processing, [this](const QString& filePath)
    {
        ui->statusBar->showMessage(QTime::currentTime().toString() + " Processing: " + filePath);
    });

    connect(hashCalculator, &HashCalculator::processed, this, &MainWindow::onProcessed);
    connect(hashCalculator, &HashCalculator::finished, this, &MainWindow::onFinished);
    connect(hashCalculator, &HashCalculator::finished, hashCalculator, &QObject::deleteLater);
    hashCalculator->start();
}

void MainWindow::createFileContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* selection = ui->treeWidgetSummary->itemAt(pos);

    if (!selection)
    {
        qDebug() << "Invalid selection";
        return;
    }

    const QString filePath = selection->text(1);

    if (filePath.isEmpty())
    {
        qDebug() << "File path is empty / no file selected";
        return;
    }

    auto openFile  = new QAction("Open file", this);
    connect(openFile, &QAction::triggered, [=]()
    {
        if (!QDesktopServices::openUrl(filePath))
        {
            QMessageBox::warning(this, "Failed to open", "Failed to open file:\n\n" + filePath + "\n");
        }
    });

    auto openParentDir  = new QAction("Open parent directory", this);
    connect(openParentDir, &QAction::triggered, [=]()
    {
        const QFileInfo fileInfo(filePath);

        if (!QDesktopServices::openUrl(fileInfo.dir().path()))
        {
            QMessageBox::warning(this, "Failed to open", "Failed to open directory:\n\n" + filePath + "\n");
        }
    });

    auto removeFile  = new QAction("Delete file", this);
    connect(removeFile, &QAction::triggered, [=]()
    {
        if (QMessageBox::question(
                    this,
                    "Remove file?", "Are you sure you want to remove:\n\n" + filePath + "\n\nThe file will be permanently deleted.\n",
                    QMessageBox::Yes|QMessageBox::No,
                    QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        if (!QFile::remove(filePath))
        {
            if (QFile::exists(filePath))
            {
                QMessageBox::warning(this, "Failed to remove file", "Failed to remove:\n\n" + filePath + "\n");
                return;
            }

            QMessageBox::warning(this, "Failed to remove file", filePath + "\n\ndoes not exist anymore!\n");
        }

        auto parent = selection->parent();
        parent->removeChild(selection);

        if (parent->childCount() <= 1)
        {
            delete parent;
        }

        if (ui->treeWidgetSummary->topLevelItemCount() <= 0)
        {
            QMessageBox::information(this, "No duplicate files", "No duplicate files left!\n");
        }
    });

    QMenu menu(this);
    menu.addAction(openFile);
    menu.addAction(openParentDir);
    menu.addAction(removeFile);
    menu.exec(ui->treeWidgetSummary->mapToGlobal(pos));
}
