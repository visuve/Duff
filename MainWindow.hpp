#pragma once

#include <QMainWindow>
#include <QCryptographicHash>

#include "HashCalculator.hpp"

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
	void onOpenDirectoryDialog();
	void onFindDuplicates();
	void onDuplicateFound(const QString& hashString, const QString& filePath);
	void onFinished();
	void deleteSelected();
	void onAbout();

private:
	Ui::MainWindow* ui;
	void populateTree(const QString& directory);
	void createFileContextMenu(const QPoint& pos);
	bool removeFile(const QString& filePath);

	QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
};
