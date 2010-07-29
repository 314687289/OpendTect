/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: horflatvieweditor2d.cc,v 1.2 2010-07-29 12:03:17 cvsumesh Exp $
________________________________________________________________________

-*/

#include "horflatvieweditor2d.h"

#include "attribstorprovider.h"
#include "flatposdata.h"
#include "emhorizon2d.h"
#include "emhorizonpainter2d.h"
#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "horizon2dseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "linesetposinfo.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "posinfo.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "seis2dline.h"
#include "undo.h"

#include "uimsg.h"
#include "uiworld2ui.h"

namespace MPE
{
    
HorizonFlatViewEditor2D::HorizonFlatViewEditor2D( FlatView::AuxDataEditor* ed,
						  const EM::ObjectID& emid )
    : editor_(ed)
    , emid_(emid)
    , horpainter_( new EM::HorizonPainter2D(ed->viewer(),emid) )
    , mehandler_(0)
    , vdselspec_(0)
    , wvaselspec_(0)
    , linenm_(0)
    , lsetid_(-1)
    , seedpickingon_(false)
    , trackersetupactive_(false)
    , updseedpkingstatus_(this)
{
    curcs_.setEmpty();
    editor_->movementFinished.notify(
	    mCB(this,HorizonFlatViewEditor2D,movementEndCB) );
    editor_->removeSelected.notify(
	    mCB(this,HorizonFlatViewEditor2D,removePosCB) );
}


HorizonFlatViewEditor2D::~HorizonFlatViewEditor2D()
{
    setMouseEventHandler( 0 );
    delete horpainter_;
}


void HorizonFlatViewEditor2D::setCubeSampling( const CubeSampling& cs )
{
    curcs_ = cs;
    horpainter_->setCubeSampling( cs ); 
}


void HorizonFlatViewEditor2D::setLineName( const char* lnm )
{
    linenm_ = lnm;
    horpainter_->setLineName( lnm );
}


TypeSet<int>& HorizonFlatViewEditor2D::getPaintingCanvTrcNos()
{
    return horpainter_->getTrcNos();
}


TypeSet<float>& HorizonFlatViewEditor2D::getPaintingCanDistances()
{
    return horpainter_->getDistances();
}


void HorizonFlatViewEditor2D::enableLine( bool yn )
{
    horpainter_->enableLine( yn );
}


void HorizonFlatViewEditor2D::enableSeed( bool yn )
{
    horpainter_->enableSeed( yn );
}


void HorizonFlatViewEditor2D::paint()
{
    horpainter_->paint();
}


void HorizonFlatViewEditor2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( !wva )
	vdselspec_ = as;
    else
	wvaselspec_ = as;
}


void HorizonFlatViewEditor2D::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
    }

    mehandler_ = meh;

    if ( mehandler_ )
    {
	editor_->removeSelected.notify(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.notify(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );

	if ( MPE::engine().getTrackerByObject(emid_) != -1 )
	{
	    int trackeridx = MPE::engine().getTrackerByObject( emid_ );
	    
	    if ( MPE::engine().getTracker(trackeridx) )
		MPE::engine().setActiveTracker( emid_ );
	}
	else
	    MPE::engine().setActiveTracker( -1 );
    }
}


void HorizonFlatViewEditor2D::setSeedPicking( bool yn )
{ seedpickingon_ = yn; }


void HorizonFlatViewEditor2D::mouseMoveCB( CallBacker* )
{
    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
    {
	int trackeridx = MPE::engine().getTrackerByObject( emid_ );
	if ( MPE::engine().getTracker(trackeridx) )
	    MPE::engine().setActiveTracker( emid_ );
    }
    else
	MPE::engine().setActiveTracker( -1 );
}


void HorizonFlatViewEditor2D::mousePressCB( CallBacker* )
{
}


void HorizonFlatViewEditor2D::mouseReleaseCB( CallBacker* )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	    || editor_->isSelActive() )
	return;

    if ( !seedpickingon_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker ) return;

    if ( tracker->objectID() != emid_ ) return;

    if ( !tracker->is2D() ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);

    if ( !seedpicker || !seedpicker->canAddSeed() ) return;

    if ( !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    bool pickinvd = true;

    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const FlatDataPack* dp = editor_->viewer().pack( !pickinvd );
    if ( !dp ) return;

    const MouseEvent& mouseevent = mehandler_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 clickedcrd = dp->getCoord( ix.nearest_, iy.nearest_ );
    clickedcrd.z = wp.y;

    if ( !prepareTracking(pickinvd,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );

    doTheSeed( *seedpicker, clickedcrd, mouseevent );

    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();

    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
	EM::EMM().undo().setUserInteractionEnd(currentevent);
}


bool HorizonFlatViewEditor2D::checkSanity( EMTracker& tracker,
					   const EMSeedPicker& spk,
					   bool& pickinvd ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    const Attrib::SelSpec* atsel = 0;

    const MPE::SectionTracker* sectiontracker =
	tracker.getSectionTracker(emobj->sectionID(0), true);

    const Attrib::SelSpec* trackedatsel = sectiontracker
			? sectiontracker->adjuster()->getAttributeSel(0)
			: 0;

    Attrib::SelSpec newatsel;

    if ( trackedatsel )
    {
	newatsel = *trackedatsel;

	if ( matchString(Attrib::StorageProvider::attribName(),
		    	 trackedatsel->defString()) )
	{
	    LineKey lk( trackedatsel->userRef() );
	    newatsel.setUserRef( lk.attrName().isEmpty()
		    		 ? LineKey::sKeyDefAttrib()
				 : lk.attrName().buf() );
	}
    }

    if ( spk.nrSeeds() < 1 )
    {
	if ( editor_->viewer().pack(false) && editor_->viewer().pack(true) )
	{
	    if ( !uiMSG().question("Which one is your seed data.",
				   "VD", "Wiggle") )
		pickinvd = false;
	}
	else if ( editor_->viewer().pack(false) )
	    pickinvd = true;
	else if ( editor_->viewer().pack(true) )
	    pickinvd = false;
	else
	{
	    uiMSG().error( "No data to choose from" );
	    return false;
	}

	atsel = pickinvd ? vdselspec_ : wvaselspec_;

	if ( !trackersetupactive_ && atsel && trackedatsel &&
	     (newatsel!=*atsel) &&
	     (spk.getSeedConnectMode()!=spk.DrawBetweenSeeds) )
	{
	    uiMSG().error( "Saved setup has different attribute. \n"
			   "Either change setup attribute or change\n"
			   "display attribute you want to track on" );
	    return false;
	}
    }
    else
    {
	if ( vdselspec_ && trackedatsel && (newatsel==*vdselspec_) )
	    pickinvd = true;
	else if ( wvaselspec_ && trackedatsel && (newatsel==*wvaselspec_) )
	    pickinvd = false;
	else if ( spk.getSeedConnectMode() !=spk.DrawBetweenSeeds )
	{
	    BufferString warnmsg( "Setup suggests tracking is done on '" );
	    warnmsg.add( newatsel.userRef() ).add( "'.\n" )
		   .add(  "But what you see is: '" );
	    if ( vdselspec_ && pickinvd )
		warnmsg += vdselspec_->userRef();
	    else if ( wvaselspec_ && !pickinvd )
		warnmsg += wvaselspec_->userRef();
	    warnmsg.add( "'.\n" )
		   .add( "To continue seed picking either " )
		   .add( "change displayed attribute or\n" )
		   .add( "change input data in Tracking Setup." );

	    uiMSG().error( warnmsg.buf() );
	    return false;
	}
    }

    return true;
}


bool HorizonFlatViewEditor2D::prepareTracking( bool picinvd, 
					       const EMTracker& trker,
					       EMSeedPicker& seedpicker,
					       const FlatDataPack& dp ) const
{
    const Attrib::SelSpec* as = 0;
    as = picinvd ? vdselspec_ : wvaselspec_;

    if ( !seedpicker.startSeedPick() )
	return false;

    MPE::engine().setActive2DLine( lsetid_, linenm_ );
    mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, &seedpicker );
    if ( h2dsp )
	h2dsp->setSelSpec( as );

    if ( dp.id() > DataPack::cNoID() )
	MPE::engine().setAttribData( *as, dp.id() );
    
    if ( !h2dsp || !h2dsp->canAddSeed(*as) )
	return false;
    
    h2dsp->setLine( lsetid_, linenm_ );
    if ( !h2dsp->startSeedPick() )
	return false;
    
    return true;
}


bool HorizonFlatViewEditor2D::doTheSeed(EMSeedPicker& spk, const Coord3& crd,
					const MouseEvent& mev ) const
{
    EM::PosID pid;
    bool posidavlble = getPosID( crd, pid );

    const bool ctrlshiftclicked =  mev.ctrlStatus() && mev.shiftStatus();
    bool addseed = !posidavlble || ( posidavlble && !mev.ctrlStatus() &&
	    			     !mev.shiftStatus() );

    if ( addseed )
    {
	if ( spk.addSeed(crd,posidavlble ? false : ctrlshiftclicked) )
	    return true;
    }
    else
    {
	bool env = false;
	bool retrack = false;

	if ( mev.ctrlStatus() )
	{
	    env = true;
	    retrack = true;
	}
	else if ( mev.shiftStatus() )
	    env = true;

	if ( spk.removeSeed(pid,env,retrack) )
	    return false;
    }

    return true;
}


bool HorizonFlatViewEditor2D::getPosID( const Coord3& crd,
					EM::PosID& pid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    PtrMan<IOObj> ioobj = IOM().get( lsetid_ );
    if ( !ioobj ) return false;

    const Seis2DLineSet lset( ioobj->fullUserExpr(true) );
    PosInfo::LineSet2DData linesetgeom;
    if ( !lset.getGeometry(linesetgeom) )
	return false;

    PosInfo::Line2DPos pos;
    linesetgeom.getLineData( linenm_ )->getPos( crd, pos, mDefEps );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);

    if ( !hor2d ) return false;

    BinID bid;
    bid.inl = hor2d->geometry().lineIndex( linenm_ );
    bid.crl = pos.nr_;

    for ( int idx=0; idx<emobj->nrSections(); idx++ )
    {
	if ( emobj->isDefined(emobj->sectionID(idx),bid.getSerialized()) )
	{
	    pid.setObjectID( emobj->id() );
	    pid.setSectionID( emobj->sectionID(idx) );
	    pid.setSubID( bid.getSerialized() );
	    return true;
	}
    }

    return false;
}


void HorizonFlatViewEditor2D::movementEndCB( CallBacker* )
{}


void HorizonFlatViewEditor2D::removePosCB( CallBacker* )
{}

}//namespace MPE
