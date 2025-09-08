#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"

#include <Qt>
#include <QIODevice>
#include <QCheckBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QtTypes>
#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QtConcurrent>

const QString MainWindow::processingInActionString = "Выполняется обработка...";
const QString MainWindow::processingDoneString = "Обработка завершена";

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// Выбор пути входных файлов
	connect(ui->inputFilesPathDialogPushButton, &QPushButton::clicked,
			this, &MainWindow::onInputFilesPathDialog);

	// Выбор пути сохранения файлов
	connect(ui->savePathDialogPushButton, &QPushButton::clicked,
			this, &MainWindow::onSavePathDialog);

	// Настройка доступности виджета, задающего интервал для таймера
	connect(ui->enableTimerCheckBox, &QCheckBox::checkStateChanged,
			ui->timerSpinBox, &QSpinBox::setEnabled);

	// Маска операнда
	ui->operandLineEdit->setInputMask("hhhhhhhhhhhhhhhh");

	// Обработка сигнала нажатия кнопки "Старт"
	connect(ui->startPushButton, &QPushButton::clicked,
			this, &MainWindow::onStartPushButtonClick);

	// Установка соответствия между слотом и сигналом изменения индикатора выполнения
	connect(this, &MainWindow::progressUpdated, this,
			&MainWindow::onProgressUpdate, Qt::QueuedConnection);

	// Установка минимального размера для основного окна приложения
	resize(this->minimumSize());
}

MainWindow::~MainWindow()
{
	delete ui;
}

// Слот для диалогового окна выбора пути входных файлов
void MainWindow::onInputFilesPathDialog()
{
	QString inputFilesPath = QFileDialog::getExistingDirectory(
		this, "Путь входных файлов",
		QDir::currentPath(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
		);

	if (!inputFilesPath.isEmpty()) {
		ui->inputFilesPathLineEdit->setText(inputFilesPath);
	}
}

// Слот для диалогового окна выбора пути сохранения файлов
void MainWindow::onSavePathDialog()
{
	const QString savePath = QFileDialog::getExistingDirectory(
		this, "Путь сохранения файлов",
		QDir::currentPath(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
		);

	if (!savePath.isEmpty()) {
		ui->savePathLineEdit->setText(savePath);
	}
}

// Слот для изменения индикатора выполнения
void MainWindow::onProgressUpdate(int percentage) {
	ui->processingProgressBar->setValue(percentage);
}

// Слот для действий по итогу завершения обработки
void MainWindow::onProcessingCompleted() {
	// Cтатус обработки: завершена
	ui->processingStatusLabel->setText(MainWindow::processingDoneString);

	// Если установлен чекбокс работы по таймеру
	if (ui->enableTimerCheckBox->isChecked()) {
		// Преобразуем указанное на форме значение таймера в миллисекунды
		int timerIntervalMs = ui->timerSpinBox->value() * 1000;
		// Одноразовый таймер, который спустя указанное время
		//  вызовет функцию обработки нажатия кнопки "Запуск"
		QTimer::singleShot(timerIntervalMs, this, &MainWindow::onStartPushButtonClick);
	}

	// Разблокировать интерактивные элементы интерфейса
	enableMostInputWidgets();
}

// Слот для обработки нажатия кнопки "Запуск"
void MainWindow::onStartPushButtonClick()
{
	// Блокировать интерактивные элементы интерфейса
	disableMostInputWidgets();

	// Сброс индикатора выполнения
	ui->processingProgressBar->reset();
	// Cтатус обработки: обработка...
	ui->processingStatusLabel->setText(MainWindow::processingInActionString);

	// Обработка в отдельном потоке, чтобы основная программа не "замирала".
	QFuture<void> future = QtConcurrent::run(doProcessing, this);

	// "Сброс" указателя - освобождение старой памяти, и замена на новую
	processingWatcher.reset(new QFutureWatcher<void>());

	// Установка соответствия между слотом и сигналом завершения исполнения future
	connect(processingWatcher.get(), &QFutureWatcherBase::finished,
			this, &MainWindow::onProcessingCompleted);

	// Отслеживание future
	processingWatcher->setFuture(future);
}

void MainWindow::doProcessing()
{
	// Маска входных файлов
	QStringList fileMask { ui->fileMaskLineEdit->text() };

	// Формируем список входных файлов директории, с отбором по маске
	QDir inputFilesDir(ui->inputFilesPathLineEdit->text());
	QFileInfoList inputFilesInfoList = inputFilesDir.entryInfoList(
		fileMask, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot
	);

	int progressCounter = 0;

	// Прохождение по списку
	for (const QFileInfo inputFileInfo : inputFilesInfoList) {

		// Путь входного файла
		QString inputFilePath = inputFileInfo.absoluteFilePath();

		// Открытие входного файла на чтение
		QFile inputFile(inputFilePath);
		inputFile.open(QIODevice::ReadOnly);

		// Извлечение компонент имени входного файла
		// Имя файла (путь, без расширения)
		QString inputFileBaseName = inputFileInfo.completeBaseName();
		// Расширение
		QString inputFileExtension = inputFileInfo.suffix();

		// Путь директории выходного файла
		QString outputFileDirPath = ui->savePathLineEdit->text();
		// Полный путь выходного файла (включая имя)
		QString outputFilePath = QDir(outputFileDirPath)
									 .filePath(inputFileBaseName + "." + inputFileExtension);

		// Информация для проверки наличия файла
		QFileInfo outputFileInfo(outputFilePath);

		// Если одноимённый выходной файл уже существует,
		//  и не установлен чекбокс на перезапись,
		//  тогда формируется новое имя файла,
		//  с счётчиком на единицу больше чем у файла,
		//  имеющего в имени максимальное значение счётчика на данный момент
		if (outputFileInfo.exists() &&
			!ui->overwriteSameNameOutputFilesCheckBox->isChecked()) {

			// Формирование нового имени выходного файла (с расширением)
			QString outputFileCounteredName = utils::generateNextCounteredFileName(
				outputFileDirPath, outputFileInfo.fileName());

			// Путь выходного файла (с расширением)
			outputFilePath = QDir(outputFileDirPath).filePath(outputFileCounteredName);
		}

		// Открытие выходного файла на запись
		QFile outputFile(outputFilePath);
		outputFile.open(QIODevice::WriteOnly);

		// Цикл для прохода по всему файлу
		while (!inputFile.atEnd()) {

			// Чтение в буфер 4 КиБ (типичный размер страницы виртуальной памяти),
			//  либо меньше
			constexpr qint64 BUF_SIZE = 4096;
			QByteArray inputFileBytesBuf = inputFile.read(BUF_SIZE);

			// Буфер-операнд для XOR
			QByteArray operandBytesBuf =
				QByteArray::fromHex(ui->operandLineEdit->text().toUtf8());

			// XOR
			QByteArray resultFileBytesBuf = utils::xorInBufWithOperandBuf(
				inputFileBytesBuf, operandBytesBuf);

			outputFile.write(resultFileBytesBuf);
		}

		// Если установлен флаг удаления входных файлов - удалить входной файл
		if (ui->deleteInputFilesCheckBox->isChecked()) {
			inputFile.remove();
		}

		// Закрытие входного файла
		inputFile.close();
		// Закрытие выходного файла
		outputFile.close();

		// Изменение индикатора выполнения
		++progressCounter;
		double progressPercentage =
			(static_cast<double>(progressCounter) / inputFilesInfoList.size()) * 100.0;
		// Отправка сигнала
		emit progressUpdated(static_cast<int>(progressPercentage));
	}
}

// Включает почти все интерактивные элементы интерфейса
//  (кроме чекбокса и спинбокса таймера)
void MainWindow::enableMostInputWidgets(bool enable) {
	ui->deleteInputFilesCheckBox->setEnabled(enable);
	ui->overwriteSameNameOutputFilesCheckBox->setEnabled(enable);
	ui->operandLineEdit->setEnabled(enable);
	ui->inputFilesPathDialogPushButton->setEnabled(enable);
	ui->fileMaskLineEdit->setEnabled(enable);
	ui->savePathDialogPushButton->setEnabled(enable);
	ui->startPushButton->setEnabled(enable);
}

// Выключает почти все интерактивные элементы интерфейса
//  (кроме чекбокса и спинбокса таймера)
void MainWindow::disableMostInputWidgets() {
	MainWindow::enableMostInputWidgets(false);
}
