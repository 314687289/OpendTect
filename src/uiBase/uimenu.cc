/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.cc,v 1.23 2005-09-28 12:11:03 cvsarend Exp $
________________________________________________________________________

-*/

#include "uimenu.h"
#include "i_qmenu.h"
#include "uiparentbody.h"
#include "uiobjbody.h"
#include "uibody.h"

#include <qmenudata.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qcursor.h>




class uiMenuItemContainerBody : public uiBodyImpl<uiMenuItemContainer,QMenuData>
{
public:

			uiMenuItemContainerBody(uiMenuItemContainer& handle, 
				       uiParent* parnt,
				       QMenuBar& qThing )
			    : uiBodyImpl<uiMenuItemContainer,QMenuData>
				( handle, parnt, qThing )
			    , bar_( &qThing )
			    , popup_( 0 )	{}

			uiMenuItemContainerBody(uiMenuItemContainer& handle, 
				       uiParent* parnt,
				       QPopupMenu& qThing )
			    : uiBodyImpl<uiMenuItemContainer,QMenuData>
				( handle, parnt, qThing )	
			    , popup_( &qThing )
			    , bar_( 0 )	{}

			~uiMenuItemContainerBody()	{ deepErase( itms ); }

    int			nrItems() const 		{ return itms.size(); }
    const ObjectSet<uiMenuItem>& items() const		{ return itms; }

    int			insertItem( uiMenuItem* it, int id, int idx )
			{
			    QString nm( it->name() );
			    i_MenuMessenger* msgr__= it->messenger();

			    int newid = qthing()->insertItem( nm, msgr__,
				    			      SLOT(activated()),
							      0, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    qthing()->setItemChecked( newid, it->checked );
			    qthing()->setItemEnabled( newid, it->enabled );
			    itms += it;

			    return newid;
			}

    int			insertItem( uiPopupMenu* pmnu, int id, int idx )
			{
			    uiPopupItem* it = &pmnu->item();

			    QString nm( it->name() );
			    QPopupMenu* pu = pmnu->body_->popup();

			    int newid = qthing()->insertItem( nm, pu, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    qthing()->setItemChecked( newid, it->checked );
			    qthing()->setItemEnabled( newid, it->enabled );
			    itms += it;

			    return newid;
			}

    int			insertItem( const char* text, const CallBack& cb, 
	    			    int id, int idx )
			{ 
			    uiMenuItem* it = new uiMenuItem( text, cb );

			    int newid = insertItem( it, id, idx );
			    it->setId( newid );
			    it->setMenu( this );
			    qthing()->setItemChecked( newid, it->checked );
			    qthing()->setItemEnabled( newid, it->enabled );
			    itms += it;

			    return newid;
			} 


    bool			isCheckable();
    void			setCheckable( bool yn );

    QMenuBar*			bar()			{ return bar_; }
    QPopupMenu*			popup()			{ return popup_; }

    virtual const QWidget*	managewidg_() const 	{ return qwidget(); }

    void			setIcon( const QPixmap& pm )
				{
    			    	    if ( bar_ ) bar_->setIcon( pm );
    			    	    if ( popup_ ) popup_->setIcon( pm );
				}

    void			setSensitive( bool yn )
				{
				    if ( bar_ ) bar_->setEnabled( yn );
				}

private:

    ObjectSet<uiMenuItem>	itms;

    QMenuBar*			bar_;
    QPopupMenu*			popup_;

};


//-----------------------------------------------------------------------

uiMenuItem::uiMenuItem( const char* nm )
    : UserIDObject(nm)
    , activated(this)
    , messenger_( *new i_MenuMessenger(this) ) 
    , id_(-1)
    , menu_(0)
    , enabled(true)
    , checked(false)
{}


uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb )
    : UserIDObject(nm )
    , activated(this)
    , messenger_( *new i_MenuMessenger(this) )
    , id_(-1)
    , menu_(0)
    , enabled(true)
    , checked(false)
{ 
    activated.notify( cb ); 
}


uiMenuItem::~uiMenuItem()
{ 
    delete &messenger_; 
}


bool uiMenuItem::isEnabled () const
{
    return menu_ && menu_->qthing() ? menu_->qthing()->isItemEnabled( id_ )
				    : enabled;
}

void uiMenuItem::setEnabled ( bool yn )
{
    enabled = yn;
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->setItemEnabled( id_, yn ); 
}


bool uiMenuItem::isChecked () const
{
    return menu_ && menu_->qthing() ? menu_->qthing()->isItemChecked( id_ )
				    : checked; 
}


void uiMenuItem::setChecked( bool yn )
{
    checked = yn;
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->setItemChecked( id_, yn ); 
}


void uiMenuItem::setText( const char* txt )
{
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->changeItem ( id_, QString(txt) ); 
}


int uiMenuItem::index() const
{
    return menu_ && menu_->qthing() ? menu_->qthing()->indexOf( id_ ) : -1;
}


//-----------------------------------------------------------------------



uiMenuItemContainer::uiMenuItemContainer( const char* nm,
					  uiMenuItemContainerBody* b )
    : uiObjHandle( nm, b )
    , body_( b )				{}


uiMenuItemContainer::~uiMenuItemContainer()	{ delete body_; }


int uiMenuItemContainer::nrItems() const
    { return body_->nrItems(); }


const ObjectSet<uiMenuItem>& uiMenuItemContainer::items() const
    { return body_->items(); }


int uiMenuItemContainer::insertItem( uiMenuItem* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

/*!
    \brief Add a menu item by menu-text and CallBack.

    If you want to be able to specify a callback flexibly, construct
    a uiMenuItem by hand and insert this into the menu. Then you can
    add callbacks at any time to the uiMenuItem.

*/
int uiMenuItemContainer::insertItem( const char* text, const CallBack& cb,
				     int id, int idx )
    { return body_->insertItem( text, cb, id, idx ); }

int uiMenuItemContainer::insertItem( uiPopupMenu* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

void uiMenuItemContainer::insertSeparator( int idx ) 
    { body_->qthing()->insertSeparator( idx ); }


void uiMenuItemContainer::setMenuBody(uiMenuItemContainerBody* b)  
{ 
    body_=b;
    setBody( b );
}

int uiMenuItemContainer::idAt( int idx ) const
    { return body_->qthing()->idAt(idx); }

int uiMenuItemContainer::indexOf( int id ) const
    { return body_->qthing()->indexOf(id); }

// ------------------------------------------------------------------------


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( nm, 0 )
{ 
    setMenuBody( new uiMenuItemContainerBody( *this, parnt,
			*new QMenuBar(parnt->body()->qwidget(),nm ) ));
}


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm, QMenuBar& qThing )
    : uiMenuItemContainer( nm, 0 )
{ 
    setMenuBody( new uiMenuItemContainerBody( *this, parnt, qThing ) ); 
}


void uiMenuBar::reDraw( bool deep )
{
    if ( body_->bar() ) 
	body_->bar()->update();
}


void uiMenuBar::setIcon( const QPixmap& pm )
{
    body_->setIcon( pm );
}


void uiMenuBar::setSensitive( bool yn )
{
    body_->setSensitive( yn );
}


// -----------------------------------------------------------------------

uiPopupItem::uiPopupItem( uiPopupMenu& popmen, const char* nm )
    : uiMenuItem( nm )
    , popmenu_( &popmen )
{}


bool uiPopupItem::isCheckable() const
{
    if( !menu_->popup() ) { pErrMsg("Huh?"); return false; }
    return menu_->popup()->isCheckable(); 
}


void uiPopupItem::setCheckable( bool yn )
{
    if( !menu_->popup() ) { pErrMsg("Huh?"); return; }
    menu_->popup()->setCheckable(yn); 
}


// -----------------------------------------------------------------------

uiPopupMenu::uiPopupMenu( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( nm, 0 )
    , item_( *new uiPopupItem( *this, nm ) )
{
    setMenuBody ( new uiMenuItemContainerBody( *this, parnt, 
                              *new QPopupMenu(parnt->body()->qwidget(),nm ) ));
    item_.setMenu( body_ );
}


uiPopupMenu::uiPopupMenu( uiParent* parnt, QPopupMenu* qmnu, const char* nm )
    : uiMenuItemContainer(nm,0)
    , item_(*new uiPopupItem(*this,nm))
{
    setMenuBody( new uiMenuItemContainerBody(*this,parnt,*qmnu) );
    item_.setMenu( body_ );
}


uiPopupMenu::~uiPopupMenu() 
{ 
    delete &item_; 
}


bool uiPopupMenu::isCheckable() const
{ return item().isCheckable(); }

void uiPopupMenu::setCheckable( bool yn ) 
{ item().setCheckable( yn ); }

bool uiPopupMenu::isChecked() const
{ return item().isChecked(); }

void uiPopupMenu::setChecked( bool yn )
{ item().setChecked( yn ); }

bool uiPopupMenu::isEnabled() const
{ return item().isEnabled(); }

void uiPopupMenu::setEnabled( bool yn )
{ item().setEnabled( yn ); }


int uiPopupMenu::exec()
{
    if ( !body_->popup() ) return -1;

    return body_->popup()->exec(QCursor::pos());
}

