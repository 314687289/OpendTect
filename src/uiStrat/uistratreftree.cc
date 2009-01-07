/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.27 2009-01-07 15:11:25 cvsbert Exp $";

#include "uistratreftree.h"

#include "pixmap.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "uigeninput.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uirgbarray.h"
#include "uistratmgr.h"
#include "uistratutildlgs.h"

#define mAddCol(wdth,nr) \
    lv_->setColumnWidth( nr, wdth )

#define PMWIDTH		11
#define PMHEIGHT	9

static const int sUnitsCol	= 0;
static const int sDescCol	= 1;
static const int sLithoCol	= 2;

using namespace Strat;

uiStratRefTree::uiStratRefTree( uiParent* p, uiStratMgr* uistratmgr )
    : tree_(0)
    , uistratmgr_(uistratmgr)
{
    lv_ = new uiListView( p, "RefTree viewer" );
    BufferStringSet labels;
    labels.add( "Unit" );
    labels.add( "Description" );
    labels.add( "Lithology" );
    lv_->addColumns( labels );
    mAddCol( 300, 0 );
    mAddCol( 200, 1 );
    mAddCol( 150, 2 );
    lv_->setPrefWidth( 650 );
    lv_->setPrefHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );

    setTree( uistratmgr_->getCurTree() );
}


uiStratRefTree::~uiStratRefTree()
{
    delete lv_;
}


void uiStratRefTree::setTree( const RefTree* rt, bool force )
{
    if ( !force && rt == tree_ ) return;

    tree_ = rt;
    if ( !tree_ ) return;

    lv_->clear();
    addNode( 0, *((NodeUnitRef*)tree_), true );
}


void uiStratRefTree::addNode( uiListViewItem* parlvit,
			      const NodeUnitRef& nur, bool root )
{
    uiListViewItem* lvit = parlvit
	? new uiListViewItem( parlvit, uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) )
	: root ? 0 : new uiListViewItem( lv_,uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) );

    if ( parlvit || !root )
    {
	ioPixmap* pm = createLevelPixmap( &nur );
	lvit->setPixmap( 0, *pm );
	delete pm;
    }
    
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    uiListViewItem* item;
	    mDynamicCastGet(const LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur->code() )
				.label( lur->description() )
				.label( uistratmgr_->getLithName(*lur) );
	    if ( lvit )
		item = new uiListViewItem( lvit, setup );
	    else
		item = new uiListViewItem( lv_, setup );
	    
	    ioPixmap* pm = createLevelPixmap( lur );
	    item->setPixmap( 0, *pm );
	    delete pm;
	}
	else
	{
	    mDynamicCastGet(const NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
		addNode( lvit, *chldnur, false );
	}
    }
}


void uiStratRefTree::expand( bool yn ) const
{
    if ( yn )
	lv_->expandAll();
    else
	lv_->collapseAll();
}


void uiStratRefTree::makeTreeEditable( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstItem();
    while ( lvit )
    {
	lvit->setRenameEnabled( sUnitsCol, yn );
	lvit->setRenameEnabled( sDescCol, yn );
	lvit->setDragEnabled( yn );
	lvit->setDropEnabled( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    uiListViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->renameEnabled(sUnitsCol) ) return;

    int col = lv_->columnNotified();
    if ( col == sUnitsCol || col == sDescCol )
    {
	uiPopupMenu mnu( lv_->parent(), "Action" );
	mnu.insertItem( new uiMenuItem("&Specify level boundary ..."), 0 );
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 1 );
	mnu.insertItem( new uiMenuItem("&Add unit ..."), 2 );
	mnu.insertItem( new uiMenuItem("&Remove"), 3 );
    /*    mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Rename"), 4 );*/

	const int mnuid = mnu.exec();
	if ( mnuid<0 ) return;
	else if ( mnuid==0 )
	    selBoundary();
	else if ( mnuid==1 )
	    insertSubUnit( lvit );
	else if ( mnuid==2 )
	    insertSubUnit( 0 );
	else if ( mnuid==3 )
	    removeUnit( lvit );
    }
    else if ( col == sLithoCol )
    {
	uiStratLithoDlg lithdlg( lv_->parent(), uistratmgr_ );
	lithdlg.setSelectedLith( lvit->text( sLithoCol ) );
	if ( lithdlg.go() )
	    lvit->setText( lithdlg.getLithName(), sLithoCol );
    }
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    uiStratUnitDlg newurdlg( lv_->parent(), uistratmgr_ );
    if ( newurdlg.go() )
    {
	uiListViewItem* newitem;
	uiListViewItem::Setup setup = uiListViewItem::Setup()
				    .label( newurdlg.getUnitName() )
				    .label( newurdlg.getUnitDesc() )
				    .label( newurdlg.getUnitLith() );
	if ( lvit )
	    newitem = new uiListViewItem( lvit, setup );
	else
	    newitem = new uiListViewItem( lv_, setup );

	newitem->setRenameEnabled( sUnitsCol, true );
	newitem->setRenameEnabled( sDescCol, true );
	newitem->setDragEnabled( true );
	newitem->setDropEnabled( true );
	ioPixmap* pm = createLevelPixmap(0);
	newitem->setPixmap( 0, *pm );
	delete pm;
	
	if ( newitem->parent() )
	    newitem->parent()->setOpen( true );

	lv_->setCurrentItem( newitem );
	uiListViewItem* parit = newitem->parent();
	if ( parit )
	    uistratmgr_->prepareParentUnit( getCodeFromLVIt( parit ).buf() );
	
	BufferString codestr = getCodeFromLVIt( newitem );
	BufferString description = newitem->text(1);
	BufferString lithonm = newitem->text(2);
	uistratmgr_->addUnit( codestr.buf(), lithonm, description, true );
    }
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
    if ( !lvit ) return;
    uistratmgr_->removeUnit( getCodeFromLVIt( lvit ).buf() );
    if ( lvit->parent() )
	lvit->parent()->removeItem( lvit );
    else
    {
	lv_->takeItem( lvit );
	delete lvit;
    }

    lv_->triggerUpdate();
}


ioPixmap* uiStratRefTree::createLevelPixmap( const UnitRef* ref ) const
{
    uiRGBArray rgbarr( false );
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( Color::White() );
    for ( int idw=0; idw<PMWIDTH; idw++ )
	rgbarr.set( idw, mNINT(PMHEIGHT/2), Color::Black() );

    if ( ref )
    {
	Color col;
	if ( uistratmgr_->getLvlCol( ref, true, col ) )
	{
	    for ( int idw=0; idw<PMWIDTH; idw++ )
	    {
		rgbarr.set( idw, 0, col );
		rgbarr.set( idw, 1, col );
		rgbarr.set( idw, 2, col );
	    }
	}
	
	if ( uistratmgr_->getLvlCol( ref, false, col ) )
	{
	    for ( int idw=0; idw<PMWIDTH; idw++ )
	    {
		rgbarr.set( idw, PMHEIGHT-3, col );
		rgbarr.set( idw, PMHEIGHT-2, col );
		rgbarr.set( idw, PMHEIGHT-1, col );
	    }
	}
    }
    return new ioPixmap( rgbarr );
}


BufferString uiStratRefTree::getCodeFromLVIt( const uiListViewItem* item ) const
{
    if ( !item )
	return BufferString();

    BufferString bs = item->text();

    while ( item->parent() )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }

    return bs;
}


void uiStratRefTree::updateLvlsPixmaps()
{
    UnitRef::Iter it( *uistratmgr_->getCurTree() );
    const UnitRef* firstun = it.unit();
    ioPixmap* pm = createLevelPixmap( firstun );
    uiListViewItem* firstlvit = lv_->findItem( firstun->code().buf(),0,false );
    if ( firstlvit )
	firstlvit->setPixmap( 0, *pm );
    delete pm;
    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	ioPixmap* pm = createLevelPixmap( un );
	uiListViewItem* lvit = lv_->findItem( un->code().buf(), 0, false );
	if ( lvit )
	    lvit->setPixmap( 0, *pm );
	delete pm;
    }
}



#define mUpdateLvlInfo(loc,istop)\
{\
    bool has##loc##lvl = lvllinkdlg.lvl##loc##listfld_->isChecked();\
    if ( !has##loc##lvl )\
	uistratmgr_->freeLevel( getCodeFromLVIt( curit ).buf(), istop);\
    if ( has##loc##lvl )\
	uistratmgr_->linkLevel( getCodeFromLVIt( curit ).buf(), \
				lvllinkdlg.lvl##loc##listfld_->text(), istop );\
}


void uiStratRefTree::selBoundary()
{
    uiListViewItem* curit = lv_->currentItem();
    if ( !curit ) return;

    uiStratLinkLvlUnitDlg lvllinkdlg( lv_->parent(), 
	    			      getCodeFromLVIt( curit ).buf(),
	   			      uistratmgr_ );
    lvllinkdlg.go();
    if ( lvllinkdlg.uiResult() == 1 )
    {
	mUpdateLvlInfo( top, true )
	mUpdateLvlInfo( base, false )
	updateLvlsPixmaps();
	lv_->triggerUpdate();
    }
}
