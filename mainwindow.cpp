#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QFileDialog>
#include <QDirIterator>
#include <QCryptographicHash>

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

    QList<QTreeWidgetItem *> items;

    for (const auto& [key, value] : m_fileHashes)
    {
        auto hash = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), { key });
        hash->addChild(new QTreeWidgetItem(hash, value));
        items.append(hash);
    }

    ui->treeWidgetSummary->insertTopLevelItems(0, items);

}
