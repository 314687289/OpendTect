/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.cc,v 1.3 2008-08-04 06:56:39 cvsnanne Exp $
________________________________________________________________________

-*/


#include "measuretoolman.h"

#include "uimeasuredlg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "vispicksetdisplay.h"
#include "visselman.h"

#include "draw.h"
#include "pickset.h"

namespace Annotations
{

MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , picksetmgr_(Pick::SetMgr::getMgr("MeasureTool"))
    , measuredlg_(0)
{
    const CallBack cb( mCB(this,MeasureToolMan,buttonClicked) );
    butidx_ = appl.menuMgr().coinTB()->addButton( "measure.png", cb, 
						  "Display Distance", true );

    appl.sceneMgr().treeToBeAdded.notify(
			mCB(this,MeasureToolMan,sceneAdded) );
    appl.sceneMgr().sceneClosed.notify(
			mCB(this,MeasureToolMan,sceneClosed) );
    appl.sceneMgr().activeSceneChanged.notify(
			mCB(this,MeasureToolMan,sceneChanged) );
    visBase::DM().selMan().selnotifier.notify(
	    		mCB(this,MeasureToolMan,objSelected) );
}


void MeasureToolMan::objSelected( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    bool isownsel = false;
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
	if ( displayobjs_[idx]->id() == sel )
	    isownsel = true;

    appl_.menuMgr().coinTB()->turnOn( butidx_, isownsel );
}


void MeasureToolMan::buttonClicked( CallBacker* cb )
{
    const bool ison = appl_.menuMgr().coinTB()->isOn( butidx_ );
    if ( ison )
	appl_.sceneMgr().setToViewMode( false );

    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	if ( ison )
	    visBase::DM().selMan().select( displayobjs_[idx]->id() );
	else
	    visBase::DM().selMan().deSelect( displayobjs_[idx]->id() );
    }
}


static MultiID getMultiID( int sceneid )
{
    // Create dummy multiid, I don't want to save these picks
    BufferString mid( "9999.", sceneid );
    return MultiID( mid.buf() );
}


void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
    psd->ref();

    Pick::Set* ps = new Pick::Set( "Measure picks" );
    ps->disp_.connect_ = Pick::Set::Disp::Open;
    ps->disp_.color_ = Color( 255, 0, 0 );
    psd->setSet( ps );
    psd->setSetMgr( &picksetmgr_ );
    picksetmgr_.set( getMultiID(sceneid), ps );
    picksetmgr_.locationChanged.notify( mCB(this,MeasureToolMan,changeCB) );

    appl_.applMgr().visServer()->addObject( psd, sceneid, false );
    sceneids_ += sceneid;
    displayobjs_ += psd;
}


void MeasureToolMan::sceneClosed( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( sceneidx<0 || sceneidx>=displayobjs_.size() )
	return;

    appl_.applMgr().visServer()->removeObject( displayobjs_[sceneidx], sceneid);
    displayobjs_[sceneidx]->unRef();
    displayobjs_.remove( sceneidx );
}


static void giveCoordsToDialog( const Pick::Set& set, uiMeasureDlg& dlg )
{
    TypeSet<Coord3> crds;
    for ( int idx=0; idx<set.size(); idx++ )
	crds += set[idx].pos;
    dlg.fill( crds );
}


void MeasureToolMan::sceneChanged( CallBacker* cb )
{
    const bool ison = appl_.menuMgr().coinTB()->isOn( butidx_ );
    if ( !ison ) return;

    const int sceneid = appl_.sceneMgr().getActiveSceneID();
    if ( sceneid < 0 ) return;

    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayobjs_.validIdx(sceneidx) ) return;

    visBase::DM().selMan().select( displayobjs_[sceneidx]->id() );

    if ( displayobjs_[sceneidx]->getSet() && measuredlg_ )
	giveCoordsToDialog( *displayobjs_[sceneidx]->getSet(), *measuredlg_ );
}


void MeasureToolMan::changeCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb);
    if ( !cd || !cd->set_ ) return;

    if ( !measuredlg_ )
    {
	measuredlg_ = new uiMeasureDlg( 0 );
	LineStyle ls;
	ls.color_ = cd->set_->disp_.color_;
	ls.width_ = cd->set_->disp_.pixsize_;
	measuredlg_->setLineStyle( ls );
	measuredlg_->lineStyleChange.notify(
				mCB(this,MeasureToolMan,lineStyleChangeCB) ) ;
	measuredlg_->clearPressed.notify( mCB(this,MeasureToolMan,clearCB) );
    }

    measuredlg_->show();
    Pick::Set chgdset( *cd->set_ );
    if ( cd->ev_ == Pick::SetMgr::ChangeData::ToBeRemoved )
	chgdset.remove( cd->loc_ );

    giveCoordsToDialog( chgdset, *measuredlg_ );
}


void MeasureToolMan::clearCB( CallBacker* )
{
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	Pick::Set* ps = displayobjs_[idx]->getSet();
	if ( !ps ) continue;

	ps->erase();
	picksetmgr_.reportChange( this, *ps );
	displayobjs_[idx]->createLine();
    }

    if ( measuredlg_ )
	measuredlg_->reset();
}


void MeasureToolMan::lineStyleChangeCB( CallBacker* cb )
{
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	Pick::Set* ps = displayobjs_[idx]->getSet();
	if ( !ps ) continue;

      	LineStyle ls( measuredlg_->getLineStyle() );
	ps->disp_.color_ = ls.color_;
	ps->disp_.pixsize_ = ls.width_;
	picksetmgr_.reportDispChange( this, *ps );
    }
}


} // namespace Annotations
