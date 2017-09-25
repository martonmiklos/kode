#ifndef FILECOMPARE_H
#define FILECOMPARE_H

#include <QFile>
#include <QString>

class FileCompare
{
public:
  FileCompare();
  static bool filesIdentical(const QString &f1, const QString &f2);
};

#endif // FILECOMPARE_H
