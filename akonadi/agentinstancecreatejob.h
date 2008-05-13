/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTINSTANCECREATEJOB_H
#define AKONADI_AGENTINSTANCECREATEJOB_H

#include "akonadi_export.h"

#include <akonadi/agenttype.h>

#include <kjob.h>

namespace Akonadi {

class AgentInstance;

/**
 * @short Job for creating new agent instances.
 *
 * This class encapsulates the procedure of creating a new agent instance
 * and optionally configuring it immediately.
 *
 * @code
 *
 * MyClass::MyClass( QWidget *parent )
 *   : QWidget( parent )
 * {
 *   // Get agent type object
 *   Akonadi::AgentType type = Akonadi::AgentManager::type( "akonadi_vcard_resource" );
 *
 *   Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( type );
 *   connect( job, SIGNAL( result( KJob * ) ),
 *            this, SLOT( slotCreated( KJob * ) ) );
 *
 *   // show config dialog with this widget as parent
 *   job->configure( this );
 *
 *   job->start();
 * }
 *
 * ...
 *
 * void MyClass::slotCreated( KJob *job )
 * {
 *   Akonadi::AgentInstanceCreateJob *createJob = static_cast<Akonadi::AgentInstanceCreateJob*>( job );
 *
 *   qDebug() << "Created agent instance:" << createJob->instance().identifier();
 * }
 *
 * @endcode
 */
class AKONADI_EXPORT AgentInstanceCreateJob : public KJob
{
  Q_OBJECT
  public:
    /**
     * Create a new agent instance creation job.
     * @param type The type of the agent to create.
     * @param parent The parent object.
     */
    explicit AgentInstanceCreateJob( const AgentType &type, QObject *parent = 0 );

    /**
     * Destroys the agent instance creation job.
     */
    ~AgentInstanceCreateJob();

    /**
     * Setup the job to show agent configuration dialog once it has
     * been successfully started.
     * @param parent The parent window for the configuration dialog.
     */
    void configure( QWidget *parent = 0 );

    /**
     * Returns the AgentInstance object of the newly created agent instance.
     */
    AgentInstance instance() const;

    /**
     * Starts the instance creation.
     */
    void start();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const Akonadi::AgentInstance& ) )
    Q_PRIVATE_SLOT( d, void doConfigure() )
    Q_PRIVATE_SLOT( d, void timeout() )
    Q_PRIVATE_SLOT( d, void emitResult() )
};

}

#endif
