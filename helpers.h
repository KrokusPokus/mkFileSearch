#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QFileInfo>

struct DesktopEntry {
    QString id;
    QString name;
    QString icon;
    QString exec;
    QString path;
    QString workDir;
    bool isValid = false;
};

bool isTextFile(const QString &filePath);
QString cleanFileName(const QString &fileName);
QString formatAdaptiveSize(quint64 bytes);
quint32 calculateCRC32(const QString &filePath);

bool atWordBoundary(const QString &fileName, const QString &word, Qt::CaseSensitivity cs);
uint getNameMatchQuality(const QFileInfo &fileInfo, const QString &searchStringFilename, const QStringList &searchStringSplit, Qt::CaseSensitivity caseSensitivity);
uint getRegExNameMatchQuality(const QFileInfo &fileInfo, const QRegularExpression &re);
uint getContentMatchCount(const QFileInfo &fileInfo, const QString &searchStringContent, Qt::CaseSensitivity caseSensitivity, const QSet<QString> &m_FileExtTextSet);
uint getRegExContentMatchCount(const QFileInfo &fileInfo, const QRegularExpression &re, const QSet<QString> &m_FileExtTextSet);

DesktopEntry getDesktopEntryById(const QString &id);
DesktopEntry getDesktopEntry(const QFileInfo &fileInfo);
void openFileListWithHandler(const QString &handler, const QStringList &fileList);
void launchDesktopFile(const DesktopEntry &info, const QStringList &fileList = {});

#endif // HELPERS_H
