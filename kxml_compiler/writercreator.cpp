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
#include <QRegularExpression>

WriterCreator::WriterCreator( KODE::File &file, Schema::Document &document,
  const QString &dtd )
  : mFile( file ), mDocument( document ), mDtd( dtd )
{
}

void WriterCreator::createFileWriter( const QString &className, const QString &errorStream )
{
  KODE::Class c = mFile.findClass( className );

  c.addHeaderInclude( "QtXml/QXmlStreamWriter" );
  c.addInclude( "QtCore/QtDebug" );
  c.addInclude( "QtCore/QFile" );

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

void WriterCreator::addWriteStartElement(QString tag, const QString & targetNameSpace, KODE::Code & code, bool printNameSpace)
{
  code += "xml.writeStartElement( \"" + tag + "\" );";
  if ( printNameSpace ) {
    code += "xml.writeAttribute( \"xmlns\", \"" + targetNameSpace + "\" );";
  }
}

void WriterCreator::createElementWriter(
    KODE::Class &c,
    const Schema::Element &element,
    const QString targetNameSpace )
{
  KODE::Function writer( "writeElement", "void" );
  writer.setConst( true );

  writer.addArgument( "QXmlStreamWriter &xml" );

  KODE::Code code;
  QString tag = element.name();
  code += "if ( mElementIsOptional && !mValueHadBeenSet )";
  code.indent();
  code += "return;";
  code.unindent();

  switch (element.type()) {
  case Schema::Element::None:
    code += "xml.writeEmptyElement( \"" + tag + "\" );";
    break;
  case Schema::Element::String:
  case Schema::Element::NormalizedString:
  case Schema::Element::Token:
  case Schema::Element::Int:
  case Schema::Element::Integer:
  case Schema::Element::Short:
  case Schema::Element::Decimal:
  case Schema::Element::Byte:
  case Schema::Element::Date:
  case Schema::Element::DateTime:
  case Schema::Element::Boolean:
    if ( element.type() == Schema::Element::Date ||
         element.type() == Schema::Element::DateTime ) {
      code += "if ( mValue.isValid() ) {";
      code.indent();
    } else if ( !element.isNumeric() ){
      code += "if ( mValue.isEmpty() ) {";
      code.indent();
    }
    addWriteStartElement( tag, targetNameSpace, code, element.isRootElement() );
    code += createAttributeWriter( element );

    code += "xml.writeCharacters( " + dataToStringConverter( "mValue", element ) + " );";
    code += "xml.writeEndElement();";
    if ( !element.isNumeric() ){
      code.unindent();
      code += "}";
    }
    break;
  case Schema::Element::Enumeration:
    addWriteStartElement( tag, targetNameSpace, code, element.isRootElement() );

    code += createAttributeWriter( element );
    code += "xml.writeCharacters( " +
            KODE::Style::lowerFirst( Namer::getClassName( element.name()))  +
            "_EnumToString( mValue ) );";
    code += "xml.writeEndElement();";
    break;
  case Schema::Element::ComplexType:
    bool pureList = true;
    bool baseType = true;
    if ( !element.attributeRelations().isEmpty() ) {
      pureList = false;
    } else {
      if ( element.elementRelations().isEmpty() ) {
        pureList = false;
      } else {
        baseType = false;
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
            Namer::getMemberVariable( r.target() ) + "List.isEmpty()" );
        }
      }
      code += "if ( " + conditions.join( " || " ) + " ) {";
      code.indent();
    }
    
    addWriteStartElement( tag, targetNameSpace, code, element.isRootElement() );
    code += createAttributeWriter( element );

    foreach( Schema::Relation r, element.elementRelations() ) {
      QString type = Namer::getClassName( r.target() );
      if ( r.isList() ) {
        code += "foreach( " + type + " e, " +
          Namer::getMemberVariable( r.target() ) + "List ) {";
        code.indent();
        code += "e.writeElement( xml );";
        code.unindent();
        code += '}';
      } else {
        Schema::Element e = mDocument.element( r );
        QString member = Namer::getMemberVariable( e ) ;
        QString data = dataToStringConverter( member, e );
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
          code += Namer::getMemberVariable( r.target() ) + ".writeElement( xml );";
        }
      }
    }

    if (baseType) {
      switch (element.baseType()) {
      case Schema::Node::String:
        code += "xml.writeCharacters( mStringValue );";
        break;
      // TODO implement for other simple types too!!
      default:
        break;
      }
    }

    code += "xml.writeEndElement();";
    
    if ( pureList ) {
      code.unindent();
      code += "}";
    }
    break;
  }

  writer.setBody( code );

  c.addFunction( writer );
}

// FIXME: Collect in class with other type specific functions from parsercreator
// and creator
QString WriterCreator::dataToStringConverter( const QString &data, const Schema::Node &element )
{
  QString converter;
  switch (element.type()) {
  case Schema::Element::Int:
  case Schema::Element::Byte:
  case Schema::Element::Short:
  case Schema::Element::Integer:
    converter = "QString::number( " + data + ", 'f', 0)";
    break;
  case Schema::Element::Decimal:
    converter = QString("QString::number( %1, 'f', %2)").arg(data).arg(element.fractionalDigits());
    break;
  case Schema::Element::Date:
    // format: [-]CCYY-MM-DD[Z|(+|-)hh:mm]
    // http://books.xmlschemata.org/relaxng/ch19-77041.html
    converter = data + ".toString( \"yyyy-MM-dd\" )";
    break;
  case Schema::Element::DateTime:
    // format: [-]CCYY-MM-DD[Z|(+|-)hh:mm]
    // http://books.xmlschemata.org/relaxng/ch19-77041.html
    converter = data + ".toString( \"yyyy-MM-dd hh:mm\" )";
    break;
  case Schema::Element::Boolean:
    // Legal values for boolean are true, false, 1 (which indicates true), and 0 (which indicates false).
    converter = data + " ? \"1\" : \"0\"";
    break;
  case Schema::Element::Token:
    /*
     * The token data type also contains characters, but the XML processor will remove line feeds, carriage returns, tabs, leading and trailing spaces, and multiple spaces.
     */
    converter = data;
    converter = converter.remove("\r");
    converter = converter.remove("\n");
    converter = converter.remove("\t");
    converter = converter.trimmed();
    converter = converter.replace(QRegularExpression("\\s\\s+"), QStringLiteral(" "));
    break;
  case Schema::Element::NormalizedString:
    /*
     * The normalizedString data type also contains characters, but the XML processor will remove line feeds, carriage returns, and tab characters.
     */
    converter = data;
    converter = converter.remove("\r");
    converter = converter.remove("\n");
    converter = converter.remove("\t");
    break;
  default:
    converter = data;
    break;
  }
  
  return converter;
}

KODE::Code WriterCreator::createAttributeWriter( const Schema::Element &element )
{
  KODE::Code code;

  foreach( Schema::Relation r, element.attributeRelations() ) {
    Schema::Attribute a = mDocument.attribute( r );
    if (a.isOptional() ) {
      code += QString("if (m%1AttributeHadBeenSet)").arg(Namer::getClassName( a.name() ));
      code.indent();
    }

    QString data = Namer::getMemberVariable(a);
    if ( a.type() != Schema::Node::Enumeration ) {
        code += "xml.writeAttribute( \"" + a.name() + "\", " +
          dataToStringConverter( data, a ) + " );";
    } else if ( a.type() == Schema::Node::Enumeration ) {
       code += "xml.writeAttribute(\"" + a.name() + "\", " +
               a.name() + "EnumToString( m" + a.name() + "() ));";
    }

    if (a.isOptional()) {
      code.unindent();
    }
  }
  
  return code;
}
