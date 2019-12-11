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

#include "gen_src/kde-features.h"

#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#include <iostream>

static void displayFeature( const Feature &f )
{
  std::cout << "FEATURE: " << f.summary().toLocal8Bit().data() << std::endl;
  foreach( Responsible r, f.responsibleList() ) {
    std::cout << "  RESPONSIBLE: " << r.name().toLocal8Bit().data() << " ("
              << r.email().toLocal8Bit().data() << ")" << std::endl;
  }
  std::cout << "  TARGET: " << f.target().toLocal8Bit().data() << std::endl;
  std::cout << "  STATUS: " << f.status().toLocal8Bit().data() << std::endl;
}

static void displayCategory( Category::List categories )
{
  foreach( Category c, categories ) {
    std::cout << "CATEGORY: " << c.name().toLocal8Bit().data() << std::endl;

    Feature::List features = c.featureList();
    foreach( Feature f, features ) {
      displayFeature( f );
    }

    displayCategory( c.categoryList() );
  }
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  QCoreApplication::setApplicationName("testfeatures");
  QCoreApplication::setOrganizationName("kode");
  QCoreApplication::setApplicationVersion("0.1");

  QCommandLineParser cmdLine;
  cmdLine.setApplicationDescription("Dump XML feature list to stdout");
  cmdLine.addHelpOption();
  cmdLine.addVersionOption();

  QCommandLineOption featureOption(
              "featurelist",
              QCoreApplication::translate("main", "Name of featurelist XML file"));
  cmdLine.addOption(featureOption);

  QCommandLineOption outputOption(
              "output",
              QCoreApplication::translate("main", "Name of output file"));
  cmdLine.addOption(outputOption);

  QString filename = cmdLine.value("featurelist");

  bool ok;
  Features features = Features::parseFile( filename, &ok );

  if ( !ok ) {
    qWarning() <<"Parse error";
  } else {
    Category::List categories = features.categoryList();
    displayCategory( categories );
  }

  if ( cmdLine.isSet( "output" ) ) {
    QString out = cmdLine.value( "output" );
    if ( !features.writeFile( out ) ) {
      qWarning() <<"Write error";
    }
  }
  return 0;
}
