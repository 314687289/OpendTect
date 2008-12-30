/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisisosurface.cc,v 1.20 2008-12-30 09:12:55 cvsumesh Exp $";

#include "uivisisosurface.h"

#include "attribdatacubes.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "picksettr.h"
#include "survinfo.h"
#include "uiaxishandler.h"
#include "uibutton.h"
#include "uihistogramdisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistatsdisplay.h"
#include "viscolortab.h"
#include "vismarchingcubessurface.h"
#include "visvolumedisplay.h"


uiVisIsoSurfaceThresholdDlg::uiVisIsoSurfaceThresholdDlg( uiParent* p,
	visBase::MarchingCubesSurface* isosurface,
	visSurvey::VolumeDisplay* vd )
    : uiDlgGroup( p, "Iso surface threshold" )
    , isosurfacedisplay_( isosurface )
    , initiallineitem_( 0 )
    , thresholdlineitem_( 0 )
    , isovallineitem_( 0 )
    , initialvalue_( vd->isoValue( isosurface ) )
    , vd_( vd )
{
    bool fullmode = vd->isFullMode(isosurface);
    modefld_ = new uiGenInput( this, "Mode",
	    BoolInpSpec(true,"Full volume","Seed based") );
    modefld_->setValue( fullmode );
    modefld_->valuechanged.notify( 
	    mCB(this,uiVisIsoSurfaceThresholdDlg,modeChangeCB) );
    
    ioobjselfld_ = new uiIOObjSel( this, *mMkCtxtIOObj(PickSet), "Seeds" );
    ioobjselfld_->setForRead( true );
    MultiID mid = vd->getSeedsID( isosurface );
    if ( !mid.isEmpty() )
    	ioobjselfld_->setInput( mid );

    ioobjselfld_->display( !fullmode );
    ioobjselfld_->attach( alignedBelow, modefld_ );

    aboveisovaluefld_ = new uiGenInput( this, "Seeds value",
	    BoolInpSpec(true,"Above iso-value","Below iso-value") );
    aboveisovaluefld_->display( !fullmode );
    aboveisovaluefld_->setValue( vd->seedAboveIsovalue(isosurface) );
    aboveisovaluefld_->attach( alignedBelow, ioobjselfld_ );
    
    TypeSet<float> histogram;
    if ( vd->getHistogram(0) ) histogram = *vd->getHistogram(0);
    const Interval<float> rg = vd->getColorTab().getInterval();

    uiStatsDisplay::Setup su; su.withtext(false);
    statsdisplay_ = new uiStatsDisplay( this, su );
    statsdisplay_->funcDisp()->setHistogram( histogram, rg );
    statsdisplay_->attach( leftAlignedBelow, aboveisovaluefld_ );
 
    funcDisp().setDragMode( uiGraphicsView::NoDrag );
    funcDisp().scene().getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,mousePressed) );
    funcDisp().scene().getMouseEventHandler().doubleClick.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,doubleClick) );
    funcDisp().reDrawNeeded.notify(
	    mCB( this,uiVisIsoSurfaceThresholdDlg,reDrawCB) );

    thresholdfld_ = new uiGenInput( this, "Iso value",
	    			    FloatInpSpec(initialvalue_) );
    thresholdfld_->setValue( vd->isoValue(isosurface) );
    thresholdfld_->attach( leftAlignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, "Update",
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updatePressed), false );
    updatebutton_->attach( rightOf, thresholdfld_ );
}


uiVisIsoSurfaceThresholdDlg::~uiVisIsoSurfaceThresholdDlg()
{
}


void uiVisIsoSurfaceThresholdDlg::reDrawCB( CallBacker* )
{ drawHistogram(); }


bool uiVisIsoSurfaceThresholdDlg::acceptOK()
{
    const float curval = vd_->isoValue( isosurfacedisplay_ );
    const float fldvalue = thresholdfld_->getfValue();
    const bool fullmode = modefld_->getBoolValue();
    const bool max = aboveisovaluefld_->getBoolValue();
    MultiID mid( 0 );
    if ( ioobjselfld_->ctxtIOObj().ioobj )
	mid = ioobjselfld_->ctxtIOObj().ioobj->key();
    else 
	mid.setEmpty();

    if ( !(fullmode!=vd_->isFullMode(isosurfacedisplay_) ||
	 !mIsZero(curval-fldvalue,(curval+fldvalue)/2000) ||
	 (!fullmode && max!=vd_->seedAboveIsovalue(isosurfacedisplay_)) ||
	 (!fullmode && mid!=vd_->getSeedsID(isosurfacedisplay_)) ) )
	return true;

    vd_->setFullMode( isosurfacedisplay_, fullmode );
    if ( !fullmode )
    {
	vd_->setSeedAboveIsovalue( isosurfacedisplay_, max );
	if ( ioobjselfld_->ctxtIOObj().ioobj )
    	    vd_->setSeedsID( isosurfacedisplay_, mid );
	else if ( vd_->getSeedsID(isosurfacedisplay_).isEmpty() )
	    return false;
    }
    
    updateIsoDisplay( fldvalue );

    return true;
}


bool uiVisIsoSurfaceThresholdDlg::rejectOK()
{
    modefld_->setValue( vd_->isFullMode(isosurfacedisplay_) );
    aboveisovaluefld_->setValue( vd_->seedAboveIsovalue(isosurfacedisplay_) );
    ioobjselfld_->setInput( vd_->getSeedsID(isosurfacedisplay_) );

    return revertChanges();
}


bool uiVisIsoSurfaceThresholdDlg::revertChanges()
{
    const float curvalue = vd_->isoValue( isosurfacedisplay_ );
    const float prec = (curvalue+initialvalue_)/2000;
    if ( !mIsEqual(curvalue,initialvalue_,prec) )
	updateIsoDisplay(initialvalue_);

    return true;
}


void uiVisIsoSurfaceThresholdDlg::updatePressed(CallBacker*)
{
    updateIsoDisplay( thresholdfld_->getfValue() );
}


void uiVisIsoSurfaceThresholdDlg::mousePressed( CallBacker* cb )
{
    handleClick( cb, false );
}


void uiVisIsoSurfaceThresholdDlg::modeChangeCB( CallBacker* )
{
    const bool fullmode = modefld_->getBoolValue();
    vd_->setFullMode( isosurfacedisplay_, fullmode );
    ioobjselfld_->display( !fullmode );
    aboveisovaluefld_->display( !fullmode );
}


void uiVisIsoSurfaceThresholdDlg::doubleClick( CallBacker* cb )
{
    handleClick( cb, true );
}


uiHistogramDisplay& uiVisIsoSurfaceThresholdDlg::funcDisp()
{
    return *statsdisplay_->funcDisp();
}


uiAxisHandler& uiVisIsoSurfaceThresholdDlg::xAxis()
{
    return *statsdisplay_->funcDisp()->xAxis();
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = funcDisp().scene().getMouseEventHandler();
    if ( eventhandler.isHandled() )
	return;

    const MouseEvent& event = eventhandler.event();
    if ( !event.leftButton() || event.rightButton() || event.middleButton() ||
	 event.ctrlStatus() || event.altStatus() || event.shiftStatus() )
	return;

    const uiPoint& pt = event.pos();

    eventhandler.setHandled( true );
    const float val = xAxis().getVal( pt.x );
    thresholdfld_->setValue( val );
    if ( isdouble )
	updateIsoDisplay( val );
    else
	drawHistogram();
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    MouseCursorChanger changer( MouseCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
    drawHistogram();
}


void uiVisIsoSurfaceThresholdDlg::drawHistogram()
{
    LineStyle ls;
    ls.width_ = 2;
    const uiAxisHandler* yaxis = funcDisp().yAxis(false);
    const int valytop = yaxis->getPix( yaxis->range().start );
    const int valybottom = yaxis->getPix( yaxis->range().stop );
    if ( !mIsUdf(initialvalue_) )
    {
	ls.color_ = Color(0,150,0);
	const int valx = xAxis().getPix(initialvalue_);
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;	    
	if ( !initiallineitem_ )
	{
	    initiallineitem_ =
		funcDisp().scene().addLine( valx, valytop, valx, valybottom );
	    initiallineitem_->setPenStyle( ls );
	    initiallineitem_->setZValue( 3 );
	}
	else
	    initiallineitem_->setLine( valx, valytop, valx, valybottom );
    }

    if ( !mIsUdf(thresholdfld_->getfValue()) )
    {
	ls.color_ = Color(0,255,0,0); 
	const int valx = xAxis().getPix(thresholdfld_->getfValue());
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;	    
	if ( !thresholdlineitem_ )
	{
	    thresholdlineitem_ =
		funcDisp().scene().addLine( valx, valytop, valx, valybottom );
	    thresholdlineitem_->setPenStyle( ls );
	    thresholdlineitem_->setZValue( 3 );
	}
	else
	    thresholdlineitem_->setLine( valx, valytop, valx, valybottom );
    }

    if ( !mIsUdf(vd_->isoValue(isosurfacedisplay_) ) )
    {
	ls.color_ = Color(255,0,0,0);
	const int valx = xAxis().getPix( vd_->isoValue( isosurfacedisplay_) );
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;	    
	if ( !isovallineitem_ )
	{
	    isovallineitem_ =
		funcDisp().scene().addLine( valx, valytop, valx, valybottom );
	    isovallineitem_->setPenStyle( ls );
	    isovallineitem_->setZValue( 2 );
	}
	else
	    isovallineitem_->setLine( valx, valytop, valx, valybottom );
    }
}
