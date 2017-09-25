#include "filecompare.h"


FileCompare::FileCompare()
{

}


bool FileCompare::filesIdentical(const QString &f1, const QString &f2)
{
    QFile file1(f1);
    QFile file2(f2);
    bool ret = true;

    if (!file1.open(QFile::ReadOnly))
      return false;

    if (!file2.open(QFile::ReadOnly)) {
      file1.close();
      return false;
    }

    if (file1.size() == file2.size()) {
        char c1, c2;
        while (file1.getChar(&c1) && file2.getChar(&c2)) {
            if (c1 != c2) {
                ret = false;
                break;
            }
        }
    } else {
        ret = false;
    }

    file1.close();
    file2.close();
    return ret;
}
