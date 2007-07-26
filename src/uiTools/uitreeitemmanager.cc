/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uitreeitemmanager.cc,v 1.34 2007-07-26 07:02:57 cvsnanne Exp $";


#include "uitreeitemmanager.h"
#include "uimenu.h"
#include "errh.h"
#include "uilistview.h"


uiTreeItem::uiTreeItem( const char* name__ )
    : parent_( 0 )
    , name_( name__ )
    , uilistviewitem_( 0 )
{
}


uiTreeItem::~uiTreeItem()
{
    while ( children_.size() )
	removeChild( children_[0] );
}


const char* uiTreeItem::name() const
{
    return name_.buf();
}



bool uiTreeItem::rightClick( uiListViewItem* item )
{
    if ( item==uilistviewitem_ )
    {
	showSubMenu();
	return true;
    }

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx]->rightClick(item) )
	    return true;
    }

    return false;
}


bool uiTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item==uilistviewitem_ )
	return select();

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx]->anyButtonClick(item) )
	    return true;
    }

    return false;
}


void uiTreeItem::updateSelection( int selid, bool downward )
{
    if ( uilistviewitem_ )
	uilistviewitem_->setSelected( shouldSelect(selid) );

    if ( downward )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->updateSelection(selid,downward);
    }
    else if ( parent_ )
	parent_->updateSelection( selid, false );
}


bool uiTreeItem::shouldSelect( int selid ) const
{
    return selid!=-1 && selid==selectionKey();
}


bool uiTreeItem::select(int selkey)
{
    return parent_ ? parent_->select(selkey) : false;
}


bool uiTreeItem::select()
{
    return select(selectionKey());
}


void uiTreeItem::prepareForShutdown()
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->prepareForShutdown();
}


void uiTreeItem::setChecked( bool yn, bool trigger )
{ if ( uilistviewitem_ ) uilistviewitem_->setChecked( yn, trigger ); }

bool uiTreeItem::isChecked() const
{ return uilistviewitem_ ? uilistviewitem_->isChecked() : false; } 

NotifierAccess* uiTreeItem::checkStatusChange() 
{ return uilistviewitem_ ? &uilistviewitem_->stateChanged : 0; }


void uiTreeItem::updateColumnText( int col )
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->updateColumnText(col);

    if ( !uilistviewitem_ ) return;

    if ( !col )
    {
	uilistviewitem_->setText( name_, col );
    }
}


const uiTreeItem* uiTreeItem::findChild( const char* nm ) const
{
    return const_cast<uiTreeItem*>(this)->findChild(nm);
}
	

const uiTreeItem* uiTreeItem::findChild( int selkey ) const
{
    return const_cast<uiTreeItem*>(this)->findChild(selkey);
}


uiTreeItem* uiTreeItem::findChild( const char* nm )
{
    if ( !strcmp( nm, name_ ) )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	uiTreeItem* res = children_[idx]->findChild(nm);
	if ( res )
	    return res;
    }

    return 0;
}
	

uiTreeItem* uiTreeItem::findChild( int selkey )
{
    if ( selectionKey()==selkey )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	uiTreeItem* res = children_[idx]->findChild(selkey);
	if ( res )
	    return res;
    }

    return 0;
}


void uiTreeItem::moveItem( uiTreeItem* below )
{
    getItem()->moveItem( below->getItem() );
}


void uiTreeItem::moveItemToTop()
{
    if ( parent_ && parent_->getItem() && getItem() )
    {
	uiListViewItem* item = getItem();
	parent_->getItem()->takeItem( item );
	parent_->getItem()->insertItem( item );
    }
}
	

int uiTreeItem::uiListViewItemType() const
{
    return uiListViewItem::Standard;
}


uiParent* uiTreeItem::getUiParent() const
{
    return parent_ ? parent_->getUiParent() : 0;
}


void uiTreeItem::setListViewItem( uiListViewItem* item )
{
    uilistviewitem_ = item;
    uilistviewitem_->setExpandable(isExpandable());
    uilistviewitem_->setSelectable(isSelectable());
}


int uiTreeItem::siblingIndex() const
{
    if ( !uilistviewitem_ ) return -1;
    return uilistviewitem_->siblingIndex();
}


uiTreeItem* uiTreeItem::siblingAbove()
{
    if ( !parent_ || !uilistviewitem_ ) return 0;

    uiListViewItem* itemabove = uilistviewitem_->itemAbove();
    if ( !itemabove ) return 0;

    for ( int idx=0; idx<parent_->children_.size(); idx++ )
    {
	if ( parent_->children_[idx]->getItem()==itemabove )
	    return parent_->children_[idx];
    }

    return 0;
}


uiTreeItem* uiTreeItem::siblingBelow()
{
    if ( !parent_ || !uilistviewitem_ ) return 0;

    uiListViewItem* itembelow = uilistviewitem_->itemBelow();
    if ( !itembelow ) return 0;

    for ( int idx=0; idx<parent_->children_.size(); idx++ )
    {
	if ( parent_->children_[idx]->getItem()==itembelow )
	    return parent_->children_[idx];
    }

    return 0;
}


#define mAddChildImpl( uiparentlist ) \
if ( !strcmp(newitem->parentType(), typeid(*this).name()) ) \
{ \
    children_ += newitem; \
    newitem->parent_ = this; \
 \
    uiListViewItem* item = new uiListViewItem(uiparentlist, \
		  uiListViewItem::Setup(newitem->name(), \
		  (uiListViewItem::Type)newitem->uiListViewItemType())); \
    newitem->setListViewItem( item ); \
    while ( below && item->nextSibling() ) \
        item->moveItem( item->nextSibling() ); \
 \
    if ( !newitem->init() ) \
    { \
	removeChild( newitem ); \
	return true; \
    } \
 \
    if ( uilistviewitem_ ) uilistviewitem_->setOpen( true ); \
    updateColumnText(0); updateColumnText(1); \
    return true; \
} \
 \
if ( downwards ) \
{ \
    for ( int idx=0; idx<children_.size(); idx++ ) \
    { \
	if ( children_[idx]->addChild( newitem, below, downwards ) ) \
	    return true; \
    } \
} \
else if ( parent_ ) \
    return parent_->addChild( newitem, below, downwards ); \
 \
return false; 


bool uiTreeItem::addChild( uiTreeItem* newitem, bool below )
{
    return addChild( newitem, below, false );
}


bool uiTreeItem::addChild( uiTreeItem* newitem, bool below, bool downwards )
{
    mAddChildImpl( uilistviewitem_ );
}


void uiTreeItem::removeChild( uiTreeItem* treeitem )
{
    const int idx=children_.indexOf( treeitem );
    if ( idx<0 )
    {
	for ( int idy=0; idy<children_.size(); idy++ )
	    children_[idy]->removeChild(treeitem);

	return;
    }

    if ( uilistviewitem_ ) uilistviewitem_->removeItem( treeitem->getItem() );
    uiTreeItem* child = children_[idx];
    children_.remove( idx );
    delete child;
}


uiTreeTopItem::uiTreeTopItem( uiListView* listview)
    : uiTreeItem( listview->name() )
    , listview_( listview )
    , disabrightclick_(false)
    , disabanyclick_(false)
    , nonsensiselkey_(-1)
{
    listview_->rightButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview_->mouseButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
    listview_->selectionChanged.notify(
	    		mCB(this,uiTreeTopItem,selectionChanged) );
}


uiTreeTopItem::~uiTreeTopItem()
{
    listview_->rightButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview_->mouseButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
    listview_->selectionChanged.remove(
	    		mCB(this,uiTreeTopItem,selectionChanged) );
}


bool uiTreeTopItem::addChild( uiTreeItem* newitem, bool below )
{
    return addChild( newitem, below, true );
}


bool uiTreeTopItem::addChild( uiTreeItem* newitem, bool below, bool downwards )
{
    downwards = true;		//We are at the top, so we should go downwards
    mAddChildImpl( listview_ );
}


void uiTreeTopItem::selectionChanged( CallBacker* )
{
    if ( !listview_->sensitive() || disabanyclick_ ) return;
    anyButtonClick( listview_->itemNotified() );
}


void uiTreeTopItem::rightClickCB( CallBacker* )
{
    if ( !listview_->sensitive() || disabanyclick_ || disabrightclick_ ) return;
    rightClick( listview_->itemNotified() );
}


void uiTreeTopItem::anyButtonClickCB( CallBacker* )
{
    if ( !listview_->sensitive() )
    {
	updateSelection( nonsensiselkey_,true);
	return;
    }
    nonsensiselkey_ = -1;
    
    if ( disabanyclick_ ) return;
    anyButtonClick( listview_->itemNotified() );
}


void uiTreeTopItem::updateSelection( int selectionkey, bool dw )
{
    uiTreeItem::updateSelection( selectionkey, true );
    listview_->triggerUpdate();

    if ( !listview_->sensitive() && selectionkey!=-1 )
	nonsensiselkey_ = selectionkey;
}


void uiTreeTopItem::updateColumnText(int col)
{
    //Is only impl to have it nicely together with updateSelection at the
    //public methods in the headerfile.
    uiTreeItem::updateColumnText(col);
}


uiParent* uiTreeTopItem::getUiParent() const
{
    return listview_->parent();
}


uiTreeFactorySet::uiTreeFactorySet()
    : addnotifier( this )
    , removenotifier( this )
{}


uiTreeFactorySet::~uiTreeFactorySet()
{
    deepErase( factories_ );
}


void uiTreeFactorySet::addFactory( uiTreeItemFactory* ptr, int placement,
       				   int pol2d )
{
    factories_ += ptr;
    placementidxs_ += placement;
    pol2ds_ += pol2d;
    addnotifier.trigger( factories_.size()-1 );
}


void uiTreeFactorySet::remove( const char* nm )
{
    int index = -1;
    for ( int idx=0; idx<factories_.size(); idx++ )
    {
	if ( !strcmp(nm,factories_[idx]->name()) )
	{
	    index=idx;
	    break;
	}
    }

    if ( index<0 )
	return;

    removenotifier.trigger( index );
    delete factories_[index];
    factories_.remove( index );
    placementidxs_.remove( index );
    pol2ds_.remove( index );
}


int uiTreeFactorySet::nrFactories() const
{ return factories_.size(); }

const uiTreeItemFactory* uiTreeFactorySet::getFactory( int idx ) const
{ return idx<nrFactories() ? factories_[idx] : 0; }

int uiTreeFactorySet::getPlacementIdx( int idx ) const
{ return placementidxs_[idx]; }

int uiTreeFactorySet::getPol2D( int idx ) const
{ return pol2ds_[idx]; }
