#pragma once

#include <QMainWindow>
#include <QCryptographicHash>
#include <QStateMachine>

namespace Ui
{
	class MainWindow;
}

class HashCalculator;
class ResultModel;

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

signals:
	void inputReady();
	void inputIncomplete();

private:
	void populateTree(const QString& directory);
	void createFileContextMenu(const QPoint& pos);
	void openFileWithDefaultAssociation(const QString& filePath);
	void openParentDirectory(const QString& filePath);
	bool removeFile(const QString& filePath);

	Ui::MainWindow* ui;
	HashCalculator* _hashCalculator;
	ResultModel* _model;
	QStateMachine _machine;
};
