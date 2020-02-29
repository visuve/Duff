#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QFileDialog>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QMessageBox>

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
            populateFileList(dialog.selectedFiles().at(0));
        }
    });

    connect(ui->actionAbout, &QAction::triggered, []()
    {
        qDebug() << "TODO!";
    });

    connect(ui->actionExit, &QAction::triggered, []()
    {
        qDebug() << "Bye!";
        qApp->quit();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateFileList(const QString& directory)
{
    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);

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
            m_fileHashes[hash.result().toHex()].append(path);
        }
    }

    ui->treeWidgetSummary->clear();

    for (const auto& [hash, paths] : m_fileHashes)
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
                    "No duplicate files were found in:\n   " + directory);
    }
    else
    {
        ui->treeWidgetSummary->resizeColumnToContents(0);
    }
}
