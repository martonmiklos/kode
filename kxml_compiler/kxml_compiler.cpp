/*
    This file is part of KDE.

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

#include "parserrelaxng.h"
#include "parserxsd.h"
#include "parserxml.h"
#include "creator.h"
#include "schema.h"

#include <libkode/code.h>
#include <libkode/printer.h>
#include <libkode/typedef.h>

/*#include <kaboutdata.h>
#include <qDebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QRegExp>
#include <QTextStream>

#include <iostream>

using namespace KXML;

int main( int argc, char **argv )
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("kxml_compiler");
  QCoreApplication::setApplicationVersion("0.1");

  QCommandLineParser parser;

  parser.setApplicationDescription("KDE xml compiler");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption dirOption(
              QStringList() << "d" << "directory",
              QCoreApplication::translate("main", "Directory to generate files in"),
              ".");
  parser.addOption(dirOption);

  QCommandLineOption verboseOption(
              "verbose",
              QCoreApplication::translate("main", "Generate debug output"));
  parser.addOption(verboseOption);

  QCommandLineOption schemaOption(
              "schema",
              QCoreApplication::translate("main", "Schema of XML file"));
  parser.addOption(schemaOption);

  QCommandLineOption extParserOption(
              "external-parser",
              QCoreApplication::translate("main", "Generate parser in separate source file"));
  parser.addOption(extParserOption);

  QCommandLineOption xsdOption(
              "xsd",
              QCoreApplication::translate("main", "Schema is XML Schema"));
  parser.addOption(xsdOption);

  QCommandLineOption rngOption(
              "rng",
              QCoreApplication::translate("main", "Schema is RelaxNG"));
  parser.addOption(rngOption);

  QCommandLineOption xmlOption(
              "xml",
              QCoreApplication::translate("main", "Schema is example XML"));
  parser.addOption(xmlOption);

  QCommandLineOption useKdeOption(
              "use-kde",
              QCoreApplication::translate("main", "Use KDE classes"));
  parser.addOption(useKdeOption);

  QCommandLineOption licenseOption(
              "license",
              QCoreApplication::translate("main", "License of generated files. Possible values: gpl, bsd, lgpl"));
  parser.addOption(licenseOption);

  QCommandLineOption namespaceOption(
              "namespace",
              QCoreApplication::translate("main", "Namespace for generated classes"));
  parser.addOption(namespaceOption);

  QCommandLineOption exportOption(
              "export",
              QCoreApplication::translate("main", "Export declaration for generated classes"));
  parser.addOption(exportOption);

  QCommandLineOption createCRUDFunctionsOption(
              "create-crud-functions",
              QCoreApplication::translate("main", "Create functions for dealing with data suitable for CRUD model"));
  parser.addOption(createCRUDFunctionsOption);

  QCommandLineOption dontCreateWriteFunctionsOption(
              "dont-create-write-functions",
              QCoreApplication::translate("main", "Do not create XML generating methods to the generated classes (useful for applications require XML parse only feature)"));
  parser.addOption(dontCreateWriteFunctionsOption);

  QCommandLineOption dontCreateParseFunctionsOption(
              "dont-create-parse-functions",
              QCoreApplication::translate("main", "Do not create XML parsing methods to the generated classes (useful for applications require XML write only feature)"));
  parser.addOption(dontCreateParseFunctionsOption);

  if (!parser.parse(QCoreApplication::arguments())) {
      //return -1;
  }

  QString schemaFilename = parser.positionalArguments().at(0);


  QFileInfo fi(parser.positionalArguments().at(0));
  QString baseName = fi.baseName();
  baseName.remove( "_" );


  QFile schemaFile( schemaFilename );
  if ( !schemaFile.open( QIODevice::ReadOnly ) ) {
    qDebug() <<"Unable to open '" << schemaFilename <<"'";
    return 1;
  }

  if ( parser.isSet(verboseOption) ) {
    qDebug() <<"Begin parsing";
  }

  Schema::Document schemaDocument;

  fi = QFileInfo( schemaFile );
  if ( parser.isSet( xsdOption ) || fi.suffix() == "xsd" ) {
    RNG::ParserXsd p;
    p.setVerbose( parser.isSet(verboseOption) );
    schemaDocument = p.parse( schemaFile );

    if ( schemaDocument.isEmpty() ) {
      qDebug() <<"Error parsing schema '" << schemaFilename <<"'";
      return 1;
    }
  } else if ( parser.isSet( rngOption ) || fi.suffix() == "rng" ) {
    QString errorMsg;
    int errorLine, errorCol;
    QDomDocument doc;
    if ( !doc.setContent( &schemaFile, false, &errorMsg, &errorLine, &errorCol ) ) {
      qDebug() << errorMsg <<" at" << errorLine <<"," << errorCol;
      return 1;
    }

    RNG::ParserRelaxng p;
    p.setVerbose( parser.isSet(verboseOption) );
    RNG::Element *start = p.parse( doc.documentElement() );
    if ( !start ) {
      qDebug() <<"Could not find start element";
      return 1;
    }

    if ( parser.isSet(verboseOption) ) {
      p.dumpDefinitionMap();
    }

  //  return 0;

    p.substituteReferences( start );

  #if 1
    if ( parser.isSet(verboseOption) ) {
      std::cout << "--- TREE:" << std::endl;
      p.dumpTree( start );
    }
  #endif

    schemaDocument = p.convertToSchema( start );
  } else if ( parser.isSet( "xml" ) || fi.suffix() == "xml" ) {
    ParserXml schemaParser;
    schemaParser.setVerbose( parser.isSet(verboseOption) );
    schemaDocument = schemaParser.parse( schemaFile );
  } else {
    qDebug() <<"Unable to determine schema type.";
    return 1;
  }

  if ( parser.isSet(verboseOption) ) {
    std::cout << "--- SCHEMA:" << std::endl;
    schemaDocument.dump();

    qDebug() <<"Begin creating code";
  }

  Creator::XmlParserType pt;
  if ( parser.isSet( "external-parser" ) ) {
    pt = Creator::XmlParserDomExternal;
  } else {
    pt = Creator::XmlParserDom;
  }

  Creator c( schemaDocument, pt );
  c.setVerbose( parser.isSet(verboseOption) );
  c.setUseKde( parser.isSet( "use-kde" ) );
  c.setCreateCrudFunctions( parser.isSet( "create-crud-functions" ) );
  c.setCreateWriteFunctions( !parser.isSet( "dont-create-write-functions" ) );
  c.setCreateParseFunctions( !parser.isSet( "dont-create-parse-functions" ) );
  if ( parser.isSet( namespaceOption ) ) {
    c.file().setNameSpace( parser.value(namespaceOption ) );
  }
  if ( parser.isSet(exportOption ) ) {
    c.setExportDeclaration( parser.value(exportOption) );
  }

  if ( parser.isSet(licenseOption) ) {
    QString l = parser.value(licenseOption);
    if ( l == "gpl" ) {
      c.setLicense( KODE::License( KODE::License::GPL ) );
    } else if ( l == "bsd" ) {
      c.setLicense( KODE::License( KODE::License::BSD ) );
    } else if ( l == "lgpl" ) {
      c.setLicense( KODE::License( KODE::License::LGPL ) );
    }
  }

  if ( parser.isSet(verboseOption) ) {
    qDebug() <<"Create classes";
  }

  foreach( Schema::Element e, schemaDocument.usedElements() ) {
    if ( !e.text() ) {
      c.createClass( e );
    }
  }

  if ( parser.isSet(verboseOption) ) {
    qDebug() <<"Create parser";
  }
  c.create();

  if ( parser.isSet(verboseOption) ) {
    qDebug() <<"Begin printing code";
  }
  c.setFilename( baseName );

  KODE::Printer printer;
  printer.setCreationWarning( true );
  printer.setGenerator( QCoreApplication::applicationName() );
  printer.setOutputDirectory( parser.value(dirOption) );
  printer.setSourceFile( parser.positionalArguments().at(0) );

  c.printFiles( printer );

  if ( parser.isSet(verboseOption) ) {
    qDebug() <<"Finished.";
  }
}
