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

private slots:
    void onDuplicateFound(const QString& hashString, const QString& filePath);
    void onFinished();

private:
    Ui::MainWindow* ui;
    void populateTree(const QString& directory);
    void createFileContextMenu(const QPoint& pos);
    bool removeFile(const QString& filePath);

    QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
};
