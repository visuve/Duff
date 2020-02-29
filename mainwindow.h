#pragma once

#include <QMainWindow>
#include <QCryptographicHash>
#include <map>

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
    std::map<QString, QStringList> calculateHashes(const QString& directory, QCryptographicHash::Algorithm algorithm);
    void populateFileList(const QString& directory);
    void populateTree(const std::map<QString, QStringList>& data);
    void createFileContextMenu(const QPoint& pos);

    QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
};
