/*
    This file is part of KDE.

    Copyright (c) 2004,2010 Cornelius Schumacher <schumacher@kde.org>

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

#include "namer.h"

#include <libkode/style.h>

QString Namer::upperFirst( const QString &str )
{
  return KODE::Style::upperFirst( str );
}

QString Namer::lowerFirst( const QString &str )
{
  return KODE::Style::lowerFirst( str );
}

QString Namer::getClassName( const Schema::Attribute &attribute )
{
  return getClassName( attribute.name() );
}

QString Namer::getClassName( const Schema::Element &element )
{
  return getClassName( element.name() );
}

QString Namer::getClassName( const QString &elementName )
{
  QString name;
  QStringList parts = elementName.split( "_" );
  foreach( QString part, parts ) {
    name += upperFirst( part );
  }

  // reserved keywords are suffixed with an '_' to make sure that
  // they do not make compiler errors. Append the _ to the return value too
  if (elementName.endsWith("_"))
    name = name.append("_");
  return name;
}

QString Namer::getAccessor( const Schema::Element &element )
{
  return getAccessor( element.name() );
}

QString Namer::getAccessor( const Schema::Attribute &attribute )
{
  return getAccessor( attribute.name() );
}

QString Namer::getAccessor( const QString &elementName )
{
  QString name = lowerFirst(getClassName( elementName ));
  if (restrictedNames.contains(name))
    name = name.append('_');
  return name;
}

QString Namer::getMemberVariable(const Schema::Element &element)
{
  return getMemberVariable( element.name() );
}

QString Namer::getMemberVariable(const Schema::Attribute &attribute)
{
  return getMemberVariable( attribute.name() );
}

QString Namer::getMemberVariable(const QString &elementName)
{
  return "m" + upperFirst(getClassName( elementName ));
}

QString Namer::getMutator( const Schema::Element &element )
{
  return getMutator( element.name() );
}

QString Namer::getMutator( const Schema::Attribute &attribute )
{
  return getMutator( attribute.name() );
}

QString Namer::getMutator( const QString &elementName )
{
  return "set" + getClassName( elementName );
}
