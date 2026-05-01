#include "helpers.h"
#include <QMimeType>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <zlib.h>

bool isTextFile(const QString &filePath) {
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath);
    return mime.inherits("text/plain");
}

QString cleanFileName(const QString &fileName) {
    QString pattern;

#ifdef Q_OS_WIN
    // Windows verbotene Zeichen: < > : " / \ | ? *
    // Plus: Steuerzeichen (0-31)
    pattern = "[<>:\"/\\\\|?*\\x00-\\x1F]";
#else
    // Linux/Unix verbotene Zeichen: / und der Null-Terminator \0
    pattern = "[/\\x00]";
#endif

    QRegularExpression re(pattern);

    QString cleanedName = fileName; // local copy
    cleanedName.replace(re, "_");

#ifdef Q_OS_WIN
    cleanedName = cleanedName.trimmed();
    while (cleanedName.endsWith('.')) {
        cleanedName.chop(1);
    }
#endif

    return cleanedName;
}

QString formatAdaptiveSize(qint64 bytes) {
    if (bytes < 1024) {
        return QLocale::system().toString(bytes) + " Bytes";
    }

    double size = static_cast<double>(bytes);
    static const QStringList units = {"Bytes", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    int unitIndex = 0;

    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }

    int precision = 0;
    if (size < 10.0) {
        precision = 2; // z.B. 1,23 MiB
    } else if (size < 100.0) {
        precision = 1; // z.B. 12,3 MiB
    } else {
        precision = 0; // z.B. 123 MiB
    }

    return QLocale::system().toString(size, 'f', precision) + " " + units[unitIndex];
}

quint32 calculateCRC32(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0; // Or handle error accordingly
    }

    // Initialize the CRC value
    uLong crc = crc32(0L, Z_NULL, 0);

    // Read in chunks to be memory efficient
    const int bufferSize = 1024 * 1024; // 1 MB buffer
    QByteArray buffer;

    while (!file.atEnd()) {
        buffer = file.read(bufferSize);
        // Update the CRC with the current chunk
        crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), buffer.size());
    }

    file.close();
    return static_cast<quint32>(crc);
}

uint getNameMatchQuality(const QFileInfo &fileInfo, const QString &searchString, Qt::CaseSensitivity caseSensitivity) {
    QString sFilePath = fileInfo.filePath();
    QString sFileName = fileInfo.fileName();
    QString sBaseNameComplete = fileInfo.completeBaseName();    // for "/home/user/archive.tar.gz" this would return "archive.tar"
    QString sBaseName =  fileInfo.baseName();                   // for "/home/user/archive.tar.gz" this would return "archive"

    if ((QString::compare(sFileName, searchString, caseSensitivity) == 0) || (QString::compare(sBaseNameComplete, searchString, caseSensitivity) == 0) || (QString::compare(sBaseName, searchString, caseSensitivity) == 0)) {
        return 1;
    }

    if (sFileName.startsWith(searchString, caseSensitivity)) {
        return 2;
    }

    if ((searchString.contains("/", Qt::CaseSensitive) || searchString.contains("\\", Qt::CaseSensitive)) && sFilePath.contains(searchString, caseSensitivity))  {
        return 3;
    }

    // Match search terms separatately
    bool bMatchType1 = true;
    bool bMatchType2 = false;
    bool bMatchType3 = false;
    int iIndex = 0;
    int iFoundPos = 0;

    QStringList parts = searchString.split(' ', Qt::SkipEmptyParts);

    for (const QString &word : std::as_const(parts)) {
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
        } else {
            if (sFileName.contains(' ' + word, caseSensitivity) || sFileName.contains('[' + word, caseSensitivity)) { // Improved match, since one searchterm found at a word boundary of filename (after " " or "[")
                bMatchType2 = true;
            }
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

uint getRegExNameMatchQuality(const QFileInfo &fileInfo, const QRegularExpression &re) {
    if (re.match(fileInfo.fileName()).hasMatch()) {
        return 1;
    }
    return 0;
}

uint getContentMatchCount(const QFileInfo &fileInfo, const QString &searchStringContent, Qt::CaseSensitivity caseSensitivity, const QSet<QString> &FileExtTextSet) {
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

    int searchStringLength = searchStringContent.length();
    int iCount = 0;
    int iPos = 0;

    while ((iPos = fileContent.indexOf(searchStringContent, iPos, caseSensitivity)) != -1) {
        iCount++;
        iPos += searchStringLength;
    }
    return iCount;
}

uint getRegExContentMatchCount(const QFileInfo &fileInfo, const QRegularExpression &re, const QSet<QString> &FileExtTextSet) {
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
        QRegularExpressionMatch match = it.next();
        iCount++;
    }

    return iCount;
}
