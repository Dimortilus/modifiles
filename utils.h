#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QByteArray>

namespace utils {
QStringList findCounteredFileNames(const QString& directoryPath, const QString& fileName);
QString generateNextCounteredFileName(const QString& directoryPath, const QString& fileName);
QByteArray xorInBufWithOperandBuf(QByteArray& inBuf, const QByteArray& operandBuf);
}

#endif // UTILS_H
