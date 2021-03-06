/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTEDITOR_H
#define AKONADI_CONTACTEDITOR_H

#include "akonadi-contact_export.h"

#include <QWidget>

namespace KABC {
class Addressee;
}

namespace Akonadi {

class AbstractContactEditorWidget;
class Collection;
class Item;

/**
 * @short An widget to edit contacts in Akonadi.
 *
 * This widget provides a way to create a new contact or edit
 * an existing contact in Akonadi.
 *
 * Example for creating a new contact:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * ContactEditor *editor = new ContactEditor( Akonadi::ContactEditor::CreateMode, this );
 *
 * ...
 *
 * if ( !editor->saveContact() ) {
 *   qDebug() << "Unable to save new contact to storage";
 *   return;
 * }
 *
 * @endcode
 *
 * Example for editing an existing contact:
 *
 * @code
 *
 * const Akonadi::Item contact = ...;
 *
 * ContactEditor *editor = new ContactEditor( Akonadi::ContactEditor::EditMode, this );
 * editor->loadContact( contact );
 *
 * ...
 *
 * editor->saveContactInAddressBook();
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactEditor : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Describes the mode of the editor.
     */
    enum Mode {
      CreateMode, ///< Creates a new contact
      EditMode    ///< Edits an existing contact
    };

    enum DisplayMode {
      FullMode, ///< Show all pages
      VCardMode ///< Show just pages with elements stored in vcard.
    };

    /**
     * Creates a new contact editor with the standard editor widget.
     *
     * @param mode The mode of the editor.
     * @param parent The parent widget of the editor.
     */
    explicit ContactEditor( Mode mode, QWidget *parent = 0 );

    /**
     * Creates a new contact editor with a custom editor widget.
     *
     * @param mode The mode of the editor.
     * @param editorWidget The contact editor widget that shall be used for editing.
     * @param parent The parent widget of the editor.
     */
    ContactEditor( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent = 0 );

    /**
     * Creates a new contact editor dialog with a custom editor widget.
     *
     * @param mode The mode of the dialog.
     * @param displayMode mode for displaying the editor
     * @param parent The parent widget of the dialog.
     * @since 4.10
     */
    ContactEditor( Mode mode, DisplayMode displayMode, QWidget *parent = 0 );


    /**
     * Destroys the contact editor.
     */
    virtual ~ContactEditor();

    /**
     * Sets a @p contact that is used as template in create mode.
     * The fields of the editor will be prefilled with the content of the contact.
     * @param contact the contact to use as template content
     */
    void setContactTemplate( const KABC::Addressee &contact );

    /**
     * Sets the @p addressbook which shall be used to store new
     * contacts.
     */
    void setDefaultAddressBook( const Akonadi::Collection &addressbook );

    /**
     * @since 4.10
     * @brief ContactEditor::contact
     * @return
     */
    KABC::Addressee contact();
  public Q_SLOTS:
    /**
     * Loads the @p contact into the editor.
     */
    void loadContact( const Akonadi::Item &contact );

    /**
     * Saves the contact from the editor back to the storage.
     * @deprecated use saveContactInAddressBook. We can't get contactStored and error infos.
     * Remove it for KDE SC5
     */
    KDE_DEPRECATED bool saveContact();

    /**
     * Save the contact from the editor back to the storage. And return error.
     * Need to connect to finished() signal, to keep time to emit signal.
     * @since 4.11
     */
    void saveContactInAddressBook();

  Q_SIGNALS:
    /**
     * This signal is emitted when the @p contact has been saved back
     * to the storage.
     */
    void contactStored( const Akonadi::Item &contact );

    /**
     * This signal is emitted when an error occurred during the save.
     * @param errorMsg The error message.
     * @since 4.11
     */
    void error( const QString &errorMsg );

    /**
     * @brief finished
     * @since 4.11
     */
    void finished();
  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void itemFetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void parentCollectionFetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void storeDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    //@endcond
};

}

#endif
