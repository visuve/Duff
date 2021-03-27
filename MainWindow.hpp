#pragma once

#include <QMainWindow>
#include <QCryptographicHash>

namespace Ui
{
	class MainWindow;
}

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

private:
	Ui::MainWindow* ui;
	void populateTree(const QString& directory);
	void createFileContextMenu(const QPoint& pos);
	void openFileWithDefaultAssociation(const QString& filePath);
	void openParentDirectory(const QString& filePath);
	bool removeFile(const QString& filePath);

	QCryptographicHash::Algorithm _algorithm = QCryptographicHash::Sha256;
	ResultModel* _model = nullptr;
};
