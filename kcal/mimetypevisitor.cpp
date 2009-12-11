/*
    Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "mimetypevisitor.h"

static QLatin1String sEventType( "application/x-vnd.akonadi.calendar.event" );
static QLatin1String sTodoType( "application/x-vnd.akonadi.calendar.todo" );
static QLatin1String sJournalType( "application/x-vnd.akonadi.calendar.journal" );
static QLatin1String sFreeBusyType( "application/x-vnd.akonadi.calendar.freebusy" );

using namespace KCal;

class MimeTypeVisitor::Private
{
  public:
    QString mType;
};

MimeTypeVisitor::MimeTypeVisitor() : d( new Private() )
{
}

MimeTypeVisitor::~MimeTypeVisitor()
{
  delete d;
}

bool MimeTypeVisitor::visit( Event *event )
{
    Q_UNUSED( event );
    d->mType = sEventType;
    return true;
}

bool MimeTypeVisitor::visit( Todo *todo )
{
    Q_UNUSED( todo );
    d->mType = sTodoType;
    return true;
}

bool MimeTypeVisitor::visit( Journal *journal )
{
    Q_UNUSED( journal );
    d->mType = sJournalType;
    return true;
}

bool MimeTypeVisitor::visit( FreeBusy *freebusy )
{
    Q_UNUSED( freebusy );
    d->mType = sFreeBusyType;
    return true;
}

QString MimeTypeVisitor::mimeType() const
{
    return d->mType;
}

QStringList MimeTypeVisitor::allMimeTypes() const
{
    return QStringList() << sEventType << sTodoType << sJournalType << sFreeBusyType;
}

QString MimeTypeVisitor::mimeType( IncidenceBase *incidence )
{
  Q_ASSERT( incidence != 0 );

  incidence->accept( *this );
  return mimeType();
}

QString MimeTypeVisitor::eventMimeType()
{
  return sEventType;
}

QString MimeTypeVisitor::todoMimeType()
{
  return sTodoType;
}

QString MimeTypeVisitor::journalMimeType()
{
  return sJournalType;
}

QString MimeTypeVisitor::freeBusyMimeType()
{
  return sFreeBusyType;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
