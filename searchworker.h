#ifndef SEARCHWORKER_H
#define SEARCHWORKER_H

#include <QFileInfo>
#include <QObject>
#include <QSet>

struct SearchResult {
    QFileInfo fileInfo;
    int iLenRem;
    int nameMatchQuality;
    int contentMatchCount;
};

// WICHTIG: Damit Qt diese Struktur zwischen Threads verschicken kann,
// muss sie bekannt gemacht werden (falls sie nicht nur einfache Typen enthält).
Q_DECLARE_METATYPE(SearchResult)

class SearchWorker : public QObject {
    Q_OBJECT

public:
    explicit SearchWorker(const QString &searchDir, const QString &searchStringFilename, const QString &searchStringContent, bool bRegExFilename, bool bRegExContent, bool bFilenameCaseSensitive, bool bContentCaseSensitive, Qt::CheckState cbDirState, const QSet<QString> &FileExtTextSet, QObject *parent = nullptr)
        : QObject(parent), m_searchDir(searchDir), m_searchStringFilename(searchStringFilename), m_searchStringContent(searchStringContent), m_bRegExFilename(bRegExFilename), m_bRegExContent(bRegExContent), m_bFilenameCaseSensitive(bFilenameCaseSensitive), m_bContentCaseSensitive(bContentCaseSensitive), m_cbDirState(cbDirState), m_FileExtTextSet(FileExtTextSet) {}

signals:
    void fileFound(const QFileInfo &fileInfo, int iRow, int iLenRem, int nameMatchQuality, int contentMatchCount);
    void filesFoundBatch(const QList<SearchResult> &results);
    void searchStats(uint iItemsFound, uint iNameMatched, uint iContentMatched, bool bSearchInterrupted);
    void finished();

public slots:
    void process();
    void abort();

private:
    bool m_abort;
    QString m_searchDir;
    QString m_searchStringFilename;
    QString m_searchStringContent;
    bool m_bRegExFilename;
    bool m_bRegExContent;
    bool m_bFilenameCaseSensitive;
    bool m_bContentCaseSensitive;
    Qt::CheckState m_cbDirState;
    QSet<QString> m_FileExtTextSet;
};

#endif // SEARCHWORKER_H
