/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          February 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicoltabman.cc,v 1.25 2009-01-27 10:17:24 cvsbert Exp $";

#include "uicoltabman.h"

#include "bufstring.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "draw.h"
#include "keystrs.h"
#include "settings.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicoltabimport.h"
#include "uicoltabmarker.h"
#include "uicoltabtools.h"
#include "uifunctiondisplay.h"
#include "uirgbarray.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilistview.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uiworld2ui.h"

#define mTransHeight	150
#define mTransWidth	200

static const char* sKeyDefault = "Default";
static const char* sKeyEdited = "Edited";
static const char* sKeyOwn = "Own";


uiColorTableMan::uiColorTableMan( uiParent* p, ColTab::Sequence& ctab )
    : uiDialog(p,uiDialog::Setup("Colortable Manager",
				 "Add, remove, change colortables",
				 "50.1.1"))
    , ctab_(ctab)
    , orgctab_(0)
    , issaved_(true)
    , tableChanged(this)
    , tableAddRem(this)
{
    setShrinkAllowed( false );

    uiGroup* leftgrp = new uiGroup( this, "Left" );

    coltablistfld_ = new uiListView( leftgrp, "Colortable Manager" );
    BufferStringSet labels;
    labels.add( "Color table" ).add( "Status" );
    coltablistfld_->addColumns( labels );
    coltablistfld_->setRootDecorated( false );
    coltablistfld_->setHScrollBarMode( uiListView::AlwaysOff );
    coltablistfld_->setStretch( 2, 2 );
    coltablistfld_->setSelectionBehavior( uiListView::SelectRows );
    coltablistfld_->selectionChanged.notify( mCB(this,uiColorTableMan,selChg) );

    uiGroup* rightgrp = new uiGroup( this, "Right" );

    uiFunctionDisplay::Setup su;
    su.annotx(false).annoty(false).border(uiBorder(0,0,0,2))
      .xrg(Interval<float>(0,1)).yrg(Interval<float>(0,255))
      .canvaswidth(mTransWidth).canvasheight(mTransHeight)
      .ycol(Color(255,0,0)).y2col(Color(190,190,190))
      .fillbelowy2(true).editable(true).pointsz(3).ptsnaptol(0.08);
    cttranscanvas_ = new uiFunctionDisplay( rightgrp, su );
    cttranscanvas_->pointChanged.notify( mCB(this,uiColorTableMan,transptChg) );
    cttranscanvas_->pointSelected.notify( mCB(this,uiColorTableMan,transptSel));
    cttranscanvas_->setStretch( 0, 0 );

    markercanvas_ = new uiColTabMarkerCanvas( rightgrp, ctab_ );
    markercanvas_->setPrefWidth( mTransWidth );
    markercanvas_->setPrefHeight( mTransWidth/15 );
    markercanvas_->setStretch( 0, 0 );
    markercanvas_->attach( alignedBelow, cttranscanvas_ );

    ctabcanvas_ = new uiColorTableCanvas( rightgrp, ctab_, false, false );
    ctabcanvas_->getMouseEventHandler().buttonPressed.notify( 
        	    mCB(this,uiColorTableMan,rightClick) );
    ctabcanvas_->reSize.notify( mCB(this,uiColorTableMan,reDraw) );
    w2uictabcanvas_ = new uiWorld2Ui( uiWorldRect(0,255,1,0),
	         uiSize(mTransWidth,mTransWidth/10) );
    ctabcanvas_->attach( alignedBelow, markercanvas_, 0 );
    ctabcanvas_->setPrefWidth( mTransWidth );
    ctabcanvas_->setPrefHeight( mTransWidth/10 );

    BufferString lbl = "Segmentize";
    segmentfld_ = new uiCheckBox( rightgrp, lbl );
    segmentfld_->setChecked( markercanvas_->isSegmentized() );
    segmentfld_->activated.notify( mCB(this,uiColorTableMan,segmentSel) );
    segmentfld_->attach( leftAlignedBelow, ctabcanvas_ );
    nrsegbox_ = new uiSpinBox( rightgrp, 0, lbl );
    nrsegbox_->setInterval( 2, 25 ); nrsegbox_->setValue( 8 );
    nrsegbox_->setSensitive( false );
    nrsegbox_->valueChanging.notify( mCB(this,uiColorTableMan,nrSegmentsCB) );
    nrsegbox_->attach( rightTo, segmentfld_ );

    undefcolfld_ = new uiColorInput( rightgrp, uiColorInput::
	    Setup( ctab_.undefColor()).lbltxt("Undefined color") );
    undefcolfld_->enableAlphaSetting( true );
    undefcolfld_->colorchanged.notify( mCB(this,uiColorTableMan,undefColSel) );
    undefcolfld_->attach( alignedBelow, nrsegbox_ );
    undefcolfld_->attach( ensureBelow, segmentfld_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

    removebut_ = new uiPushButton( this, "&Remove", false );
    removebut_->activated.notify( mCB(this,uiColorTableMan,removeCB) );
    removebut_->attach( alignedBelow, splitter );

    importbut_ = new uiPushButton( this, "&Import", false );
    importbut_->activated.notify( mCB(this,uiColorTableMan,importColTab) );
    importbut_->attach( centeredRightOf, removebut_ ); 
    
    uiPushButton* savebut = new uiPushButton( this, "&Save as", false );
    savebut->activated.notify( mCB(this,uiColorTableMan,saveCB) );
    savebut->attach( rightTo, importbut_ ); 
    savebut->attach( rightBorder, 0 );

    markercanvas_->markerChanged.notify( mCB(this,
					     uiColorTableMan,markerChange) );
    ctab_.colorChanged.notify( mCB(this,uiColorTableMan,sequenceChange) );
    ctab_.transparencyChanged.notify( mCB(this,uiColorTableMan,sequenceChange));
    finaliseDone.notify( mCB(this,uiColorTableMan,doFinalise) );
}


uiColorTableMan::~uiColorTableMan()
{
    ctab_.colorChanged.remove( mCB(this,uiColorTableMan,sequenceChange) );
    ctab_.transparencyChanged.remove( mCB(this,uiColorTableMan,sequenceChange));

    delete orgctab_;
    delete w2uictabcanvas_;
}


void uiColorTableMan::doFinalise( CallBacker* )
{
    //ctabcanvas_->setPrefWidth( mTransWidth );
    //ctabcanvas_->setPrefHeight( mTransWidth/10 );
    refreshColTabList( ctab_.name() );
    sequenceChange(0);
    toStatusBar( "", 1 );
}


void uiColorTableMan::refreshColTabList( const char* selctnm )
{
    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    coltablistfld_->clear();
    for ( int idx=0; idx<allctnms.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( allctnms.get(idx) );
	if ( seqidx<0 ) continue;
	const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

	BufferString status;
        if ( seq->type() == ColTab::Sequence::System )
	    status = sKeyDefault;
	else if ( seq->type() == ColTab::Sequence::Edited )
	    status = sKeyEdited;
	else
	    status = sKeyOwn;

	uiListViewItem* itm = new uiListViewItem( coltablistfld_,
		uiListViewItem::Setup().label(seq->name()).label(status) );
    }

    uiListViewItem* itm = coltablistfld_->findItem( selctnm, 0, true );
    if ( !itm ) return;

    coltablistfld_->setCurrentItem( itm );
    coltablistfld_->setSelected( itm, true );
    coltablistfld_->ensureItemVisible( itm );
}


void uiColorTableMan::selChg( CallBacker* cb )
{
    const uiListViewItem* itm = coltablistfld_->selectedItem();
    if ( !itm || !ColTab::SM().get(itm->text(0),ctab_) )
	return;

    selstatus_ = itm->text( 1 );
    removebut_->setSensitive( selstatus_ != sKeyDefault );

    markercanvas_->update();
    undefcolfld_->setColor( ctab_.undefColor() );

    TypeSet<float> xvals;
    TypeSet<float> yvals;
    for ( int idx=0; idx<ctab_.transparencySize(); idx++ )
    {
	xvals += ctab_.transparency(idx).x;
	yvals += ctab_.transparency(idx).y;
    }
    cttranscanvas_->setVals( xvals.arr(), yvals.arr(), xvals.size()  );

    delete orgctab_;
    orgctab_ = new ColTab::Sequence( ctab_ );
    issaved_ = true;
    updateSegmentFields();
    tableChanged.trigger();
}


void uiColorTableMan::removeCB( CallBacker* )
{
    if ( selstatus_ == sKeyDefault )
    {
	uiMSG().error( "This is a default colortable and connot be removed" );
	return;
    }

    const char* ctnm = ctab_.name();
    BufferString msg( selstatus_ == sKeyEdited ? "Edited colortable '" 
	    				       : "Own made colortable '" );
    msg += ctnm; msg += "' will be removed";
    if ( selstatus_ == sKeyEdited )
	msg += "\nand replaced by the default";
    msg += ".\n"; msg += "Do you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) )
	return;

    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    int newctabidx = allctnms.indexOf( ctnm );
    const BufferString selctnm = allctnms.get( newctabidx );
    const int toberemovedid = ColTab::SM().indexOf( ctnm );
    ColTab::SM().remove( toberemovedid );
    ColTab::SM().write();
    ColTab::SM().refresh();

    refreshColTabList( selctnm.buf() );
    selChg(0);
    tableAddRem.trigger();
}


void uiColorTableMan::saveCB( CallBacker* )
{
    if (  saveColTab( true ) )
    {
        ColTab::SM().write();
	ColTab::SM().refresh();
    }
    tableAddRem.trigger();
}


bool uiColorTableMan::saveColTab( bool saveas )
{
    BufferString newname = ctab_.name();
    if ( saveas )
    {
	uiGenInputDlg dlg( this, "Colortable name", "Name",
			   new StringInpSpec(newname) );
	if ( !dlg.go() ) return false;
	newname = dlg.text();
    }

    ColTab::Sequence newctab( ctab_ );
    newctab.setName( newname );

    const int newidx = ColTab::SM().indexOf( newname );

    BufferString msg;
    if ( newidx<0 )
	newctab.setType( ColTab::Sequence::User );
    else
    {
	ColTab::Sequence::Type tp = ColTab::SM().get(newidx)->type();
	if ( tp == ColTab::Sequence::System )
	{
	    msg += "The default colortable will be replaced.\n"
		   "Do you wish to continue?\n"
		   "(Default colortable can be recovered by "
		       "removing the edited one)";
	    newctab.setType( ColTab::Sequence::Edited );
	}
	else if ( tp == ColTab::Sequence::User )
	{
	    msg += "Your own made colortable will be replaced\n"
		   "Do you wish to continue?";
	}
	else if ( tp == ColTab::Sequence::Edited )
	{
	    msg += "The Edited colortable will be replaced\n"
		   "Do you wish to continue?";
	}
    }

    if ( !msg.isEmpty() && !uiMSG().askGoOn( msg ) ) 
	return false;

    newctab.setUndefColor( undefcolfld_->color() );

    IOPar* ctpar = new IOPar;
    newctab.fillPar( *ctpar );
    ColTab::SM().set(newctab);
    ColTab::SM().write();
    ColTab::SM().refresh();

    refreshColTabList( newctab.name() );

    delete orgctab_;
    orgctab_ = new ColTab::Sequence( newctab );
    issaved_ = true;
    selChg(0);
    return true;
}


bool uiColorTableMan::acceptOK( CallBacker* )
{
    ctab_.undefColor() = undefcolfld_->color();

    if ( !(ctab_==*orgctab_) )
	issaved_ = false;

    if ( !issaved_ )
	saveColTab( false );

    if ( !issaved_ ) 
	return false;

    return true;
}


bool uiColorTableMan::rejectOK( CallBacker* )
{
    if ( orgctab_ ) ctab_ = *orgctab_;
    tableChanged.trigger();
    return true;
}


void uiColorTableMan::undefColSel( CallBacker* )
{
    ctab_.setUndefColor( undefcolfld_->color() );
    ctabcanvas_->setRGB();
    tableChanged.trigger();
}


void uiColorTableMan::setHistogram( const TypeSet<float>& hist )
{
    TypeSet<float>& myhist = const_cast<TypeSet<float>&>(hist);
    myhist.remove( 0 ); myhist.remove( hist.size()-1 );
    TypeSet<float> x2vals;
    const float step = (float)1/(float)myhist.size();
    for ( int idx=0; idx<myhist.size(); idx++ )
	x2vals += idx*step;
    cttranscanvas_->setY2Vals( x2vals.arr(), myhist.arr(), myhist.size() );
}


void uiColorTableMan::updateSegmentFields()
{
    NotifyStopper ns1( segmentfld_->activated );
    NotifyStopper ns2( nrsegbox_->valueChanging );
    segmentfld_->setChecked( markercanvas_->isSegmentized() );
    nrsegbox_->setSensitive( segmentfld_->isChecked() );

    if ( markercanvas_->isSegmentized() )
	nrsegbox_->setValue( ctab_.size()/2 );
    else
	nrsegbox_->setValue( 8 );
}


void uiColorTableMan::segmentSel( CallBacker* )
{
    nrsegbox_->setSensitive( segmentfld_->isChecked() );
    doSegmentize();
}


void uiColorTableMan::nrSegmentsCB( CallBacker* )
{
    NotifyStopper( ctab_.colorChanged );
    doSegmentize();
}

#define mAddColor(orgidx,newpos) { \
    const Color col = indextbl.colorForIndex( orgidx ); \
    ctab_.setColor( newpos, col.r(), col.g(), col.b() ); }

#define mEps 0.00001

void uiColorTableMan::doSegmentize()
{
    if ( !segmentfld_->isChecked() )
    {
	ColTab::SM().get( orgctab_->name(), ctab_ );
	markercanvas_->update();
	tableChanged.trigger();
	return;
    }

    const int nrseg = nrsegbox_->getValue();
    if ( mIsUdf(nrseg) || nrseg < 2 )
	return;

    NotifyStopper ns( ctab_.colorChanged );
    *orgctab_ = ctab_;
    ctab_.removeAllColors();
    const float step = 1 / (float)(nrseg-1);
    ColTab::IndexedLookUpTable indextbl( *orgctab_, nrseg );
    mAddColor( 0, 0 );
    for ( int idx=0; idx<nrseg-1; idx++ )
    {
	const float newpos = (idx+0.5) * step;
	mAddColor( idx, newpos );
	mAddColor( idx+1, newpos+0.9*mEps );
    }
    mAddColor( nrseg-1, 1 );

    markercanvas_->update();
    ctabcanvas_->setRGB();
    tableChanged.trigger();
}


void uiColorTableMan::rightClick( CallBacker* )
{
    if ( !segmentfld_->isChecked() )
	return;
    if ( ctabcanvas_->getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev =
	ctabcanvas_->getMouseEventHandler().event();
    uiWorldPoint wpt = w2uictabcanvas_->transform( ev.pos() );

    selidx_ = -1;
    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
	if ( ctab_.position(idx) > wpt.x )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( selidx_<0 ) return;
    Color col = ctab_.color( wpt.x );
    if ( selectColor(col,this,"Color selection",false) )
    {
	ctab_.changeColor( selidx_-1, col.r(), col.g(), col.b() );
	ctab_.changeColor( selidx_, col.r(), col.g(), col.b() );
	tableChanged.trigger();
    }

    ctabcanvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTableMan::markerChange( CallBacker* )
{
    updateSegmentFields();
    ctabcanvas_->setRGB();
    sequenceChange( 0 );
}


void uiColorTableMan::reDraw( CallBacker* )
{
    ctabcanvas_->setRGB();
}


void uiColorTableMan::sequenceChange( CallBacker* )
{
    ctabcanvas_->setRGB();
}


void uiColorTableMan::transptChg( CallBacker* )
{
    const int ptidx = cttranscanvas_->selPt();
    const int nrpts = cttranscanvas_->xVals().size();
    if ( ptidx < 0 )
    {
	ctab_.removeTransparencies();
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    Geom::Point2D<float> pt( cttranscanvas_->xVals()[idx],
				     cttranscanvas_->yVals()[idx] );
	    if ( idx==0 && pt.x>0 )
		pt.x = 0;
	    else if ( idx==nrpts-1 && pt.x<1 )
		pt.x = 1;

	    ctab_.setTransparency( pt );
	}
    }
    else
    {
	Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
				 cttranscanvas_->yVals()[ptidx] );
	bool reset = false;
	if ( ptidx==0 && !mIsZero(pt.x,mEps) )
	{
	    pt.x = 0;
	    reset = true;
	}
	else if ( ptidx==nrpts-1 && !mIsZero(pt.x-1,mEps) )
	{
	    pt.x = 1;
	    reset = true;
	}

	if ( reset )
	{
	    TypeSet<float> xvals;
	    TypeSet<float> yvals;
	    for ( int idx=0; idx<ctab_.transparencySize(); idx++ )
	    {
		xvals += ctab_.transparency(idx).x;
		yvals += ctab_.transparency(idx).y;
	    }
	    cttranscanvas_->setVals( xvals.arr(), yvals.arr(), xvals.size() );
	}

	ctab_.changeTransparency( ptidx, pt );
    }
    tableChanged.trigger();
}


void uiColorTableMan::transptSel( CallBacker* )
{
    const int ptidx = cttranscanvas_->selPt();
    const int nrpts = cttranscanvas_->xVals().size();
    if ( ptidx<0 || nrpts == ctab_.transparencySize() )
	return;

    Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
	    		     cttranscanvas_->yVals()[ptidx] );
    ctab_.setTransparency( pt );
    tableChanged.trigger();
}


void uiColorTableMan::importColTab( CallBacker* )
{
    NotifyStopper notifstop( importbut_->activated );
    uiColTabImport dlg( this );
    if ( dlg.go() )
    {
	ColTab::SM().write(false);
	refreshColTabList( dlg.getCurrentSelColTab() );
	tableAddRem.trigger();
	selChg(0);
    }
}
