/*
    This file is part of KXForms.

    Copyright (c) 2005,2006 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef GUIHANDLER_H
#define GUIHANDLER_H

#include "reference.h"
#include "guielement.h"

class QLayout;
class QWidget;

namespace KXForms {

class Manager;

class GuiHandler
{
  public:
    enum LayoutStyle { Grid, Vertical };
    /**
      Create GuiHandler and register it with the manager. The manager takes
      ownership of the GuiHandler object.
    */
    GuiHandler( Manager * );
    virtual ~GuiHandler();

    virtual QWidget *createRootGui( QWidget *parent ) = 0;
    virtual void createGui( const Reference &ref, QWidget *parent ) = 0;

    virtual QLayout *getTopLayout() = 0;
    virtual void addWidget( QLayout *, QWidget * ) = 0;
    virtual void addElement( QLayout *, QWidget *label, QWidget *widget, GuiElement::Properties *prop = 0 ) = 0;

    Manager *manager() const;

    void setLayoutStyle( LayoutStyle style ) { mLayoutStyle = style; }
    LayoutStyle layoutStyle() { return mLayoutStyle; }

  private:
    Manager *mManager;
    LayoutStyle mLayoutStyle;
};

}

#endif
