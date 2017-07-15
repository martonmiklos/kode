/* 
    This file is part of KDE Schema Parser

    Copyright (c) 2005 Tobias Koenig <tokoe@kde.org>
                       based on wsdlpull parser by Vivek Krishna

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

#ifndef SCHEMA_TYPESTABLE_H
#define SCHEMA_TYPESTABLE_H

#include <QMap>
#include <QString>

#include "complextype.h"
#include "simpletype.h"

namespace XSD {

class TypesTable
{
public:
    TypesTable();
    TypesTable( const TypesTable &other );
    ~TypesTable();

    TypesTable &operator=( const TypesTable &other );

    void clear();

    int addType( XSDType *type );
    int addExtType( XSDType *type, int id );

    int typeId( const QName &name, bool create = false );

    QString typeName( int id ) const;

    int addExternalTypeId( const QName &name, XSDType *type );

    int numExtRefs() const;
    QName extRefName( int index ) const;
    int extRefType( int index ) const;

    void resolveForwardElementRefs( const QString &name, Element &element );
    void resolveForwardAttributeRefs( const QString &name, Attribute &attribute );

    XSDType *typePtr( int id ) const;

    int numTypes() const;

    bool detectUndefinedTypes();

    void setTargetNamespace( const QString &nameSpace );
    QString targetNamespace() const;

private:
    class Private {
    public:
        Private()
        {
            mCurrentId = XSDType::ANYURI + 1;

            //map of simple types
            mBasicTypes["string"] = XSDType::STRING;
            mBasicTypes["integer"] = XSDType::INTEGER;
            mBasicTypes["int"] = XSDType::INT;
            mBasicTypes["byte"] = XSDType::BYTE;
            mBasicTypes["unsignedByte"] = XSDType::UBYTE;
            mBasicTypes["positiveInteger"] = XSDType::POSINT;
            mBasicTypes["unsignedInt"] = XSDType::UINT;
            mBasicTypes["long"] = XSDType::LONG;
            mBasicTypes["unsignedLong"] = XSDType::ULONG;
            mBasicTypes["short"] = XSDType::SHORT;
            mBasicTypes["unsignedShort"] = XSDType::USHORT;
            mBasicTypes["decimal"] = XSDType::DECIMAL;
            mBasicTypes["float"] = XSDType::FLOAT;
            mBasicTypes["double"] = XSDType::DOUBLE;
            mBasicTypes["boolean"] = XSDType::BOOLEAN;
            mBasicTypes["time"] = XSDType::TIME;
            mBasicTypes["dateTime"] = XSDType::DATETIME;
            mBasicTypes["date"] = XSDType::DATE;
            mBasicTypes["token"] = XSDType::TOKEN;
            mBasicTypes["QName"] = XSDType::QNAME;
            mBasicTypes["NCName"] = XSDType::NCNAME;
            mBasicTypes["NMTOKEN"] = XSDType::NMTOKEN;
            mBasicTypes["NMTOKENS"] = XSDType::NMTOKENS;
            mBasicTypes["base64Binary"] = XSDType::BASE64BIN;
            mBasicTypes["hexBinary"] = XSDType::HEXBIN;
            mBasicTypes["anyType"] = XSDType::ANYTYPE;
            mBasicTypes["any"] = XSDType::ANY;
            mBasicTypes["anyURI"] = XSDType::ANYURI;
        }

        XSDType::List mTypes;

        //maintains a map of all user defined type names and their ids
        QMap<QString, int> mUserTypes;
        QMap<QString, int> mBasicTypes;

        int mCurrentId;

        QString mNameSpace;

        struct ExternRef
        {
            int localTypeId;
            QName qname;
        };

        QList<struct ExternRef> mExternRefs;
    };

    Private *d;
};


} // end XSD namespace

#endif
