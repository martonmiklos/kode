/*
    This file is part of KDE.

    Copyright (c) 2010 Cornelius Schumacher <schumacher@kde.org>

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

#include "classdescription.h"

ClassProperty::ClassProperty(const QString &type, const QString &name , bool isOptional, ClassPropertyType classPropertyType)
  : m_type( type ), m_name( name ), m_isList( false ), m_targetHasId( false ), m_isOptional(isOptional), m_classPropertyType(classPropertyType)
{
}

QString ClassProperty::type() const
{
  return m_type;
}

QString ClassProperty::name() const
{
  return m_name;
}

void ClassProperty::setIsList( bool v )
{
  m_isList = v;
}

bool ClassProperty::isList() const
{
  return m_isList;
}

void ClassProperty::setTargetHasId( bool v )
{
  m_targetHasId = v;
}

bool ClassProperty::targetHasId() const
{
  return m_targetHasId;
}
bool ClassProperty::isOptional() const
{
    return m_isOptional;
}

void ClassProperty::setOptional(bool isOptionalElement)
{
    m_isOptional = isOptionalElement;
}

void ClassProperty::setName(const QString &name)
{
  m_name = name;
}

ClassProperty::ClassPropertyType ClassProperty::classPropertyType() const
{
  return m_classPropertyType;
}

void ClassProperty::setClassPropertyType(const ClassPropertyType &classPropertyType)
{
  m_classPropertyType = classPropertyType;
}

ClassDescription::ClassDescription( const QString &name )
  : m_name( name )
{

}

QString ClassDescription::name() const
{
  return m_name;
}

void ClassDescription::addProperty( const QString &type, const QString &name, bool isOptional, ClassProperty::ClassPropertyType propertyType )
{
  m_properties.append( ClassProperty( type, name, isOptional, propertyType ) );
}

void ClassDescription::addProperty( const ClassProperty &p )
{
  m_properties.append( p );
}

ClassProperty::List ClassDescription::properties() const
{
  return m_properties;
}

bool ClassDescription::hasProperty( const QString &name ) const
{
  foreach( ClassProperty p, m_properties ) {
    if ( p.name() == name ) return true;
  }
  return false;
}

void ClassDescription::addEnum(const KODE::Enum &classEnum)
{
  m_enums.append( classEnum );
}

void ClassDescription::addEnum(const QString &name, const QList<XSD::SimpleType::EnumItem> &enumItems)
{
  m_enums.append( KODE::Enum(name, enumItems) );
}

KODE::Enum::List ClassDescription::enums() const
{
  return m_enums;
}

bool ClassDescription::hasEnum(const QString &name) const
{
  foreach( KODE::Enum e, m_enums ) {
    if ( e.name() == name ) return true;
  }
  return false;
}

