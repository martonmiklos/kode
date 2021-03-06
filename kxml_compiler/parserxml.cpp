/*
    This file is part of KDE.

    Copyright (c) 2009 Cornelius Schumacher <schumacher@kde.org>

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

#include "parserxml.h"

#include <schema/parser.h>

#include <QDebug>

#include <QDebug>
#include <QXmlStreamReader>
#include <math.h>

using namespace KXML;

ParserXml::ParserXml() : mVerbose(false) {}

void ParserXml::setVerbose(bool verbose)
{
    mVerbose = verbose;
}

Schema::Document ParserXml::parse(QFile &file)
{
    if (mVerbose) {
        qDebug() << "----ParserXml::parse() file";
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            Schema::Element element = parseElement(xml);

            mDocument.setStartElement(element);
            mDocument.addElement(element);
        }
    }
    if (xml.hasError()) {
        qDebug() << "XML parsing error in line" << xml.lineNumber() << ": " << xml.errorString();
    }

    return mDocument;
}

Schema::Element ParserXml::parseElement(QXmlStreamReader &xml, bool isArray)
{
    Schema::Element element;

    QString elementName = xml.name().toString();

    //  qDebug() << "PARSE ELEMENT" << elementName;

    element.setIdentifier(elementName);
    element.setName(elementName);

    QXmlStreamAttributes attributes = xml.attributes();

    foreach (QXmlStreamAttribute attribute, attributes) {
        //          qDebug() << "  ATTRIBUTE" << attribute.name();
        Schema::Attribute a;
        a.setType(detectType(attribute.value().toString()));
        a.setIdentifier(attribute.name().toString());
        a.setName(attribute.name().toString());

        Schema::Relation relation(a.identifier());
        //          qDebug() << "  ADD" << a.identifier() << element.identifier();
        element.addAttributeRelation(relation);

        if (!mDocument.hasAttribute(a)) {
            mDocument.addAttribute(a);
        }
    }

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            //      qDebug() << "  ELEMENT" << element.identifier();
            //      qDebug() << "  START ELEMENT" << xml.name();

            bool childIsArray = xml.attributes().value("type") == "array";

            Schema::Element childElement = parseElement(xml, childIsArray);

            if (element.hasElementRelation(childElement)) {
                Schema::Relation &relation = element.elementRelation(childElement);
                relation.setMaxOccurs(Schema::Relation::Unbounded);
            } else {
                Schema::Relation relation(childElement.identifier());
                if (isArray) {
                    relation.setMaxOccurs(Schema::Relation::Unbounded);
                }
                element.addElementRelation(relation);

                if (!mDocument.hasElement(childElement)) {
                    mDocument.addElement(childElement);
                }
            }
        } else if (xml.isEndElement() && xml.name() == elementName) {
            break;
        } else {
            if (xml.isCharacters() && !xml.isWhitespace()) {
                element.setText(true);

                QString text = xml.text().toString();

                element.setType(detectType(text));
            } else {
                element.setType(Schema::Element::ComplexType);
            }
        }
    }

    return element;
}

Schema::Node::Type ParserXml::detectType(const QString &text)
{
    Schema::Node::Type type = Schema::Node::String;

    QRegExp dateR("\\d{8}");
    QRegExp dateTimeR("\\d{8}T\\d{6}Z");

    if (dateR.exactMatch(text)) {
        type = Schema::Node::Date;
    } else if (dateTimeR.exactMatch(text)) {
        type = Schema::Node::DateTime;
    } else {
        bool ok;
        double val = text.toDouble(&ok);
        double intPart = 0;
        if (ok) {
            if (modf(val, &intPart) != 0) {
                type = Schema::Node::Decimal;
            } else {
                type = Schema::Node::Integer;
            }
        }
    }

    return type;
}
