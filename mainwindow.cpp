#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QFileDialog>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QDesktopServices>

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
            populateFileList(dialog.selectedFiles().at(0));
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
        text << "Duff calculates SHA-256 checksums of files withing given folder."
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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateFileList(const QString& directory)
{
    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
    std::map<QString, QStringList> fileHashes;

    while (it.hasNext())
    {
        QString path = it.next();
        QFile file(path);

        if (!file.open(QFile::ReadOnly))
        {
            qWarning() << "Failed to open" << path;
            continue;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);

        if (hash.addData(&file))
        {
            fileHashes[hash.result().toHex()].append(path);
        }
    }

    ui->treeWidgetSummary->clear();

    for (const auto& [hash, paths] : fileHashes)
    {
        if (paths.size() > 1)
        {
            auto hashItem = new QTreeWidgetItem();
            hashItem->setText(0, hash);

            for (const QString& path : paths)
            {
                auto pathItem = new QTreeWidgetItem();
                pathItem->setText(1, path);
                hashItem->addChild(pathItem);
            }

            ui->treeWidgetSummary->insertTopLevelItem(0, hashItem);
        }
    }

    if (ui->treeWidgetSummary->topLevelItemCount() <= 0)
    {
        QMessageBox::information(
                    this,
                    "No duplicate files",
                    "No duplicate files were found in:\n\n" + directory + "\n");
    }
    else
    {
        ui->treeWidgetSummary->resizeColumnToContents(0);
    }
}

void MainWindow::createFileContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* selection = ui->treeWidgetSummary->itemAt(pos);

    if (!selection)
    {
        return;
    }

    const QString filePath = selection->text(1);

    if (filePath.isEmpty())
    {
        return;
    }

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
                    QMessageBox::No) == QMessageBox::Yes)
        {
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
        }
    });

    QMenu menu(this);
    menu.addAction(openParentDir);
    menu.addAction(removeFile);
    menu.exec(ui->treeWidgetSummary->mapToGlobal(pos));
}
