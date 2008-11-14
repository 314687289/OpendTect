/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:		Feb 2007
 RCS:           $Id: uiflatviewcontrol.cc,v 1.39 2008-11-14 04:43:52 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"
#include "uiobjdisposer.h"
#include "datapackbase.h"
#include "bufstringset.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, uiParent* p,
				      bool wrubb, bool withwva )
    : uiGroup(p ? p : vwr.attachObj()->parent(),"Flat viewer control")
    , zoommgr_(*new FlatView::ZoomMgr)
    , haverubber_(wrubb)
    , propdlg_(0)
    , infoChanged(this)
    , viewerAdded(this)
    , withwva_(withwva)
{
    setBorder( 0 );
    addViewer( vwr );
    vwr.attachObj()->parent()->finaliseDone.notify(
				    mCB(this,uiFlatViewControl,onFinalise) );
}


uiFlatViewControl::~uiFlatViewControl()
{
    delete &zoommgr_;
}


void uiFlatViewControl::addViewer( uiFlatViewer& vwr )
{
    vwrs_ += &vwr;
    vwr.dataChanged.notify( mCB(this,uiFlatViewControl,dataChangeCB) );
    vwr.control_ = this;

    uiRGBArrayCanvas& cnvs = vwr.rgbCanvas();
    if ( haverubber_ )
	cnvs.rubberBandUsed.notify( mCB(this,uiFlatViewControl,rubBandCB));

    //cnvs.setMouseTracking( true );
    MouseEventHandler& mevh = mouseEventHandler( vwrs_.size()-1 );
    mevh.movement.notify( mCB( this, uiFlatViewControl, mouseMoveCB ) );
    mevh.buttonReleased.notify( mCB(this,uiFlatViewControl,usrClickCB) );

    viewerAdded.trigger();
}


uiWorldRect uiFlatViewControl::getBoundingBox() const
{
    uiWorldRect bb( vwrs_[0]->boundingBox() );

    for ( int idx=1; idx<vwrs_.size(); idx++ )
    {
	const uiWorldRect bb2( vwrs_[idx]->boundingBox() );
	if ( bb2.left() < bb.left() ) bb.setLeft( bb2.left() );
	if ( bb2.right() > bb.right() ) bb.setRight( bb2.right() );
	if ( bb2.bottom() < bb.bottom() ) bb.setBottom( bb2.bottom() );
	if ( bb2.top() > bb.top() ) bb.setTop( bb2.top() );
    }

    return bb;
}


void uiFlatViewControl::onFinalise( CallBacker* )
{
    uiWorldRect viewrect = vwrs_[0]->curView();
    const bool canreuse = zoommgr_.current().width() > 10
			 && canReUseZoomSettings( vwrs_[0]->curView().centre(),
						  zoommgr_.current() );
    if ( !canreuse )
    {
	viewrect = getBoundingBox();
	zoommgr_.init( viewrect );
    }

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setView( viewrect );
    }

    finalPrepare();
}


void uiFlatViewControl::dataChangeCB( CallBacker* cb )
{
    zoommgr_.reInit( getBoundingBox() );
}


bool uiFlatViewControl::havePan(
	Geom::Point2D<double> oldcentre, Geom::Point2D<double> newcentre,
	Geom::Size2D<double> sz )
{
    const Geom::Point2D<double> eps( sz.width()*1e-6, sz.height()*1e-6 );
    return !mIsZero(oldcentre.x-newcentre.x,eps.x)
	|| !mIsZero(oldcentre.y-newcentre.y,eps.y);
}


bool uiFlatViewControl::haveZoom( Geom::Size2D<double> oldsz,
				  Geom::Size2D<double> newsz )
{
    const Geom::Point2D<double> eps( oldsz.width()*1e-6, oldsz.height()*1e-6 );
    return !mIsZero(oldsz.width()-newsz.width(),eps.x)
	|| !mIsZero(oldsz.height()-newsz.height(),eps.y);
}


uiWorldRect uiFlatViewControl::getNewWorldRect( Geom::Point2D<double>& centre,
						Geom::Size2D<double>& sz,
						const uiWorldRect& bb) const
{
    const uiWorldRect cv( vwrs_[0]->curView() );
    const bool havepan = havePan( cv.centre(), centre, cv.size() );
    const bool havezoom = haveZoom( cv.size(), sz );
    if ( !havepan && !havezoom ) return cv;

    uiWorldRect wr( havepan && havezoom ? getZoomAndPanRect(centre,sz,bb)
	    				: getZoomOrPanRect(centre,sz,bb) );
    centre = wr.centre(); sz = wr.size();
    if ( havezoom )
	zoommgr_.add( sz );

    if ( cv.left() > cv.right() ) wr.swapHor();
    if ( cv.bottom() > cv.top() ) wr.swapVer();
    return wr;
}


void uiFlatViewControl::setNewView( Geom::Point2D<double>& centre,
				    Geom::Size2D<double>& sz )
{
    const uiWorldRect wr = getNewWorldRect( centre, sz, getBoundingBox() );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setView( wr );
	vwrs_[idx]->drawBitMaps();
	vwrs_[idx]->drawAnnot();
    }
}


uiWorldRect uiFlatViewControl::getZoomAndPanRect( Geom::Point2D<double> centre,
						Geom::Size2D<double> sz,
       						const uiWorldRect& bbox	)
{
    //TODO we should have a different policy for requests outside
    return getZoomOrPanRect( centre, sz, bbox );
}


uiWorldRect uiFlatViewControl::getZoomOrPanRect( Geom::Point2D<double> centre,
						 Geom::Size2D<double> sz,
						 const uiWorldRect& inputbb )
{
    uiWorldRect bb( inputbb );
    if ( bb.left() > bb.right() ) bb.swapHor();
    if ( bb.bottom() > bb.top() ) bb.swapVer();

    if ( sz.width() > bb.width() )
	sz.setWidth( bb.width() );
    if ( sz.height() > bb.height() )
	sz.setHeight( bb.height() );
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    if ( centre.x - hwdth < bb.left() )		centre.x = bb.left() + hwdth;
    if ( centre.y - hhght < bb.bottom() )	centre.y = bb.bottom() + hhght;
    if ( centre.x + hwdth > bb.right() )	centre.x = bb.right() - hwdth;
    if ( centre.y + hhght > bb.top() )		centre.y = bb.top() - hhght;

    return uiWorldRect( centre.x - hwdth, centre.y + hhght,
			centre.x + hwdth, centre.y - hhght );
}


void uiFlatViewControl::flip( bool hor )
{
    const uiWorldRect cv( vwrs_[0]->curView() );
    const uiWorldRect newview(	hor ? cv.right()  : cv.left(),
				hor ? cv.top()    : cv.bottom(),
				hor ? cv.left()   : cv.right(),
				hor ? cv.bottom() : cv.top() );

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::Annotation::AxisData& ad
			    = hor ? vwrs_[idx]->appearance().annot_.x1_
				  : vwrs_[idx]->appearance().annot_.x2_;
	ad.reversed_ = !ad.reversed_;
	vwrs_[idx]->setView( newview );
	vwrs_[idx]->drawBitMaps();
	vwrs_[idx]->drawAnnot();
    }
}


void uiFlatViewControl::rubBandCB( CallBacker* cb )
{
    //TODO handle when zoom is disabled
    const uiRect* selarea = vwrs_[0]->rgbCanvas().getSelectedArea();
    uiWorld2Ui w2u;
    vwrs_[0]->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    const uiWorldRect oldview( vwrs_[0]->curView() );
    setNewView( centre, newsz );
}


void uiFlatViewControl::doPropertiesDialog( int vieweridx, bool dowva )
{
    //TODO what if more than one viewer? Also functions below
    uiFlatViewer& vwr = *vwrs_[vieweridx];
    BufferStringSet annots;
    const int selannot = vwr.getAnnotChoices( annots ); 

    if ( propdlg_ ) delete propdlg_;
    propdlg_ = new uiFlatViewPropDlg( vwr.attachObj()->parent(), vwr,
				  mCB(this,uiFlatViewControl,applyProperties),
	   			  annots.size() ? &annots : 0, selannot,
	   			  withwva_ && dowva );
    propdlg_->windowClosed.notify(mCB(this,uiFlatViewControl,propDlgClosed));
    propdlg_->go();
}


void uiFlatViewControl::propDlgClosed( CallBacker* )
{
    if ( propdlg_->uiResult() != 1 )
	return;

    applyProperties(0);
    if ( propdlg_->saveButtonChecked() )
	saveProperties( propdlg_->viewer() );

    uiOBJDISP()->go( propdlg_ );
    propdlg_ = 0;
}


void uiFlatViewControl::applyProperties( CallBacker* cb )
{
    if ( !propdlg_ ) return;

    mDynamicCastGet( uiFlatViewer*, vwr, &propdlg_->viewer() );
    if ( !vwr ) return;

    const uiWorldRect cv( vwr->curView() );
    FlatView::Annotation& annot = vwr->appearance().annot_;
    if ( (cv.right() > cv.left()) == annot.x1_.reversed_ )
	{ annot.x1_.reversed_ = !annot.x1_.reversed_; flip( true ); }
    if ( (cv.top() > cv.bottom()) == annot.x2_.reversed_ )
	{ annot.x2_.reversed_ = !annot.x2_.reversed_; flip( false ); }

    const int selannot = propdlg_->selectedAnnot();

    vwr->setAnnotChoice( selannot );
    //Don't send FlatView::Viewer::All, since that triggers a viewchange
    vwr->handleChange( FlatView::Viewer::Annot );
    vwr->handleChange( FlatView::Viewer::WVAPars );
    vwr->handleChange( FlatView::Viewer::VDPars );
}


void uiFlatViewControl::saveProperties( FlatView::Viewer& vwr )
{
    const FlatDataPack* fdp = vwr.pack( true );
    if ( !fdp ) fdp = vwr.pack( false );

    BufferString cat( "General" );

    if ( fdp )
    {
	cat = fdp->category();
	if ( category_.isEmpty() )
	    category_ = cat;
    }

    vwr.storeDefaults( cat );
}


MouseEventHandler& uiFlatViewControl::mouseEventHandler( int vieweridx )
{
    return vwrs_[vieweridx]->rgbCanvas().getMouseEventHandler();
}


void uiFlatViewControl::mouseMoveCB( CallBacker* cb )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !mouseEventHandler( idx ).hasEvent() )
	    continue;

	const MouseEvent& ev = mouseEventHandler(idx).event();
	uiWorld2Ui w2u;
	vwrs_[idx]->getWorld2Ui(w2u);
	const uiWorldPoint wp = w2u.transform( ev.pos() );
	vwrs_[idx]->getAuxInfo( wp, infopars_ );
	CBCapsule<IOPar> caps( infopars_, this );
	infoChanged.trigger( &caps );
    }
}



void uiFlatViewControl::usrClickCB( CallBacker* cb )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !mouseEventHandler( idx ).hasEvent() )
	    continue;

	if ( mouseEventHandler(idx).isHandled() )
	    return;

	mouseEventHandler(idx).setHandled( handleUserClick() );
    }
}


bool uiFlatViewControl::canReUseZoomSettings( Geom::Point2D<double> centre,
					      Geom::Size2D<double> sz ) const
{
    //TODO: allow user to decide to reuse or not with a specific parameter
    const uiWorldRect bb( getBoundingBox() );
    if ( sz.width() > bb.width() || sz.height() > bb.height() )
	return false;
    
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    Geom::Point2D<double> topleft( centre.x - hwdth, centre.y - hhght );
    Geom::Point2D<double> botright( centre.x + hwdth, centre.y + hhght );
    if ( ! ( bb.contains( topleft, 1e-6 ) && bb.contains( botright, 1e-6 ) ) )
	return false;

    return true;
}
