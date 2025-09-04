#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

signals:
	void progressUpdated(int percent);

private slots:
	void onInputFilesPathDialog();
	void onSavePathDialog();
	void onProgressUpdate(int percentage);
	void onProcessingCompleted();
	void onStartPushButtonClick();

private:
	Ui::MainWindow* ui;
	static const QString processingInActionString;
	static const QString processingDoneString;
	std::unique_ptr<QFutureWatcher<void>> processingWatcher = nullptr;
	void doProcessing();
	void disableMostInputWidgets();
	void enableMostInputWidgets(bool enable = true);

};
#endif // MAINWINDOW_H
