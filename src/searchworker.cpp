#include "searchworker.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QRegularExpression>

void SearchWorker::process() {
    QElapsedTimer BenchmarkTimer;
    BenchmarkTimer.start();

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

    QStringList searchStringFilenameSplit = m_searchStringFilename.split(' ', Qt::SkipEmptyParts);

    int nameMatchQuality = -1;
    int contentMatchCount = -1;
    QList<SearchResult> resultsBatch;
    resultsBatch.reserve(1000); // small optimization. no measurable gains though.

    QDirIterator iter(m_searchDir, searchFlags, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();

        if (m_abort.load()) {
            emit searchStats(iItemsFound, iNameMatched, iContentMatched, true);
            emit finished();
            return;
        }

        iItemsFound++;

        if (!bSearchStringFilenameEmpty) {
            if (m_bRegExFilename) {
                nameMatchQuality = getRegExNameMatchQuality(iter.fileInfo(), qreFileName);
            } else {
                nameMatchQuality = getNameMatchQuality(iter.fileInfo(), m_searchStringFilename, searchStringFilenameSplit, caseSensitivityFilename);
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
        }
    }

    if (!resultsBatch.isEmpty() && !m_abort.load()) {
        emit filesFoundBatch(resultsBatch); // Send the remainder
    }

    qDebug() << "SearchWorker::process() finished in" << BenchmarkTimer.elapsed() << "ms";

    emit searchStats(iItemsFound, iNameMatched, iContentMatched, bSearchInterrupted);
    emit finished();
}

void SearchWorker::abort() {
    m_abort.store(true);
}

bool SearchWorker::atWordBoundary(const QString &fileName, const QString &word, Qt::CaseSensitivity caseSensitivity) {
    static const QString separators = " .(-_[";
    int pos = 0;
    while ((pos = fileName.indexOf(word, pos, caseSensitivity)) != -1) {
        if (pos > 0 && separators.contains(fileName[pos - 1])) {
            return true;
        }
        pos += word.length();
    }
    return false;
};

uint SearchWorker::getNameMatchQuality(const QFileInfo &fileInfo, const QString &searchString, const QStringList &searchStringSplit, Qt::CaseSensitivity caseSensitivity) {
    QString sFileName = fileInfo.fileName();
    QString sBaseNameComplete = fileInfo.completeBaseName();    // for "/home/user/archive.tar.gz" this would return "archive.tar"
    QString sBaseName =  fileInfo.baseName();                   // for "/home/user/archive.tar.gz" this would return "archive"

    if ((QString::compare(sFileName, searchString, caseSensitivity) == 0) || (QString::compare(sBaseNameComplete, searchString, caseSensitivity) == 0) || (QString::compare(sBaseName, searchString, caseSensitivity) == 0)) {
        return 1;
    }

    if (sFileName.startsWith(searchString, caseSensitivity)) {
        return 2;
    }

    if (searchString.contains("/", Qt::CaseSensitive) || searchString.contains("\\", Qt::CaseSensitive))  {
        QString sFilePath = fileInfo.filePath();
        if (sFilePath.contains(searchString, caseSensitivity)) {
            return 3;
        }
    }

    // Match search terms separatately
    bool bMatchType1 = true;
    bool bMatchType2 = false;
    bool bMatchType3 = false;
    int iIndex = 0;
    int iFoundPos = 0;

    for (const QString &word : std::as_const(searchStringSplit)) {
        iFoundPos = sFileName.indexOf(word, 0, caseSensitivity);

        if (iFoundPos == -1) { // not found
            bMatchType1 = false;
            break;
        }

        if (iFoundPos == 0) {   // Improved match, since one searchterm found at very beginning of filename
            bMatchType2 = true;
            if (iIndex == 0) {  // Further improved match, since *first* searchterm found at very beginning of filename
                bMatchType3 = true;
            }
        } else if (atWordBoundary(sFileName, word, caseSensitivity)) {
            // Improved match: one searchterm found at a word boundary in filename
            bMatchType2 = true;
        }

        iIndex++;
    }

    if (bMatchType1 == false) {
        return 0;
    }

    if (bMatchType3 == true) {
        return 4;
    } else if (bMatchType2 == true) {
        return 5;
    } else {
        return 6;
    }
}

uint SearchWorker::getRegExNameMatchQuality(const QFileInfo &fileInfo, const QRegularExpression &re) {
    if (re.match(fileInfo.fileName()).hasMatch()) {
        return 1;
    }
    return 0;
}

uint SearchWorker::getContentMatchCount(const QFileInfo &fileInfo, const QString &searchStringContent, Qt::CaseSensitivity caseSensitivity, const QSet<QString> &FileExtTextSet) {
    //if (!isTextFile(fileInfo.filePath())) {   // much, MUCH slower...

    QString sFileExt = fileInfo.suffix().toLower(); // since contains() is CaseSensitive by default
    if (!FileExtTextSet.contains(sFileExt)) {
        return 0;
    }

    if (fileInfo.size() > 64 * 1024 * 1024) {
        return 0;
    }

    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    QTextStream in(&file);

    // Option A: explizit UTF-8 erzwingen (aktuelles Verhalten dokumentieren)
    //in.setEncoding(QStringConverter::Utf8);
    // Option B: Locale-basiert dekodieren (breiter, aber langsamer)
    //in.setEncoding(QStringConverter::System);

    QString fileContent = in.readAll();

    int searchStringLength = searchStringContent.length();
    int iCount = 0;
    int iPos = 0;

    while ((iPos = fileContent.indexOf(searchStringContent, iPos, caseSensitivity)) != -1) {
        iCount++;
        iPos += searchStringLength;
    }
    return iCount;
}

uint SearchWorker::getRegExContentMatchCount(const QFileInfo &fileInfo, const QRegularExpression &re, const QSet<QString> &FileExtTextSet) {
    //if (!isTextFile(fileInfo.filePath())) {   // much, MUCH slower...

    QString sFileExt = fileInfo.suffix().toLower(); // since contains() is CaseSensitive by default
    if (!FileExtTextSet.contains(sFileExt)) {
        return 0;
    }

    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    if (file.size() > 64 * 1024 * 1024) {
        return 0;
    }

    QTextStream in(&file);
    QString fileContent = in.readAll();

    QRegularExpressionMatchIterator it = re.globalMatch(fileContent);
    int iCount = 0;

    while (it.hasNext()) {
        it.next();
        iCount++;
    }

    return iCount;
}
