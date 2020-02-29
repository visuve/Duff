#pragma once

#include <QMainWindow>
#include <QCryptographicHash>

#include "hashcalculator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    void populateFileList(const QString& directory);
    void populateTree(const HashToFilePaths& data);

    void createFileContextMenu(const QPoint& pos);

    QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
};
