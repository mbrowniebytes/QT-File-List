#include "mainwindow.h"
#include "util.h"
#include "structs.h"

#include <QDebug>
#include <QMessageBox>

#include <QDirIterator>
#include <QDateTime>
#include <QtWidgets>

#include <QtConcurrent>
#include <QThread>
#include <QFutureWatcher>
#include <QEventLoop>

util::util()
{

}

/**
 * @brief format bytes to largest unit
 * @param bytes
 * @return
 */
QString util::formatBytes(qint64 bytes)
{
    qint64 size = bytes;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("B");

    while (size >= 1024.0 && i.hasNext()) {
        unit = i.next();
        size /= 1024.0;
    }
    return QString().number(size, 'f', 2)+" "+unit;
}

/**
 * @brief choose an icon based on suffix
 * @param suffix
 * @return
 */
suffixIcon util::suffix2icon(QString suffix)
{
    QIcon iconImage;
    QString iconFile;
    QString iconName;

    if ((QStringList() << "mkv" << "mp4" << "mpg" << "m4v").indexOf(suffix) != -1) {
        iconFile = "movie.png";
        iconName = "Movie";
    } else if ((QStringList() << "jpg" << "jpeg" << "png").indexOf(suffix) != -1) {
        iconFile = "image.png";
        iconName = "Image";
    } else if ((QStringList() << "nfo").indexOf(suffix) != -1) {
        iconFile = "nfo.png";
        iconName = "Info";
    } else if ((QStringList() << "txt" << "text").indexOf(suffix) != -1) {
        iconFile = "text.png";
        iconName = "Text";
    } else if (suffix == "dir") {
        iconFile = "folder.png";
        iconName = "Folder";
    } else {
        iconFile = "unknown.png";
        iconName = "Unknown";
    }

    iconFile = ":/icons/" + iconFile;
    iconImage.addFile(iconFile, QSize(16, 16));

    suffixIcon icon;
    icon.iconImage = iconImage;
    icon.iconName = iconName;

    return icon;
}

/**
 * @brief find size of all files and sub dirs in dir
 * @param str
 * @return
 */
qint64 util::dirSize(const QString & scanDir)
{
    qint64 dSize = 0;
    QFileInfo fileInfo(scanDir);
    if (fileInfo.isDir()) {
        QDir dir(scanDir);
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs |  QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        long nbrList = list.size();
        for (long i = 0; i < nbrList; ++i) {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.isDir()) {
                dSize += dirSize(fileInfo.absoluteFilePath());
            } else {
                dSize += fileInfo.size();
            }
        }
    }
    return dSize;
}

/**
 * @brief overly simple escape csv string
 * @param csv
 * @return
 */
QString util::simpleEscapeCsv(QString csv)
{
    return csv.replace("\"", "\"\"").replace(",", "\\,").trimmed();
}

/**
 * @brief recursivley scan a directory and return info about its directories and files
 * @param scanDir
 * @param scanBaseDir
 * @param recursive
 * @param depth
 * @return
 */
QVector <fileScanInfo> util::scanDirectory(QString scanDir, QString scanBaseDir, bool recursive, qint32 depth)
{
    QVector <fileScanInfo> fileScanInfos;
    fileScanInfos.reserve(1000);
    QVector <fileScanInfo> subDirFileScanInfos;
    subDirFileScanInfos.reserve(1000);
    QMessageBox messageBox;

    // validate directory to scan
    if (scanDir.isEmpty()) {
        messageBox.setText("Select or enter a directory");
        messageBox.exec();
        return fileScanInfos;
    }
    if (!QDir(scanDir).exists()) {
        messageBox.setText("Cannot access " + scanDir);
        messageBox.exec();
        return fileScanInfos;
    }

    qint32 scanBaseDirLength = scanBaseDir.length();

    QDirIterator::IteratorFlags iteratorFlags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        // qt will scan subdirs, but to stop at a max depth and to sum dir size, do recurrsion ourselves
        // iteratorFlags = QDirIterator::Subdirectories;
    } else {
        iteratorFlags = QDirIterator::NoIteratorFlags;
    }

    // scan dir
    QDirIterator it(scanDir, QDir::Files | QDir::Dirs |  QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, iteratorFlags);
    while (it.hasNext()) {
        if (fileScanInfos.length() > 10000) {
            // break;
        }
        // get system info about file
        QFileInfo fileInfo;
        try {
            fileInfo = it.next();
        } catch (std::exception &e) {
            qDebug("Error %s with fileInfo on QDirIterator ", e.what());
            continue;
        } catch (...) {
            qDebug("Error <unknown> with fileInfo on QDirIterator");
            continue;
        }

        if (!fileInfo.isReadable() || fileInfo.absoluteFilePath().isEmpty() || fileInfo.birthTime().isNull()) {
            qDebug("Error <empty> with fileInfo on QDirIterator %s ", qUtf8Printable(fileInfo.absoluteFilePath()));
            continue;
        }


        fileScanInfo scanResult;
        scanResult.absoluteFilePath = fileInfo.absoluteFilePath();
        // qint32 absoluteFilePathLength = scanResult.absoluteFilePath.length();

        // remove scan dir from file path
        scanResult.relativeFilePath = fileInfo.absoluteFilePath().remove(0, scanBaseDirLength + 1);
        qint32 relativeFilePathLength = scanResult.relativeFilePath.length();

        scanResult.fileName = fileInfo.fileName();
        qint32 fileNameLength = scanResult.fileName.length();

        // remove scan dir from file path and remove file name
        scanResult.relativeDir = scanResult.relativeFilePath;
        if (scanResult.fileName != scanResult.relativeDir) {
            scanResult.relativeDir.remove(relativeFilePathLength - fileNameLength - 1, relativeFilePathLength);
        }

        scanResult.suffix = fileInfo.isDir() ? "dir" : fileInfo.suffix();

        suffixIcon icon = util::suffix2icon(scanResult.suffix);
        scanResult.iconImage = icon.iconImage;
        scanResult.iconName = icon.iconName;

        QDateTime lastModified = fileInfo.lastModified();
        scanResult.lastModified = lastModified.toString("yyyy-MM-dd hh:mm:ss");

        scanResult.size = fileInfo.size();
        scanResult.sizeFormatted = util::formatBytes(scanResult.size);

        scanResult.depth = depth;

        if (recursive && fileInfo.isDir()) {
            // allow gui to be somewhat responsive
            QCoreApplication::processEvents(QEventLoop::AllEvents, 4000);

            if (depth > 5) {
                scanResult.dirType = "sub";
                scanResult.size = 0;
                scanResult.sizeFormatted = '-';
                fileScanInfos.append(scanResult);
                continue;
            }

            // scan dir and sum up size
            qint64 subDirSize = 0;
            subDirFileScanInfos = scanDirectory(fileInfo.absoluteFilePath(), scanBaseDir, recursive, depth + 1);

            long nbrFileScanInfos = subDirFileScanInfos.length();

            // accumulate size of dir/file in parent dir
            for (long indexS1 = 0; indexS1 < nbrFileScanInfos; indexS1++) {
                fileScanInfo scanInfo = subDirFileScanInfos.at(indexS1);

                subDirSize += scanInfo.size;
            }

            // add parent dir info
            scanResult.dirType = "sub";
            scanResult.size = subDirSize;
            scanResult.sizeFormatted = util::formatBytes(scanResult.size);

            fileScanInfos.append(scanResult);

            // add sub dir/file info
            for (long indexS2 = 0; indexS2 < nbrFileScanInfos; indexS2++) {
                fileScanInfo scanInfo = subDirFileScanInfos.at(indexS2);

                fileScanInfos.append(scanInfo);
            }
        } else {
            // add file info
            fileScanInfos.append(scanResult);
        }
    }

    return fileScanInfos;
}

/**
 * @brief scan top level dirs, and then scan thier sub dirs
 * @param scanDir
 * @return
 */
QVector <fileScanInfo> util::scanDirectoryJob(QString scanDir)
{
    QMessageBox messageBox;

    QVector <fileScanInfo> fileScanInfos;
    fileScanInfos.reserve(100000);

    // scan parent dirs only, then scan their dirs in threads
    QVector <fileScanInfo> parentFileScanInfos = util::scanDirectory(scanDir, scanDir, false, 1);
    qint32 nbrParentFileScanInfos = parentFileScanInfos.length();

    for (qint32 indexP = 0; indexP < nbrParentFileScanInfos; indexP++) {
        fileScanInfo parentScanInfo = parentFileScanInfos.at(indexP);

        if (parentScanInfo.suffix != "dir") {
            continue;
        }

        // scan parent dir in depth
        qint64 parentDirSize = 0;
        QVector <fileScanInfo> scanInfos = util::scanDirectory(parentScanInfo.absoluteFilePath, scanDir, true, 2);
        qint32 nbrFileScanInfos = scanInfos.length();

        // accumulate size of dir/file in parent dir
        for (qint32 indexS1 = 0; indexS1 < nbrFileScanInfos; indexS1++) {
            fileScanInfo scanInfo = scanInfos.at(indexS1);

            parentDirSize += scanInfo.size;
        }

        // add parent dir info
        parentScanInfo.dirType = "base";
        parentScanInfo.size = parentDirSize;
        parentScanInfo.sizeFormatted = util::formatBytes(parentScanInfo.size);

        fileScanInfos.append(parentScanInfo);

        // add sub dir/file info
        for (qint32 indexS2 = 0; indexS2 < nbrFileScanInfos; indexS2++) {
            fileScanInfo scanInfo = scanInfos.at(indexS2);

            fileScanInfos.append(scanInfo);
        }

        // allow gui to be somewhat responsive
        QCoreApplication::processEvents(QEventLoop::AllEvents, 4000);
    }

    return fileScanInfos;
}
