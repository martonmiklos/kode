/*
    This file is part of KDE.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "writercreator.h"

#include "namer.h"
#include "style.h"

#include <QDebug>

WriterCreator::WriterCreator(KODE::File &file, Schema::Document &document,
  const QString &dtd , Creator *creator)
  : mFile( file ), mDocument( document ), mDtd( dtd ), mCreator(creator)
{
}

void WriterCreator::createFileWriter( const QString &className, const QString &errorStream )
{
  KODE::Class c = mFile.findClass( className );

  c.addHeaderInclude( "QXmlStreamWriter" );
  c.addInclude( "QtDebug" );
  c.addInclude( "QFile" );

  KODE::Function writer( "writeFile", "bool" );
  writer.setConst( true );

  writer.addArgument( "const QString &filename" );

  KODE::Code code;

  code += "QFile file( filename );";
  code += "if ( !file.open( QIODevice::WriteOnly ) ) {";
  code += "  " + errorStream + " << \"Unable to open file '\" << filename << \"'\";";
  code += "  return false;";
  code += '}';
  code += "";

  code += "QXmlStreamWriter xml( &file );";
  code += "xml.setAutoFormatting( true );";
  code += "xml.setAutoFormattingIndent( 2 );";

  code += "xml.writeStartDocument( \"1.0\" );";
  if ( !mDtd.isEmpty() ) {
    code += "xml.writeDTD( \\\"" + mDtd + "\\\" );";
  }

  code += "writeElement( xml );";
  code += "xml.writeEndDocument();";
  code += "file.close();";
  code += "";
  code += "return true;";

  writer.setBody( code );

  c.addFunction( writer );

  mFile.insertClass( c );
}

void WriterCreator::createElementWriter( KODE::Class &c,
  const Schema::Element &element )
{
  KODE::Function writer( "writeElement", "void" );
  writer.setConst( true );

  writer.addArgument( "QXmlStreamWriter &xml" );

  KODE::Code code;

  QString tag = element.name();

  if ( element.isEmpty() ) {
    if (element.attributeRelations().isEmpty())
      code.addLine("xml.writeEmptyElement( \"" + tag + "\" );");
    else {
      code.addLine("xml.writeStartElement( \"" + tag + "\" );");
      createAttributeWriter( element, code );
      code.addLine("xml.writeEndElement();");
    }
  } else if ( element.text() ) {
    if ( element.type() == Schema::Element::Date ) {
      code += "if ( value().isValid() ) {";
    } else {
      code += "if ( !value().isEmpty() ) {";
    }
    code.indent();
    code += "xml.writeStartElement( \"" + tag + "\" );";
    
    createAttributeWriter( element, code );

    QString data = dataToStringConverter( "value()", element.type() );
    code += "xml.writeCharacters( " + data + " );";
    code += "xml.writeEndElement();";
    code.unindent();
    code += "}";
  } else {
    bool pureList = true;
    if ( !element.attributeRelations().isEmpty() ) {
      pureList = false;
    } else {
      if ( element.elementRelations().isEmpty() ) {
        pureList = false;
      } else {
        foreach( Schema::Relation r, element.elementRelations() ) {
          if ( !r.isList() ) {
            pureList = false;
            break;
          }
        }
      }
    }

    if ( pureList ) {
      QStringList conditions;
      foreach( Schema::Relation r, element.elementRelations() ) {
        if ( r.isList() ) {
          conditions.append( "!" +
            Namer::getListMemberVariable(r.target()) + ".isEmpty()" );
        }
      }
      code += "if ( " + conditions.join( " || " ) + " ) {";
      code.indent();
    }
    
    code += "xml.writeStartElement( \"" + tag + "\" );";

    createAttributeWriter( element, code );

    foreach( Schema::Relation r, element.elementRelations() ) {
      QString type = Namer::getClassName( r.target() );
      QString memberAccessor = ".";
      if (mCreator->pointerBasedAccessors()
          && !ClassProperty::isBasicType(type)
          && !type.endsWith("Enum")) {
        type.append("*");
        memberAccessor = QStringLiteral("->");
      }

      if ( r.isList() ) {
        code += "foreach( " + type + " e, " +
                Namer::getListMemberVariable(r.target()) + " ) {";
        code.indent();
        if (mCreator->pointerBasedAccessors()) {
          code.addLine("if (e)");
          code.indent();
          code += "e->writeElement( xml );";
          code.unindent();
        } else {
          code += "e.writeElement( xml );";
        }
        code.unindent();
        code += '}';
      } else {
        Schema::Element e = mDocument.element( r );
        QString accessor = Namer::getAccessor( e.name() ) + "()";
        QString data = dataToStringConverter( accessor, e.type() );
        if ( e.text() && !e.hasAttributeRelations() ) {
          if ( e.type() == Schema::Element::String ) {
            code += "if ( !" + data + ".isEmpty() ) {";
            code.indent();
          }
          code += "xml.writeTextElement(  \"" + e.name() + "\", " + data +
            " );";
          if ( e.type() == Schema::Element::String ) {
            code.unindent();
            code += "}";
          }
        } else {
          if (memberAccessor == "->") {
            code.addLine("if (" +  Namer::getMemberVariable( r.target() ) + ")");
            code.indent();
          }
          code += Namer::getMemberVariable( r.target() ) + memberAccessor + "writeElement( xml );";
          if (memberAccessor == "->") {
            code.unindent();
          }
        }
      }
    }
    code += "xml.writeEndElement();";
    
    if ( pureList ) {
      code.unindent();
      code += "}";
    }
  }

  writer.setBody( code );

  c.addFunction( writer );
}

// FIXME: Collect in class with other type specific functions from parsercreator
// and creator
QString WriterCreator::dataToStringConverter( const QString &data,
  Schema::Node::Type type )
{
  QString converter;

  if ( type == Schema::Element::Integer || type == Schema::Element::Decimal ) {
    converter = "QString::number( " + data + " )";
  } else if ( type == Schema::Element::Boolean ) {
    converter = data + " ? \"true\" : \"false\"";
  } else if ( type == Schema::Element::Date ) {
    converter = data + ".toString( \"yyyyMMdd\" )";
  } else if ( type == Schema::Element::DateTime ) {
    converter = data + ".toString( \"yyyyMMddThhmmssZ\" )";
  } else {
    converter = data;
  }
  
  return converter;
}

void WriterCreator::createAttributeWriter( const Schema::Element &element, KODE::Code & code )
{
  foreach( Schema::Relation r, element.attributeRelations() ) {
    Schema::Attribute a = mDocument.attribute(r, element.name());

    QString variableName = Namer::getMemberVariable(Namer::getClassName(a.name()));
    if ( a.type() != Schema::Node::Enumeration ) {
      if (a.type() == Schema::Node::String) {
        code.addLine("if (!" + variableName + ".isEmpty())");
        code.indent();
      }

      if (a.type() == Schema::Node::Integer
          || a.type() == Schema::Node::Decimal) {
        code.addLine("if (" + variableName + "_set)");
        code.indent();
      }

      code.addLine("xml.writeAttribute(\"" + a.name() + "\", " +
          dataToStringConverter( variableName, a.type() ) + " );");
      if (a.type() == Schema::Node::String
          || a.type() == Schema::Node::Integer
          || a.type() == Schema::Node::Decimal) {
        code.unindent();
      }
    } else if ( a.type() == Schema::Node::Enumeration ) {
      code.addLine("if (" + Namer::getMemberVariable( Namer::getClassName(a.name()) ) + " != "
                   + Namer::getClassName(a.name())
                   + "_Invalid)");
      code.indent();
      code.addLine("xml.writeAttribute(\"" + a.name() + "\", " +
                   KODE::Style::lowerFirst(Namer::getClassName(a.name())) + "EnumToString(" +
                   Namer::getMemberVariable(Namer::getClassName(a.name())) + "));");
      code.unindent();
    }
  }
}
