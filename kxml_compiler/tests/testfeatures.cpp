/*
    This file is part of kdepim.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kde-features.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>


static void displayFeature(const Feature &f)
{
    qWarning() << "FEATURE: " << f.summary();
    for (const auto r : f.responsibleList())
        qWarning() << "  RESPONSIBLE: " << r.name() << " (" << r.email() << ")";

    qWarning() << "  TARGET: " << f.target();
    qWarning() << "  STATUS: " << f.status();
}

static void displayCategory(const Category::List categories)
{
    for (const auto c : categories) {
        qWarning() << "CATEGORY: " << c.name();

        auto features = c.featureList();
        for (const auto f : qAsConst(features))
            displayFeature(f);

        displayCategory(c.categoryList());
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("testfeatures");
    QCoreApplication::setOrganizationName("kode");

    QCommandLineParser cmdLine;
    QCommandLineOption featureListOption(
                "featurelist",
                QCoreApplication::translate("main", "Name of featurelist XML file"),
                QString());
    cmdLine.addOption(featureListOption);

    QCommandLineOption outputFile(
                "output",
                QCoreApplication::translate("main", "Name of output file"),
                QString());

    bool ok;
    Features features = Features::parseFile(cmdLine.value(featureListOption), &ok);

    if (!ok) {
        qDebug() << "Parse error";
    } else {
        Category::List categories = features.categoryList();
        displayCategory(categories);
    }

    if (cmdLine.isSet(outputFile)) {
        QString out = cmdLine.value(outputFile);
        if (!features.writeFile(out)) {
            qDebug() << "Write error";
        }
    }
    return 0;
}
