#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QFileInfo>

bool isTextFile(const QString &filePath);
QString cleanFileName(const QString &fileName);
QString formatAdaptiveSize(qint64 bytes);
quint32 calculateCRC32(const QString &filePath);

bool atWordBoundary(const QString &fileName, const QString &word, Qt::CaseSensitivity cs);
uint getNameMatchQuality(const QFileInfo &fileInfo, const QString &searchStringFilename, const QStringList &searchStringSplit, Qt::CaseSensitivity caseSensitivity);
uint getRegExNameMatchQuality(const QFileInfo &fileInfo, const QRegularExpression &re);
uint getContentMatchCount(const QFileInfo &fileInfo, const QString &searchStringContent, Qt::CaseSensitivity caseSensitivity, const QSet<QString> &m_FileExtTextSet);
uint getRegExContentMatchCount(const QFileInfo &fileInfo, const QRegularExpression &re, const QSet<QString> &m_FileExtTextSet);
void launchDesktopFile(const QFileInfo &fileInfo);

#endif // HELPERS_H
