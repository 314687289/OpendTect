/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.123 2007-12-24 05:31:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "undo.h"
#include "emobject.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "faultseedpicker.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "ioman.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visdataman.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vissurvscene.h"
#include "vistexture3.h"
#include "vistransform.h"
#include "vistransmgr.h"

using namespace MPE;

#define mAddButton(pm,func,tip,toggle) \
    toolbar->addButton( pm, mCB(this,uiMPEMan,func), tip, toggle )

#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );



uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : clickcatcher(0)
    , clickablesceneid(-1)
    , visserv(ps)
    , colbardlg(0)
    , seedpickwason(false)
    , oldactivevol(false)
    , mpeintropending(false)
{
    mDynamicCastGet(uiMainWin*,mw,p)
    mw->addToolBarBreak();
    toolbar = new uiToolBar( p, "Tracking controls" );

    addButtons();

#ifdef USEQT3
    toolbar->setCloseMode( 2 );
    toolbar->setResizeEnabled();
    toolbar->setVerticallyStretchable(false);
#endif
    updateAttribNames();

    EM::EMM().undo().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanechange.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanetrack.notify(
	    		mCB(this,uiMPEMan,trackPlaneTrackCB) );
    engine().trackeraddremove.notify(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    MPE::engine().activevolumechange.notify(
			mCB(this,uiMPEMan,finishMPEDispIntro) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.notify( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.notify(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    seedconmodefld = new uiComboBox( toolbar, "Seed connect mode" );
    seedconmodefld->setToolTip( "Seed connect mode" );
    seedconmodefld->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar->addObject( seedconmodefld );
    toolbar->addSeparator();

    seedidx = mAddButton( "seedpickmode.png", addSeedCB, 
	    		  "Create seed ( key: 'Tab' )", true );
    toolbar->setShortcut(seedidx,"Tab");
    toolbar->addSeparator();

    trackinvolidx = mAddButton( "track_seed.png", trackInVolume,
    				"Auto-track", false );
    toolbar->addSeparator();

    showcubeidx = mAddButton( "trackcube.png", showCubeCB,
	    		      "Show track area", true );
    toolbar->addSeparator();

    moveplaneidx = mAddButton( "moveplane.png", movePlaneCB,
			       "Move track plane", true );
    extendidx = mAddButton( "trackplane.png", extendModeCB,
	    		    "Extend mode", true );
    retrackidx = mAddButton( "retrackplane.png", retrackModeCB, 
	    		     "Retrack mode", true );
    eraseidx = mAddButton( "erasingplane.png", eraseModeCB, 
	    		   "Erase mode", true );
    toolbar->addSeparator();
/*
    mouseeraseridx = mAddButton( "eraser.png", mouseEraseModeCB, 
	    			 "Erase with mouse", true );
    toolbar->addSeparator();
*/

    attribfld = new uiComboBox( toolbar, "Attribute" );
    attribfld->setToolTip( "QC Attribute" );
    attribfld->selectionChanged.notify( mCB(this,uiMPEMan,attribSel) );
    toolbar->addObject( attribfld );

    clrtabidx = mAddButton( "colorbar.png", setColorbarCB,
			    "Set track plane colorbar", true );

    transfld = new uiSlider( toolbar, "Slider", 2 );
    transfld->setOrientation( uiSlider::Horizontal );
    transfld->setMaxValue( 1 );
    transfld->setToolTip( "Transparency" );
    transfld->setStretch( 0, 0 );
    transfld->valueChanged.notify( mCB(this,uiMPEMan,transpChg) );
    toolbar->addObject( transfld );
    toolbar->addSeparator();

    trackforwardidx = mAddButton( "leftarrow.png", trackBackward,
	    			  "Track backward ( key: '[' )", false );
    toolbar->setShortcut(trackforwardidx,"[");
    trackbackwardidx = mAddButton( "rightarrow.png", trackForward,
	    			   "Track forward ( key: ']' )", false );
    toolbar->setShortcut(trackbackwardidx,"]");

    nrstepsbox = new uiSpinBox( toolbar );
    nrstepsbox->setToolTip( "Nr of track steps" );
    nrstepsbox->setMinValue( 1 );
    toolbar->addObject( nrstepsbox );
    toolbar->addSeparator();

    undoidx = mAddButton( "undo.png", undoPush, "Undo", false );
    redoidx = mAddButton( "redo.png", redoPush, "Redo", false );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().undo().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    engine().trackplanechange.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanetrack.remove(
	    		mCB(this,uiMPEMan,trackPlaneTrackCB) );
    engine().trackeraddremove.remove(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    MPE::engine().activevolumechange.remove(
			mCB(this,uiMPEMan,finishMPEDispIntro) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.remove( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
}


void uiMPEMan::deleteVisObjects()
{
    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx]);
	if ( mped )
	{
	    mped->boxDraggerStatusChange.remove(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );
	    visserv->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( clickcatcher )
    {
	if ( clickablesceneid>=0 )
	    visserv->removeObject( clickcatcher->id(), clickablesceneid );

	clickcatcher->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher->unRef();
	clickcatcher = 0;
    }
}


void uiMPEMan::seedClick( CallBacker* )
{
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) 
	return;

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) 
	return;

    const int clickedobject = clickcatcher->info().getObjID();
    if ( clickedobject == -1 )
	return;

    if ( !clickcatcher->info().isLegalClick() )
    {
	visBase::DataObject* dataobj = visserv->getObject( clickedobject );
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, randomdisp, dataobj );

	if ( tracker->is2D() && !clickcatcher->info().getObjLineName() )
	    uiMSG().error( "2D tracking cannot handle picks on 3D lines.");
	else if ( !tracker->is2D() && clickcatcher->info().getObjLineName() )
	    uiMSG().error( "3D tracking cannot handle picks on 2D lines.");
	else if ( randomdisp )
	    uiMSG().error( emobj->getTypeStr(),
			   " tracking cannot handle picks on random lines.");
	else if ( clickcatcher->info().getObjCS().nrZ()==1 &&
		  !clickcatcher->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getTypeStr(), 
			   " tracking cannot handle picks on time slices." );
	return;
    }
	
    const EM::PosID pid = clickcatcher->info().getNode();
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) ) 
    {
	return;
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	const Coord3 pos0 = clickcatcher->info().getPos();
	const Coord3 pos1 = scene->getZScaleTransform()->transformBack( pos0 );
	seedpos = scene->getUTM2DisplayTransform()->transformBack( pos1 );
    }
    else
    {
	seedpos = emobj->getPos(pid);
    }

    mGetDisplays(false);
    const bool trackerisshown = displays.size() && 
			        displays[0]->isDraggerShown();

    if ( tracker->is2D() )
    {
	const MultiID& lset = clickcatcher->info().getObjLineSet();
	const BufferString& lname = clickcatcher->info().getObjLineName();

	const bool lineswitch = lset!=engine.active2DLineSetID() ||
			        lname!=engine.active2DLineName(); 

	// Not allowed to pick on more than one line in wizard stage
	if ( lineswitch && isPickingInWizard() && seedpicker->nrSeeds() )
	    return;

	engine.setActive2DLine( lset, lname );

	RefMan<const Attrib::DataCubes> cached =
	    				clickcatcher->info().getObjData();
	if ( cached )
	{
	    engine.setAttribData( *clickcatcher->info().getObjDataSelSpec(),
				  cached );
	}

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	h2dsp->setLine( lset, lname, clickcatcher->info().getObjLineData() );

	if ( !h2dsp->startSeedPick() )
	    return;
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    return;
	
	CubeSampling newvolume = clickcatcher->info().getObjCS();
	const CubeSampling trkplanecs = engine.trackPlane().boundingBox();

	if ( trackerisshown && trkplanecs.zrg.includes(seedpos.z) && 
	     trkplanecs.hrg.includes( SI().transform(seedpos) ) &&
	     trkplanecs.defaultDir()==newvolume.defaultDir() )
	{
	    newvolume = trkplanecs;
	}
	
	if ( newvolume.isEmpty() )
	    return;
	
	if ( newvolume != engine.activeVolume() )
	{
	    // Not allowed to pick on more than one plane in wizard stage
	    if ( isPickingInWizard() && seedpicker->nrSeeds() )
		return;

	    if ( oldactivevol.isEmpty() )
	    {
		if ( newvolume == trkplanecs )
		    loadPostponedData();
		else
		    engine.swapCacheAndItsBackup();

		oldactivevol = engine.activeVolume();
		oldtrackplane = engine.trackPlane();
	    }

	    NotifyStopper notifystopper( engine.activevolumechange );
	    engine.setActiveVolume( newvolume );
	    notifystopper.restore();

	    const Attrib::SelSpec* clickedas = 
				   clickcatcher->info().getObjDataSelSpec();
	    if ( clickedas && !engine.cacheIncludes(*clickedas,newvolume) )
	    {
		RefMan<const Attrib::DataCubes> clickeddata = 
					    clickcatcher->info().getObjData();
		if ( clickeddata )
		    engine.setAttribData( *clickedas, clickeddata );
	    }

	    for ( int idx=0; idx<displays.size(); idx++ )
		displays[idx]->freezeBoxPosition( true );

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();
	}
    }

    const int currentevent = EM::EMM().undo().currentEventID();
    uiCursor::setOverride( uiCursor::Wait );
    emobj->setBurstAlert( true );
    
    const bool ctrlshiftclicked = clickcatcher->info().isCtrlClicked() &&
				  clickcatcher->info().isShiftClicked();
    if ( pid.objectID()!=-1 )
    {
	if ( ctrlshiftclicked )
	    seedpicker->removeSeed( pid, false, false );
	else if ( clickcatcher->info().isCtrlClicked() )
	    seedpicker->removeSeed( pid, true, true );
	else if ( clickcatcher->info().isShiftClicked() )
	    seedpicker->removeSeed( pid, true, false );
	else
	    seedpicker->addSeed( seedpos, false );
    }
    else
	seedpicker->addSeed( seedpos, ctrlshiftclicked );
    
    emobj->setBurstAlert( false );
    uiCursor::restoreOverride();
    setUndoLevel(currentevent);
    
    if ( !isPickingInWizard() )
	restoreActiveVol();
}


void uiMPEMan::restoreActiveVol()
{
    MPE::Engine& engine = MPE::engine();
    if ( !oldactivevol.isEmpty() ) 
    {
	if ( engine.activeVolume() != oldtrackplane.boundingBox() )
	    engine.swapCacheAndItsBackup();
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume( oldactivevol );
	notifystopper.restore();
	engine.setTrackPlane( oldtrackplane, false );
	engine.unsetOneActiveTracker();
	engine.activevolumechange.trigger();

	mGetDisplays(false);
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->freezeBoxPosition( false );

	oldactivevol.setEmpty();
    }
}


uiToolBar* uiMPEMan::getToolBar() const 
{ 
    return toolbar; 
}


visSurvey::MPEDisplay* uiMPEMan::getDisplay( int sceneid, bool create )
{
    mDynamicCastGet(const visSurvey::Scene*,scene,visserv->getObject(sceneid));
    if ( !scene ) return 0;

    TypeSet<int> displayids;
    visserv->findObject( typeid(visSurvey::MPEDisplay), displayids );

    for ( int idx=0; idx<displayids.size(); idx++ )
    {
	if ( scene->getFirstIdx(displayids[idx]) == -1 )
	    continue;

	visBase::DataObject* dobj = visserv->getObject( displayids[idx] );
	return reinterpret_cast<visSurvey::MPEDisplay*>( dobj );
    }

    if ( !create ) return 0;

    visSurvey::MPEDisplay* mpedisplay = visSurvey::MPEDisplay::create();

    visserv->addObject( mpedisplay, scene->id(), false );
    mpedisplay->setDraggerTransparency( transfld->getValue() );
    mpedisplay->showDragger( toolbar->isOn(extendidx) );

    mpedisplay->boxDraggerStatusChange.notify(
	    mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

    return mpedisplay;
}


void uiMPEMan::updateAttribNames()
{
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( sKeyNoAttrib() );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld->addItem( spec->userRef() );
    }
    attribfld->setCurrentItem( oldsel );

    updateSelectedAttrib();
    attribSel(0);
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSelectedAttrib()
{
    mGetDisplays(false);

    if ( displays.isEmpty() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays[0]->getSelSpecUserRef();
    if ( !userref && !attribspecs.isEmpty() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays[0]->getSelSpecUserRef();
    }
    else if ( userref==sKey::None )
	userref = sKeyNoAttrib();
    
    if ( userref )  	
	attribfld->setCurrentItem( userref );
}


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher && clickcatcher->isOn();
}


bool uiMPEMan::isPickingInWizard() const
{
    return isSeedPickingOn() && !seedconmodefld->sensitive();
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    if ( isSeedPickingOn() == yn )
	return;
    toolbar->turnOn( seedidx, yn );
    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	visserv->setViewMode(false);
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);

	if ( !clickcatcher )
	{
	    TypeSet<int> catcherids;
	    visserv->findObject( typeid(visSurvey::MPEClickCatcher), 
		    		 catcherids );
	    if ( catcherids.size() )
	    {
		visBase::DataObject* dobj = visserv->getObject( catcherids[0] );
	        clickcatcher = 
		    reinterpret_cast<visSurvey::MPEClickCatcher*>( dobj );
	    }
	    else
	    {
		clickcatcher = visSurvey::MPEClickCatcher::create();
	    }
	    clickcatcher->ref();
	    clickcatcher->click.notify(mCB(this,uiMPEMan,seedClick));
	}

	clickcatcher->turnOn( true );
	updateClickCatcher();
	
	const EM::EMObject* emobj = 
	    		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;

    	if ( emobj ) 
	    clickcatcher->setTrackerType( emobj->getTypeStr() );
    }
    else
    {
	MPE::EMSeedPicker* seedpicker = tracker ? 
				        tracker->getSeedPicker(true) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher ) clickcatcher->turnOn( false );

	restoreActiveVol();
    }
    visserv->sendPickingStatusChangeEvent();
}


void uiMPEMan::updateClickCatcher()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( !clickcatcher || selectedids.size()!=1 ) 
	return;

    const int newsceneid = visserv->getSceneID( selectedids[0] );
    if ( newsceneid<0 || newsceneid == clickablesceneid )
	return;
    
    visserv->removeObject( clickcatcher->id(), clickablesceneid );
    visserv->addObject( clickcatcher, newsceneid, false );
    clickablesceneid = newsceneid;
}


void uiMPEMan::boxDraggerStatusChangeCB( CallBacker* cb )
{
    mGetDisplays(false);
    bool ison = false;
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	if ( cb!=displays[idx] )
	    continue;

	ison = displays[idx]->isBoxDraggerShown();
	if ( !ison )
	    displays[idx]->updateMPEActiveVolume();
    }

    toolbar->turnOn( showcubeidx, ison );
}


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = toolbar->isOn( showcubeidx );
    if ( isshown)
	turnSeedPickingOn( false );
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

    toolbar->setToolTip( showcubeidx, isshown ? "Hide track area"
					      : "Show track area" );
}


void uiMPEMan::attribSel( CallBacker* )
{
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();
    if ( trackerisshown && attribfld->currentItem() )
	loadPostponedData();

    uiCursorChanger cursorchanger( uiCursor::Wait );

    if ( !attribfld->currentItem() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	{
	    Attrib::SelSpec spec( 0, Attrib::SelSpec::cNoAttrib() );
	    displays[idx]->setSelSpec( 0, spec );
	    if ( trackerisshown )
		displays[idx]->updateTexture();
	}
    }
    else
    {
	ObjectSet<const Attrib::SelSpec> attribspecs;
	engine().getNeededAttribs( attribspecs );
	for ( int idx=0; idx<attribspecs.size(); idx++ )
	{
	    const Attrib::SelSpec* spec = attribspecs[idx];
	    if ( strcmp(spec->userRef(),attribfld->text()) )
		continue;

	    for ( int idy=0; idy<displays.size(); idy++ )
	    {
		displays[idy]->setSelSpec( 0, *spec );
		if ( trackerisshown )
		    displays[idy]->updateTexture();
	    }
	    break;
	}	
    }

    if ( colbardlg && displays.size() )
    {
	const int coltabid =
	    displays[0]->getTexture() && displays[0]->getTexture()->isOn()
		?  displays[0]->getTexture()->getColorTab().id()
		: -1;

	colbardlg->setColTab( coltabid );
    }

    updateButtonSensitivity();
}


void uiMPEMan::transpChg( CallBacker* )
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->setDraggerTransparency( transfld->getValue() );
}


void uiMPEMan::mouseEraseModeCB( CallBacker* )
{
    mGetDisplays(false)
}


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( toolbar->isOn(seedidx) );
    updateButtonSensitivity(0);
}


void uiMPEMan::seedConnectModeSel( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker )
	return;

    const int oldseedconmode = seedpicker->getSeedConnectMode();
    seedpicker->setSeedConnectMode( seedconmodefld->currentItem() ); 

    if ( seedpicker->doesModeUseSetup() )
    {
	const SectionTracker* sectiontracker = 
	      tracker->getSectionTracker( seedpicker->getSectionID(), true );
	if (  sectiontracker && !sectiontracker->hasInitializedSetup() )
	    visserv->sendShowSetupDlgEvent();
	if (  !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    seedpicker->setSeedConnectMode( oldseedconmode ); 
    }
    
    turnSeedPickingOn( true );
    visserv->setViewMode(false);
    updateButtonSensitivity(0);
}


void uiMPEMan::treeItemSelCB( CallBacker* )
{
    validateSeedConMode();
    updateClickCatcher();
    updateButtonSensitivity(0);
}


void uiMPEMan::validateSeedConMode()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) 
	return;
    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) 
	return;
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker ) return;

    const SectionTracker* sectiontracker = 
			tracker->getSectionTracker( emobj->sectionID(0), true );
    const bool setupavailable = sectiontracker && 
				sectiontracker->hasInitializedSetup();
    if ( setupavailable || !seedpicker->doesModeUseSetup() ) 
	return;

    const int defaultmode = seedpicker->defaultSeedConMode( false ); 
    seedpicker->setSeedConnectMode( defaultmode );
    
    updateButtonSensitivity(0);
}


void uiMPEMan::introduceMPEDisplay()
{
    EMTracker* tracker = getSelectedTracker();
    const EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    validateSeedConMode();

    if ( !SI().has3D() )
	return;
    if ( !seedpicker || !seedpicker->doesModeUseVolume() )
	return;

    toolbar->turnOn( showcubeidx, true );
    showCubeCB(0);
    
    attribfld->setCurrentItem( sKeyNoAttrib() );
    attribSel(0);

    mGetDisplays(false);
    if ( displays.size() && !displays[0]->isDraggerShown() )
    {
	toolbar->turnOn( moveplaneidx, true );
	movePlaneCB(0);
    }

    mpeintropending = true; 
}


void uiMPEMan::finishMPEDispIntro( CallBacker* )
{
    if ( !mpeintropending || !oldactivevol.isEmpty() )
	return;
    mpeintropending = false;
    
    mGetDisplays(false);
    if ( !displays.size() || !displays[0]->isDraggerShown() )
	return;
    
    EMTracker* tracker = getSelectedTracker();
    if ( !tracker)
	tracker = engine().getTracker( engine().highestTrackerID() );

    if ( !tracker || attribfld->currentItem() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    tracker->getNeededAttribs(attribspecs);
    if ( attribspecs.isEmpty() )
    	return;

    attribfld->setCurrentItem( attribspecs[0]->userRef() );
    attribSel(0);
}


void uiMPEMan::loadPostponedData()
{
    visserv->loadPostponedData();
    finishMPEDispIntro( 0 );
}


#define mBurstAlertToAllEMObjects( yn ) \
{ \
    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- ) \
    { \
	const EM::ObjectID oid = EM::EMM().objectID( idx ); \
	EM::EMObject* emobj = EM::EMM().getObject( oid ); \
	emobj->setBurstAlert( yn ); \
    } \
}

void uiMPEMan::undoPush( CallBacker* )
{
    mBurstAlertToAllEMObjects(true);
    if ( !EM::EMM().undo().unDo( 1, true  ) )
	uiMSG().error("Could not undo everything.");
    mBurstAlertToAllEMObjects(false);

    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    mBurstAlertToAllEMObjects(true);
    if ( !EM::EMM().undo().reDo( 1, true ) )
	uiMSG().error("Could not redo everything.");
    mBurstAlertToAllEMObjects(false);

    updateButtonSensitivity(0);
}


MPE::EMTracker* uiMPEMan::getSelectedTracker() 
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv->isLocked(selectedids[0]) )
	return 0;
    const MultiID mid = visserv->getMultiID( selectedids[0] );
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

    return tracker && tracker->isEnabled() ? tracker : 0;
}


void uiMPEMan::updateButtonSensitivity( CallBacker* ) 
{
    //Undo/Redo
    toolbar->setSensitive( undoidx, EM::EMM().undo().canUnDo() );
    toolbar->setSensitive( redoidx, EM::EMM().undo().canReDo() );

    //Seed button
    updateSeedPickState();

    const bool is2d = !SI().has3D();
    const bool isseedpicking = toolbar->isOn(seedidx);
    
    toolbar->setSensitive( extendidx, !is2d );
    toolbar->setSensitive( retrackidx, !is2d );
    toolbar->setSensitive( eraseidx, !is2d );
    toolbar->setSensitive( moveplaneidx, !is2d );
    toolbar->setSensitive( showcubeidx, !is2d); 

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    const bool isinvolumemode = !seedpicker || seedpicker->doesModeUseVolume();
    toolbar->setSensitive( trackinvolidx, !is2d && isinvolumemode );
    
    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    toolbar->setSensitive( trackforwardidx, !is2d && trackerisshown );
    toolbar->setSensitive( trackbackwardidx, !is2d && trackerisshown );
    attribfld->setSensitive( !is2d && trackerisshown );
    transfld->setSensitive( !is2d && trackerisshown );
    nrstepsbox->setSensitive( !is2d && trackerisshown );

    //coltab
    toolbar->setSensitive( clrtabidx, !is2d && trackerisshown && !colbardlg &&
			   attribfld->currentItem()>0 );


    //trackmode buttons
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    toolbar->turnOn( extendidx, trackerisshown && tm==TrackPlane::Extend );
    toolbar->turnOn( retrackidx, trackerisshown && tm==TrackPlane::ReTrack );
    toolbar->turnOn( eraseidx, trackerisshown && tm==TrackPlane::Erase );
    toolbar->turnOn( moveplaneidx, trackerisshown && tm==TrackPlane::Move );
}


#define mAddSeedConModeItems( seedconmodefld, typ ) \
    if ( emobj && emobj->getTypeStr() == EM##typ##TranslatorGroup::keyword ) \
    { \
	for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
	{ \
	    seedconmodefld-> \
		    addItem( typ##SeedPicker::seedConModeText(idx,true) ); \
	} \
	if ( typ##SeedPicker::nrSeedConnectModes()<=0 ) \
	    seedconmodefld->addItem("No seed mode"); \
    }


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    
    toolbar->setSensitive( seedidx, seedpicker );
    seedconmodefld->setSensitive( seedpicker );
    seedconmodefld->empty();

    if ( !seedpicker )
    {
	seedconmodefld->addItem("No seed mode");
	if ( isSeedPickingOn() )
	{
	    turnSeedPickingOn( false );
	    seedpickwason = true;
	}
	return;
    }

    if ( seedpickwason )
    {
	seedpickwason = false;
	turnSeedPickingOn( true );
	if ( isPickingInWizard() )
	    turnSeedPickingOn( false );
    }

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    mAddSeedConModeItems( seedconmodefld, Horizon3D );
    mAddSeedConModeItems( seedconmodefld, Horizon2D );
    mAddSeedConModeItems( seedconmodefld, Fault );

    seedconmodefld->setCurrentItem( seedpicker->getSeedConnectMode() );
}


void uiMPEMan::trackPlaneTrackCB( CallBacker* )
{
    if ( engine().trackPlane().getTrackMode() != TrackPlane::Erase )
	loadPostponedData();
}


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    if ( !engine().nrTrackersAlive() )
    {
	seedpickwason = false;
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);
	showTracker(false);
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }

    updateAttribNames();
}


void uiMPEMan::visObjectLockedCB()
{
    updateButtonSensitivity();
}


void uiMPEMan::trackForward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::trackBackward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::trackInVolume( CallBacker* )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateMPEActiveVolume();
    loadPostponedData();

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode(TrackPlane::Extend);
    updateButtonSensitivity();
   
    NotifyStopper selstopper( EM::EMM().undo().changenotifier );
    uiCursor::setOverride( uiCursor::Wait );
    PtrMan<Executor> exec = engine().trackInVolume();
    if ( exec )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	uiExecutor uiexec( toolbar, *exec );
	if ( !uiexec.go() )
	{
	    if ( engine().errMsg() )
		uiMSG().error( engine().errMsg() );
	}
	setUndoLevel(currentevent);
    }

    uiCursor::restoreOverride();
    engine().setTrackMode(tm);
    updateButtonSensitivity();
}


void uiMPEMan::showTracker( bool yn )
{
    if ( yn && attribfld->currentItem() ) 
	loadPostponedData();

    uiCursor::setOverride( uiCursor::Wait );
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger(yn);
    uiCursor::restoreOverride();
    updateButtonSensitivity();
}


void uiMPEMan::workAreaChgCB( CallBacker* )
{
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateBoxSpace();

    if ( !SI().sampling(true).includes( engine().activeVolume() ) )
    {
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);
	showTracker( false );
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }
}


void uiMPEMan::setColorbarCB(CallBacker*)
{
    if ( colbardlg || !toolbar->isOn(clrtabidx) )
	return;

    mGetDisplays(false);

    if ( displays.size()<1 )
	return;

    const int coltabid = displays[0]->getTexture()
	?  displays[0]->getTexture()->getColorTab().id()
	: -1;

    colbardlg = new uiColorBarDialog( toolbar, coltabid,
				      "Track plane colorbar" );
    colbardlg->winClosing.notify( mCB(this,uiMPEMan,onColTabClosing) );
    colbardlg->go();

    updateButtonSensitivity();
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    toolbar->turnOn( clrtabidx, false );
    colbardlg = 0;

    updateButtonSensitivity();
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = toolbar->isOn( moveplaneidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );
}


void uiMPEMan::extendModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( extendidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Extend : TrackPlane::None );
}


void uiMPEMan::retrackModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( retrackidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::ReTrack : TrackPlane::None );
}


void uiMPEMan::eraseModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( eraseidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Erase : TrackPlane::None );
}


void uiMPEMan::initFromDisplay()
{
    // compatibility for session files where box outside workarea
    workAreaChgCB(0);

    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->boxDraggerStatusChange.notify(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

	if ( idx==0 )
	{
	    transfld->setValue( displays[idx]->getDraggerTransparency() );
	    toolbar->turnOn( showcubeidx, displays[idx]->isBoxDraggerShown() );
	}
    }
    
    bool showtracker = engine().trackPlane().getTrackMode()!=TrackPlane::None;
    if ( !engine().nrTrackersAlive() )
    {
	engine().setTrackMode( TrackPlane::None );
	showtracker = false;
    }

    showTracker( showtracker );
    updateSelectedAttrib();
    updateButtonSensitivity(0);
}


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent != preveventnr )
	    undo.setUserInteractionEnd(currentevent);
}
