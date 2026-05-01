#include "searchworker.h"
#include "helpers.h"
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QCoreApplication>

void SearchWorker::process() {
    QElapsedTimer BenchmarkTimer;
    BenchmarkTimer.start();

    m_abort = false;
    uint iItemsFound = 0;
    uint iNameMatched = 0;
    uint iContentMatched = 0;
    bool bSearchInterrupted = false;
    int iLenRem = m_searchDir.length();
    bool bSearchStringFilenameEmpty = m_searchStringFilename.trimmed().isEmpty();
    bool bsearchStringContentEmpty = m_searchStringContent.trimmed().isEmpty();

    if (!m_searchDir.endsWith(QDir::separator())) {
        iLenRem++;
    }

    QDir::Filters searchFlags = QDir::Hidden | QDir::NoDotAndDotDot | QDir::System; // QDir::System needed for *.lnk files on Windows

    if (m_cbDirState == Qt::Unchecked) {
        searchFlags = searchFlags | QDir::Files;
    } else if (m_cbDirState == Qt::PartiallyChecked) {
        searchFlags = searchFlags | QDir::Files | QDir::Dirs;
    } else /* if (m_cbDirState == Qt::Checked) */ {
        searchFlags = searchFlags | QDir::Dirs;
    }

    QRegularExpression::PatternOptions reOptionsFilename = QRegularExpression::NoPatternOption;
    Qt::CaseSensitivity caseSensitivityFilename = Qt::CaseSensitive;
    if (!m_bFilenameCaseSensitive) {
        reOptionsFilename |= QRegularExpression::CaseInsensitiveOption;
        caseSensitivityFilename = Qt::CaseInsensitive;
    }

    QRegularExpression qreFileName(m_searchStringFilename, reOptionsFilename);
    if (!qreFileName.isValid() && m_bRegExFilename) {
        // The user typed an invalid Regex (e.g., unmatched brackets)
        qDebug() << "Invalid name field Regex:" << qreFileName.errorString();
        return;
    }

    QRegularExpression::PatternOptions reOptionsContent = QRegularExpression::NoPatternOption;
    Qt::CaseSensitivity caseSensitivityContent = Qt::CaseSensitive;
    if (!m_bContentCaseSensitive) {
        reOptionsContent |= QRegularExpression::CaseInsensitiveOption;
        caseSensitivityContent = Qt::CaseInsensitive;
    }

    QRegularExpression qreContent(m_searchStringContent, reOptionsContent);
    if (!qreContent.isValid() && m_bRegExContent) {
        // The user typed an invalid Regex (e.g., unmatched brackets)
        qDebug() << "Invalid content field Regex:" << qreContent.errorString();
        return;
    }

    int nameMatchQuality = -1;
    int contentMatchCount = -1;
    QList<SearchResult> resultsBatch;

    QDirIterator iter(m_searchDir, searchFlags, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        iItemsFound++;

        if (!bSearchStringFilenameEmpty) {
            if (m_bRegExFilename) {
                nameMatchQuality = getRegExNameMatchQuality(iter.fileInfo(), qreFileName);
            } else {
                nameMatchQuality = getNameMatchQuality(iter.fileInfo(), m_searchStringFilename, caseSensitivityFilename);
            }

            if (nameMatchQuality == 0) {
                continue;
            }
        }


        if (!bsearchStringContentEmpty) {
            if (m_bRegExContent) {
                contentMatchCount = getRegExContentMatchCount(iter.fileInfo(), qreContent, m_FileExtTextSet);
            } else {
                contentMatchCount = getContentMatchCount(iter.fileInfo(), m_searchStringContent, caseSensitivityContent, m_FileExtTextSet);
            }

            if (contentMatchCount == 0) {
                continue;
            }

            iContentMatched += contentMatchCount;
        }

        iNameMatched++;

        SearchResult res;
        res.fileInfo = iter.fileInfo();
        res.iLenRem = iLenRem;
        res.nameMatchQuality = nameMatchQuality;
        res.contentMatchCount = contentMatchCount;

        resultsBatch.append(res);

        // Send out the resultsBatch package every 100 hits
        if (resultsBatch.size() >= 1000) {
            emit filesFoundBatch(resultsBatch);
            resultsBatch.clear();
            QCoreApplication::processEvents();
        }

        if (m_abort) {
            bSearchInterrupted = true;
            break;
        }
    }

    if (!resultsBatch.isEmpty()) {
        emit filesFoundBatch(resultsBatch); // Send the remainder
    }

    qDebug() << "SearchWorker::process() finished in" << BenchmarkTimer.elapsed() << "ms";

    emit searchStats(iItemsFound, iNameMatched, iContentMatched, bSearchInterrupted);
    emit finished();
}

void SearchWorker::abort() {
    m_abort = true;
}
