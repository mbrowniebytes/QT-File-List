#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <QIcon>

struct fileScanInfo {
    QString absoluteFilePath;
    int depth;
    QString dirType;
    QIcon iconImage;
    QString iconName;
    QString fileName;
    QString lastModified;
    QString relativeDir;
    QString relativeFilePath;
    qint64 size;
    QString sizeFormatted;
    QString suffix;
};

struct suffixIcon {
    QIcon iconImage;
    QString iconName;
};


#endif // STRUCTS_H
