#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QFileInfo>

bool isTextFile(const QString &filePath);
QString cleanFileName(const QString &fileName);
QString formatAdaptiveSize(qint64 bytes);
quint32 calculateCRC32(const QString &filePath);

uint getNameMatchQuality(const QFileInfo &fileInfo, const QString &searchStringFilename, Qt::CaseSensitivity caseSensitivity);
uint getRegExNameMatchQuality(const QFileInfo &fileInfo, const QRegularExpression &re);
uint getContentMatchCount(const QFileInfo &fileInfo, const QString &searchStringContent, Qt::CaseSensitivity caseSensitivity, const QSet<QString> &m_FileExtTextSet);
uint getRegExContentMatchCount(const QFileInfo &fileInfo, const QRegularExpression &re, const QSet<QString> &m_FileExtTextSet);

#endif // HELPERS_H
