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

#include "creator.h"

#include "namer.h"

#include "parsercreatordom.h"
#include "writercreator.h"

#include <libkode/code.h>
#include <libkode/printer.h>
#include <libkode/typedef.h>
#include <libkode/statemachine.h>

#include <QDebug>

#include <QFile>
#include <QTextStream>
#include <qdom.h>
#include <QRegExp>
#include <QMap>
#include <QList>

#include <iostream>

Creator::ClassFlags::ClassFlags( const Schema::Element &element )
{
  m_hasId = element.hasRelation( "id" );
  m_hasUpdatedTimestamp = element.hasRelation( "updated_at" );
}

bool Creator::ClassFlags::hasUpdatedTimestamp() const
{
  return m_hasUpdatedTimestamp;
}

bool Creator::ClassFlags::hasId() const
{
  return m_hasId;
}


Creator::Creator( const Schema::Document &document, XmlParserType p )
  : mDocument( document ), mXmlParserType( p )
{
  setExternalClassNames();
}

// FIXME: Handle creation options via flags enum.

void Creator::setVerbose( bool verbose ) 
{
  mVerbose = verbose;
}

void Creator::setUseKde( bool useKde )
{
  mUseKde = useKde;
}

bool Creator::useKde() const
{
  return mUseKde;
}

void Creator::setCreateCrudFunctions( bool enabled )
{
  mCreateCrudFunctions = enabled;
}

void Creator::setLicense( const KODE::License &l )
{
  mFile.setLicense( l );
}

void Creator::setExportDeclaration( const QString &name )
{
  mExportDeclaration = name;
}

void Creator::setExternalClassPrefix( const QString &prefix )
{
  mExternalClassPrefix = prefix;

  setExternalClassNames();
}

void Creator::setExternalClassNames()
{
  mParserClass.setName( mExternalClassPrefix + "Parser" );
  mWriterClass.setName( mExternalClassPrefix + "Writer" );
}

KODE::File &Creator::file()
{
  return mFile;
}

void Creator::createProperty(KODE::Class &c,
                              const ClassDescription &d, const QString &type,
                              const QString &name,
                              ClassProperty::MethodGeneration generateMethods)
{
  if ( type.startsWith( "Q" ) ) {
    c.addHeaderInclude( type );
  }

  bool propertyIsPointer = (mPointerBasedAccessors
                         && !type.endsWith("Enum")
                         && !type.endsWith("List")
                         && !ClassProperty::isBasicType(type));
  QString memberType = type;
  if (propertyIsPointer)
    memberType.append("*");
  QString memberVariableName = name;
  if (name.endsWith("_set")) {
    memberVariableName = memberVariableName.left(name.length() - 4);
    memberVariableName = Namer::getClassName(memberVariableName) + "_set";
  } else {
    memberVariableName = Namer::getClassName(name);
  }
  KODE::MemberVariable v(memberVariableName, memberType);
  c.addMemberVariable( v );

  if (generateMethods & ClassProperty::GenerateMutator) {
    KODE::Function mutator( Namer::getMutator( name ), "void" );
    if ( type == "int" || type == "double" ) {
      mutator.addArgument( "const " + type + " v" );
      mutator.addBodyLine(v.name() + "_set = true;");
    } else {
      if (propertyIsPointer)
        mutator.addArgument( type + " *v" );
      else
        mutator.addArgument( "const " + type + " &v" );
    }
    mutator.addBodyLine( v.name() + " = v;" );
    if ( mCreateCrudFunctions ) {
      if ( name != "UpdatedAt" && name != "CreatedAt" ) {
        if ( d.hasProperty( "UpdatedAt" ) ) {
          mutator.addBodyLine( "setUpdatedAt( QDateTime::currentDateTime() );" );
        }
      }
    }
    c.addFunction( mutator );
  }

  if (generateMethods & ClassProperty::GenerateAccessor) {
    QString accessorType = type;
    if (propertyIsPointer || (mPointerBasedAccessors && type.endsWith("List")))
      accessorType.append("*");

    KODE::Function accessor( Namer::getAccessor( name ), accessorType );
    if (!propertyIsPointer
        && !(mPointerBasedAccessors && type.endsWith("List")))
      accessor.setConst( true );

    if ( type.right(4) == "Enum" ) {
      accessor.setReturnType( c.name() + "::" + type );
    }

    if (mPointerBasedAccessors && type.endsWith("List"))
      accessor.addBodyLine( "return &" + v.name() + ';' );
    else
      accessor.addBodyLine( "return " + v.name() + ';' );
    c.addFunction( accessor );
  }
}

void Creator::createCrudFunctions( KODE::Class &c, const QString &type )
{
  if ( !c.hasEnum( "Flags" ) ) {
    QStringList enumValues;
    enumValues << "None" << "AutoCreate";
    KODE::Enum flags( "Flags", enumValues );
    c.addEnum( flags );
  }

  KODE::Function finder( "find" + type, type );

  finder.addArgument( "const QString &id" );
  finder.addArgument( KODE::Function::Argument( "Flags flags", "Flags_None" ) );

  QString listMember = "m" + type + "List";

  KODE::Code code;
  code += "foreach( " + type + " v, " + listMember + " ) {";
  code += "  if ( v.id() == id ) return v;";
  code += "}";
  code += type + " v;";
  code += "if ( flags == Flags_AutoCreate ) {";
  code += "  v.setId( id );";
  code += "}";
  code += "return v;";

  finder.setBody( code );

  c.addFunction( finder );

  KODE::Function inserter( "insert", "bool" );

  code.clear();

  inserter.addArgument( "const " + type + " &v" );

  code += "int i = 0;";
  code += "for( ; i < " + listMember + ".size(); ++i ) {";
  code += "  if ( " + listMember + "[i].id() == v.id() ) {";
  code += "    " + listMember + "[i] = v;";
  code += "    return true;";
  code += "  }";
  code += "}";
  code += "if ( i == " + listMember + ".size() ) {";
  code += "  add" + type + "( v );";
  code += "}";
  code += "return true;";

  inserter.setBody( code );

  c.addFunction( inserter );

  KODE::Function remover( "remove", "bool" );

  remover.addArgument( "const " + type + " &v" );

  code.clear();

  code += type + "::List::Iterator it;";
  code += "for( it = " + listMember + ".begin(); it != " + listMember +
          ".end(); ++it ) {";
  code += "  if ( (*it).id() == v.id() ) break;";
  code += "}";
  code += "if ( it != " + listMember + ".end() ) {";
  code += "  " + listMember + ".erase( it );";
  code += "}";
  code += "return true;";

  remover.setBody( code );

  c.addFunction( remover );
}


ClassDescription Creator::createClassDescription(const Schema::Element &element)
{
  ClassDescription description( Namer::getClassName( element.name() ) );

  foreach( Schema::Relation r, element.attributeRelations() ) {
    Schema::Attribute a = mDocument.attribute( r, element.name() );
    if ( a.enumerationValues().count() ) {
      if (!description.hasEnum(a.name())) {
        description.addEnum(KODE::Enum(Namer::getClassName( a.name() ) + "Enum", a.enumerationValues()));
      }
      description.addProperty( Namer::getClassName( a.name() ) + "Enum",
                               Namer::getClassName( a.name() ) );
    } else {
      description.addProperty( typeName( a.type() ),
                               Namer::getClassName( a.name() ) );
      if (mPointerBasedAccessors
          && (
             a.type() == Schema::Element::Integer
          || a.type() == Schema::Element::Decimal
            )) {
        description.addProperty("bool",
                                Namer::getClassName(a.name()) + "_set",
                                ClassProperty::GenerateAccessor);
      }
    }
  }

  if ( element.text() ) {
    description.addProperty( typeName( element.type() ), "Value" );
  }

  foreach( Schema::Relation r, element.elementRelations() ) {
    Schema::Element targetElement = mDocument.element( r );

    QString targetClassName = Namer::getClassName( targetElement.name() );

    if ( targetElement.text() && !targetElement.hasAttributeRelations() &&
         !r.isList() ) {
      if ( mVerbose ) {
        qDebug() << "  FLATTEN";
      }
      if ( targetElement.type() == Schema::Element::Integer ) {
        description.addProperty( "int", targetClassName );
      } else if ( targetElement.type() == Schema::Element::Decimal ) {
        description.addProperty( "double", targetClassName );
      } else if ( targetElement.type() == Schema::Element::Date ) {
        description.addProperty( "QDate", targetClassName );
      } else if ( targetElement.type() == Schema::Element::Boolean ) {
        description.addProperty( "bool", targetClassName );
      } else {
        description.addProperty( "QString", targetClassName );
      }
    } else {
      if ( !mFile.hasClass( targetClassName ) ) {
        createClass( targetElement );
      }

      QString name = KODE::Style::lowerFirst( targetClassName );

      if ( r.isList() ) {
        ClassProperty p( targetClassName, Namer::getClassName( name ) );
        p.setIsList( true );
        
        ClassFlags targetClassFlags( targetElement );
        if ( targetClassFlags.hasId() ) {
          p.setTargetHasId( true );
        }

        description.addProperty( p );
      } else {
        description.addProperty( targetClassName, Namer::getClassName( name ) );
      }
    }
  }
  
  return description;
}

void Creator::createClass( const Schema::Element &element )
{
  QString className = Namer::getClassName( element.name() );
  if ( mVerbose ) {
    qDebug() <<"Creator::createClass()" << element.identifier() << className;
    foreach( Schema::Relation r, element.elementRelations() ) {
      qDebug() << "  SUBELEMENTS" << r.target();
    }
  }

  if ( mProcessedClasses.contains( className ) ) {
    if ( mVerbose ) {
      qDebug() << "  ALREADY DONE";
    }
    return;
  }

  mProcessedClasses.append( className );

  KODE::Class c( className );
  ClassDescription description = createClassDescription(element);

  if ( !mExportDeclaration.isEmpty() ) {
    c.setExportDeclaration( mExportDeclaration );
  }

  bool hasCreatedAt = description.hasProperty( "CreatedAt" );
  bool hasUpdatedAt = description.hasProperty( "UpdatedAt" );

  KODE::Function constructor( className, "" );
  bool addConstructor = false;
  KODE::Code constructorCode;
  if ( mCreateCrudFunctions ) {
    if ( hasCreatedAt || hasUpdatedAt ) {
      constructorCode += "QDateTime now = QDateTime::currentDateTime();";
      if ( hasCreatedAt ) {
        constructorCode += "setCreatedAt( now );";
      }
      if ( hasUpdatedAt ) {
        constructorCode += "setUpdatedAt( now );";
      }
      addConstructor = true;
    }

    if ( description.hasProperty( "Id" ) ) {
      KODE::Function isValid( "isValid", "bool" );
      isValid.setConst( true );
      KODE::Code code;
      code += "return !mId.isEmpty();";
      isValid.setBody( code );
      c.addFunction( isValid );
    }
  }

  bool classIsPointer = (mPointerBasedAccessors
                         && !ClassProperty::isBasicType(c.name())
                         && !c.name().endsWith("Enum"));
  if (classIsPointer) {
    // add constructor and desctructor if pointer based acces is present
    KODE::Function constructor(c.name());
    foreach(ClassProperty p, description.properties()) {
      KODE::MemberVariable v(Namer::getClassName( p.name() ), p.type());
      if (mPointerBasedAccessors
          && !p.isBasicType()
          && !p.isList()
          && !p.type().endsWith("Enum"))  {
        // TODO move them to header if C++11 support will be implemented
        constructor.addInitializer(QString("%1(nullptr)").arg(v.name()));
      }

      // initialize enums to invalid
      if (p.type().endsWith("Enum")) {
        constructor.addInitializer(QString("%1(%2_Invalid)")
                                   .arg(v.name())
                                   .arg(p.type().left(p.type().length() - 4)));
      }

      // TODO move them to header if C++11 support will be implemented
      if (p.type() == "int")
        constructor.addInitializer(QString("%1(0)").arg(v.name()));
      if (p.type() == "double")
        constructor.addInitializer(QString("%1(0.0)").arg(v.name()));
      if (p.type() == "bool") {
        if (p.name().endsWith("_set")) {
          KODE::MemberVariable vs(p.name(), p.type());
          constructor.addInitializer(QString("%1(false)").arg(vs.name()));
        } else {
          constructor.addInitializer(QString("%1(false)").arg(v.name()));
        }
      }
    }

    constructor.setBody( constructorCode );
    c.addFunction(constructor);

    KODE::Function destructor("~" + c.name());
    c.addFunction(destructor);
  } else {
    if (addConstructor) {
      constructor.setBody( constructorCode );
      c.addFunction(constructor);
    }
  }

  foreach( ClassProperty p, description.properties() ) {
    if ( p.isList() ) {
      registerListTypedef( p.type() );

      c.addHeaderInclude( "QList" );
      QString listName = p.name() + "List";

      KODE::Function adder( "add" + p.type(), "void" );
      if (mPointerBasedAccessors && !p.isBasicType()) {
        adder.addArgument( p.type() + "* v" );
      } else {
        adder.addArgument( "const " + p.type() + " &v" );
      }

      KODE::Code code;
      code += 'm' + KODE::Style::upperFirst( listName ) + ".append( v );";

      adder.setBody( code );

      c.addFunction( adder );

      createProperty(c, description, p.type() + "::List", listName, p.accessorGeneration());

      if ( mCreateCrudFunctions && p.targetHasId() ) {
        createCrudFunctions( c, p.type() );
      }
    } else {
      createProperty(c, description, p.type(), p.name(), p.accessorGeneration());
    }
  }

  foreach(KODE::Enum e, description.enums() ) {
    c.addEnum(e);
  }

  createElementParser( c, element );
  
  WriterCreator writerCreator( mFile, mDocument, mDtd, this );
  writerCreator.createElementWriter( c, element );

  mFile.insertClass( c );
}

void Creator::createElementParser( KODE::Class &c, const Schema::Element &e )
{
  switch (mXmlParserType) {
  case XmlParserDom:
  case XmlParserDomExternal: {
    ParserCreator * parserCreator = new ParserCreatorDom( this );
    parserCreator->createElementParser(c, e);
    delete parserCreator;
  } break;
  }
}

void Creator::registerListTypedef( const QString &type )
{
  if ( !mListTypedefs.contains( type ) ) mListTypedefs.append( type );
}

void Creator::createListTypedefs()
{
  QStringList::ConstIterator it;
  for( it = mListTypedefs.constBegin(); it != mListTypedefs.constEnd(); ++it ) {
    KODE::Class c = mFile.findClass( *it );
    if ( !c.isValid() ) continue;

    if (mPointerBasedAccessors)
      c.addTypedef( KODE::Typedef( "QList<" + *it + "*>", "List" ) );
    else
      c.addTypedef( KODE::Typedef( "QList<" + *it + '>', "List" ) );
    mFile.insertClass( c );
  }
}

void Creator::setDtd( const QString &dtd )
{
  mDtd = dtd;
}

void Creator::createFileWriter( const Schema::Element &element )
{
  WriterCreator writerCreator( mFile, mDocument, mDtd, this );
  writerCreator.createFileWriter( Namer::getClassName( element.name() ),
                                  errorStream() );
}

void Creator::createFileParser( const Schema::Element &element )
{
  ParserCreator *parserCreator = 0;

  switch ( mXmlParserType ) {
  case XmlParserDom:
  case XmlParserDomExternal:
    parserCreator = new ParserCreatorDom( this );
    break;
  }

  parserCreator->createFileParser( element );
  parserCreator->createStringParser( element );

  delete parserCreator;
}

void Creator::printFiles( KODE::Printer &printer )
{
  if ( externalParser() ) {
    KODE::File parserFile( file() );
    parserFile.setFilename( mBaseName + "_parser" );

    parserFile.clearCode();

    mParserClass.addHeaderInclude( file().filenameHeader() );
    parserFile.insertClass( mParserClass );

    if ( mVerbose ) {
      qDebug() <<"Print external parser header" << parserFile.filenameHeader();
    }
    printer.printHeader( parserFile );
    if ( mVerbose ) {
      qDebug() <<"Print external parser implementation"
              << parserFile.filenameImplementation();
    }
    printer.printImplementation( parserFile );
  }

  if ( mVerbose ) {
    qDebug() <<"Print header" << file().filenameHeader();
  }
  printer.printHeader( file() );

  if ( mVerbose ) {
    qDebug() <<"Print implementation" << file().filenameImplementation();
  }
  printer.printImplementation( file() );

}

bool Creator::externalParser() const
{
  return mXmlParserType == XmlParserDomExternal;
}

bool Creator::externalWriter() const
{
  return false;
}

const Schema::Document &Creator::document() const
{
  return mDocument;
}

void Creator::setParserClass( const KODE::Class &c )
{
  mParserClass = c;
}

KODE::Class &Creator::parserClass()
{
  return mParserClass;
}

QString Creator::errorStream() const
{
  if ( useKde() ) {
    return "kError()";
  } else {
    return "qCritical()";
  }
}

QString Creator::debugStream() const
{
  if ( useKde() ) {
    return "qDebug()";
  } else {
    return "qDebug()";
  }
}

void Creator::create()
{
  Schema::Element startElement = mDocument.startElement();
  setExternalClassPrefix( KODE::Style::upperFirst( startElement.name() ) );
  createFileParser( startElement );
  //  setDtd( schemaFilename.replace( "rng", "dtd" ) );
  createFileWriter( startElement );

  createListTypedefs();
}

void Creator::setFilename( const QString& baseName )
{
  mFile.setFilename( baseName );
  mBaseName = baseName;
}

QString Creator::typeName( Schema::Node::Type type )
{
  if (type == Schema::Element::DateTime)
    return "QDateTime";
  else if (type == Schema::Element::Date)
    return "QDate";
  else if (type == Schema::Element::Integer)
    return "int";
  else if (type == Schema::Element::Decimal)
    return "double";
  else if (type == Schema::Element::Boolean)
    return "bool";
  return "QString";
}

bool Creator::pointerBasedAccessors() const
{
  return mPointerBasedAccessors;
}

void Creator::setPointerBasedAccessors(bool pointerBasedAccessors)
{
  mPointerBasedAccessors = pointerBasedAccessors;
}

ParserCreator::ParserCreator( Creator *c )
  : mCreator( c )
{
}

ParserCreator::~ParserCreator()
{
}

Creator *ParserCreator::creator() const
{
  return mCreator;
}
