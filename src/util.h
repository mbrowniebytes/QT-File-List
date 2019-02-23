#ifndef UTIL_H
#define UTIL_H

#include "structs.h"

class util
{
public:
    util();
    static QString formatBytes(qint64 bytes);
    static suffixIcon suffix2icon(QString suffix);
    static QVector <fileScanInfo> scanDirectory(QString scanDir, QString scanBaseDir, bool recursive, qint32 depth);
    static QVector <fileScanInfo> scanDirectoryJob(QString scanDir);
    static qint64 dirSize(const QString & scanDir);
    static QString simpleEscapeCsv(QString csv);
};

#endif // UTIL_H
