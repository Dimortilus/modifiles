#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtLogging>
#include <QVector>
#include <QtTypes>
#include <QtMinMax>
#include "utils.h"

namespace utils {

// Генерирует для файлов с общей частью имени, имеющих счётчик, следующее имя со счётчиком
QString generateNextCounteredFileName(const QString& directoryPath, const QString& commonFileName) {

	// Информация для извлечения компонент имени файла
	QFileInfo commonFileInfo(commonFileName);
	// Имя файла (без пути и последнего расширения)
	QString commonFileBaseName = commonFileInfo.baseName();
	// Последнее расширение
	QString commonFileExtension = commonFileInfo.suffix();

	// Формирование списка файлов со счётчиком
	QStringList counteredFileNames = findCounteredFileNames(directoryPath, commonFileName);

	// Если список пуст, возвращается имя с счётчиком, равным единице
	if (counteredFileNames.isEmpty()) {
		return QString("%1 (1).%2").arg(commonFileBaseName).arg(commonFileExtension);
	}

	// Регулярное выражение для извлечения значений счётчика
	const QRegularExpression regex(
		QString("^%1 \\(([0-9]+)\\).%2$").arg(commonFileBaseName).arg(commonFileExtension));

	// Текущее максимальное найденное значение счётчика
	quint64 currCounterMaxValue = 0;

	// Перебор списка имён файлов
	for (const QString& counteredfileName : counteredFileNames) {
		// Поиск соответствия регулярному выражению
		QRegularExpressionMatch match = regex.match(counteredfileName);

		// Проверка наличия соответствия
		if (match.hasMatch()) {
			bool conversionSuccess;
			// Преобразование значения первой группы в число
			quint64 counterValue = match.captured(1).toULongLong(&conversionSuccess);

			if (conversionSuccess) {
				// Обновление текущего максимального найденного значения счётчика
				currCounterMaxValue = qMax(currCounterMaxValue, counterValue);
			}
		}
	}

	// Увеличение счётчика на единицу
	currCounterMaxValue++;

	// Новое имя файла
	QString newCounteredFileName = QString("%1 (%2).%3")
									   .arg(commonFileBaseName)
									   .arg(currCounterMaxValue)
									   .arg(commonFileExtension);
	return newCounteredFileName;
}

// Ищет файлы с общей частью имени (только имеющие счётчик) в указанной директории
QStringList findCounteredFileNames(const QString& directoryPath, const QString& commonFileName) {

	// Создание объекта QDir для указанного пути
	QDir dir(directoryPath);

	if (!dir.exists()) {
		qCritical("Директория не существует");
	}

	// Информация для извлечения компонент имени файла
	QFileInfo commonFileInfo(commonFileName);
	QString commonFileBaseName = commonFileInfo.completeBaseName();
	QString commonFileExtension = commonFileInfo.suffix();

	// Регулярное выражение для отбора файлов
	const QString regexStr = QString("^%1 \\([0-9]+\\)\\.%2$")
						   .arg(commonFileBaseName).arg(commonFileExtension);
	const QRegularExpression regex(regexStr);

	QStringList counteredFileNames;

	// Получение списка файлов
	QFileInfoList dirEntries = dir.entryInfoList(QDir::Files);

	// Перебор списка файлов
	for (const auto& dirEntry : dirEntries) {
		QString fileName = dirEntry.fileName();

		// Если имя файла соответствует регулярному выражению, оно добавляется в список
		if (regex.match(fileName).hasMatch()) {
			counteredFileNames.append(fileName);
		}
	}

	return counteredFileNames;
}

// Выполняет XOR на входном буфере, используя буфер-операнд, и возвращает выходной буфер
QByteArray xorInBufWithOperandBuf(QByteArray& inBuf, const QByteArray& operandBuf) {

	// Создание, равного входному по размеру, выходного буфера, и заполенение его нулями
	QByteArray outBuf(inBuf.size(), '\0');
	// Указатель на начало выходного буфера
	char* outBufDataPtr = outBuf.data();
	// Позиция внутри выходного буфера
	size_t outBufPos = 0;
	// Размер буфера-операнда
	const size_t operandBufSize = operandBuf.size();
	// Указатель на начало буфера-операнда
	const char* operandBufDataPtr = operandBuf.data();

	// Побайтовый перебор входного буфера, XOR, заполнение выходного буфера
	for (char inBufCurrByte : inBuf) {
		// Позиция текущего байта буфера-операнда определяется
		//  как остаток от деления позиции выходного буфера на размер буфера-операнда
		outBufDataPtr[outBufPos] =
			inBufCurrByte ^ operandBufDataPtr[outBufPos % operandBufSize];
		outBufPos++;
	}

	return outBuf;
}

}
