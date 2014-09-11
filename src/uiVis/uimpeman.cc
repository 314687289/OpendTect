/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimpeman.h"

#include "attribstorprovider.h"
#include "coltabsequence.h"
#include "emobject.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emundo.h"
#include "executor.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "pixmap.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "selector.h"
#include "survinfo.h"

#include "uicolortable.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "visemobjdisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vistransform.h"
#include "vistransmgr.h"

using namespace MPE;

#define mAddButton(pm,func,tip,toggle) \
    toolbar->addButton( pm, tip, mCB(this,uiMPEMan,func), toggle )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiMPEMan,fn) ); \
    mnu->insertItem( itm, idx ); itm->setIcon( uiPixmap(fnm) ); }


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
    , propdlg_(0)
    , seedpickwason(false)
    , oldactivevol(false)
    , mpeintropending(false)
    , showtexture_(true)
    , cureventnr_(mUdf(int))
    , polyselstoppedseedpick(false)
{
    toolbar = new uiToolBar( p, "Tracking controls", uiToolBar::Bottom );
    addButtons();

    EM::EMM().undo().undoredochange.notify(
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
    visserv->selectionmodechange.notify( mCB(this,uiMPEMan,selectionMode) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    mAddButton( "tools", showSettingsCB, "Settings", false );

    seedconmodefld = new uiComboBox( toolbar, "Seed connect mode" );
    seedconmodefld->setToolTip( tr("Seed connect mode") );
    seedconmodefld->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar->addObject( seedconmodefld );
    toolbar->addSeparator();

    seedidx = mAddButton( "seedpickmode", addSeedCB,
			  tr("Create seed ( key: 'Tab' )"), true );
    toolbar->setShortcut( seedidx, "Tab" );

    trackinvolidx = mAddButton( "autotrack", trackFromSeedsAndEdges,
				tr("Auto-track"), false );

    trackwithseedonlyidx = mAddButton( "trackfromseeds", trackFromSeedsOnly,
				       tr("Track From Seeds Only"), false );

    retrackallinx = mAddButton( "retrackhorizon", retrackAllCB,
				tr("Retrack All"), false );
    toolbar->addSeparator();

    showcubeidx = mAddButton( "trackcube", showCubeCB,
			      tr("Show track area"), true );
    uiMenu* cubemnu = new uiMenu( toolbar, "CubeMenu" );
    mAddMnuItm( cubemnu, tr("Position ..."), setCubePosCB, "orientation64", 0 );
    toolbar->setButtonMenu( showcubeidx, cubemnu );

    moveplaneidx = mAddButton( "QCplane-inline", movePlaneCB,
			       tr("Display QC plane"), true );
    uiMenu* mnu = new uiMenu( toolbar, tr("Menu") );
    mAddMnuItm( mnu, uiStrings::sInline(), handleOrientationClick,
		"QCplane-inline", 0 );
    mAddMnuItm( mnu, uiStrings::sCrossline(), handleOrientationClick,
		"QCplane-crossline", 1 );
    mAddMnuItm( mnu, tr("Z"), handleOrientationClick, "QCplane-z", 2 );
    toolbar->setButtonMenu( moveplaneidx, mnu );

    displayatsectionidx = mAddButton( "sectiononly", displayAtSectionCB,
				      tr("Display at section only"), true );

    nrstepsbox = new uiSpinBox( toolbar, 0, "QC plane step" );
    nrstepsbox->setToolTip( tr("QC plane step") );
    nrstepsbox->setMinValue( 1 );
    toolbar->addObject( nrstepsbox );
    trackforwardidx = mAddButton( "prevpos", moveBackward,
				  tr("Move QC plane backward (key: '[')"),
                                  false );
    toolbar->setShortcut(trackforwardidx,"[");
    trackbackwardidx = mAddButton( "nextpos", moveForward,
				   tr("Move QC plane forward (key: ']')"),
                                   false );
    toolbar->setShortcut(trackbackwardidx,"]");
    clrtabidx = mAddButton( "colorbar", setColorbarCB,
			    tr("Set QC plane colortable"), false );
    toolbar->addSeparator();

    polyselectidx =  mAddButton( "polygonselect", selectionMode,
				 tr("Polygon Selection mode"), true );
    uiMenu* polymnu = new uiMenu( toolbar, "PolyMenu" );
    mAddMnuItm( polymnu,uiStrings::sPolygon(), handleToolClick, "polygonselect",
                0 );
    mAddMnuItm( polymnu,uiStrings::sRectangle(),handleToolClick,
                "rectangleselect", 1 );
    toolbar->setButtonMenu( polyselectidx, polymnu );

    removeinpolygon = mAddButton( "trashcan", removeInPolygon,
				  tr(" Remove PolySelection"), false );
    toolbar->addSeparator();

    undoidx = mAddButton( "undo", undoPush, uiStrings::sUndo(), false );
    redoidx = mAddButton( "redo", redoPush, uiStrings::sRedo(), false );

    toolbar->addSeparator();
    mAddButton( "save", savePush, uiStrings::sSave(true), false );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().undo().undoredochange.remove(
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
    visserv->selectionmodechange.remove( mCB(this,uiMPEMan,selectionMode) );
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
	    mped->planeOrientationChange.remove(
		mCB(this,uiMPEMan,planeOrientationChangedCB) );
	    visserv->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( clickcatcher )
    {
	if ( clickablesceneid>=0 )
	    visserv->removeObject( clickcatcher->id(), clickablesceneid );

	clickcatcher->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher->setEditor( 0 );
	clickcatcher->unRef();
	clickcatcher = 0;
	clickablesceneid = -1;
    }
}


#define mSeedClickReturn() \
{ endSeedClickEvent(emobj);  return; }

void uiMPEMan::seedClick( CallBacker* )
{
    EM::EMObject* emobj = 0;
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	mSeedClickReturn();

    emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	mSeedClickReturn();

    const int trackerid =
		MPE::engine().getTrackerByObject( tracker->objectID() );

    const int clickedobject = clickcatcher->info().getObjID();
    if ( clickedobject == -1 )
	mSeedClickReturn();

    if ( !clickcatcher->info().isLegalClick() )
    {
	visBase::DataObject* dataobj = visserv->getObject( clickedobject );
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, randomdisp, dataobj );

	if ( tracker->is2D() && !clickcatcher->info().getObjLineName() )
	    uiMSG().error( tr("2D tracking cannot handle picks on 3D lines.") );
	else if ( !tracker->is2D() && clickcatcher->info().getObjLineName() )
	    uiMSG().error( tr("3D tracking cannot handle picks on 2D lines.") );
	else if ( randomdisp )
	    uiMSG().error( emobj->getTypeStr(),
			   "Tracking cannot handle picks on random lines." );
	else if ( clickcatcher->info().getObjCS().nrZ()==1 &&
		  !clickcatcher->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getTypeStr(),
			   "Tracking cannot handle picks on time slices." );
	mSeedClickReturn();
    }

    const EM::PosID pid = clickcatcher->info().getNode();
    CubeSampling newvolume;
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	mSeedClickReturn();

    const Attrib::SelSpec* clickedas =
	clickcatcher->info().getObjDataSelSpec();
    if ( !clickedas )
	mSeedClickReturn();

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
    {
	mSeedClickReturn();
    }

    const MPE::SectionTracker* sectiontracker =
	tracker->getSectionTracker(emobj->sectionID(0), true);
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0)
	: 0;

    if ( !visserv->isTrackingSetupActive() && (seedpicker->nrSeeds()==0) )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		uiMSG().error( tr("Saved setup has different attribute. \n"
			       "Either change setup attribute or change\n"
			       "display attribute you want to track on") );
		mSeedClickReturn();
	    }
	}
    }

    if ( seedpicker->nrSeeds() > 0 )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		BufferString warnmsg( "Setup suggests tracking is done on: '",
				      trackedatsel->userRef(), "'\n" );
		warnmsg.add( "but what you see is: '" )
		       .add( clickedas->userRef() ).add( "'.\n" )
		       .add( "To continue seed picking either " )
		       .add( "change displayed attribute or\n" )
		       .add( "change input data in Tracking Setup." );
		uiMSG().error( warnmsg.buf() );
		mSeedClickReturn();
	    }
	}
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	seedpos = clickcatcher->info().getPos();
	scene->getTempZStretchTransform()->transformBack( seedpos );
	scene->getUTM2DisplayTransform()->transformBack( seedpos );
    }
    else
    {
	seedpos = emobj->getPos(pid);
    }

    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();

    bool ctrlshiftclicked = clickcatcher->info().isCtrlClicked() &&
			    clickcatcher->info().isShiftClicked();

    if ( pid.objectID()==-1 && !ctrlshiftclicked &&
	 clickcatcher->activateSower(emobj->preferredColor(),
				     seedpicker->getSeedPickArea()) )
    {
	 mSeedClickReturn();
    }

    if ( tracker->is2D() )
    {
	Pos::GeomID geomid = clickcatcher->info().getGeomID();
	engine.setActive2DLine( geomid );

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	DataPack::ID datapackid = clickcatcher->info().getObjDataPackID();

	if ( clickedas && h2dsp )
	    h2dsp->setSelSpec( clickedas );

	if ( !clickedas || !h2dsp || !h2dsp->canAddSeed(*clickedas) )
	{
	    uiMSG().error( tr("2D tracking requires attribute from setup "
			   "to be displayed") );
	    mSeedClickReturn();
	}
	if ( datapackid > DataPack::cNoID() )
	    engine.setAttribData( *clickedas, datapackid );

	h2dsp->setLine( geomid );
	if ( !h2dsp->startSeedPick() )
	    mSeedClickReturn();
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    mSeedClickReturn();

	newvolume = clickcatcher->info().getObjCS();
	const CubeSampling trkplanecs = engine.trackPlane().boundingBox();

	if ( trackerisshown && trkplanecs.zrg.includes(seedpos.z,true) &&
	     trkplanecs.hrg.includes( SI().transform(seedpos) ) &&
	     trkplanecs.defaultDir()==newvolume.defaultDir() )
	{
	    newvolume = trkplanecs;
	}

	if ( newvolume.isEmpty() || !newvolume.isDefined() )
	    mSeedClickReturn();

	if ( newvolume != engine.activeVolume() )
	{
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

	    if ( clickedas && !engine.cacheIncludes(*clickedas,newvolume) )
	    {
		DataPack::ID datapackid =
				clickcatcher->info().getObjDataPackID();
		if ( datapackid > DataPack::cNoID() )
		    engine.setAttribData( *clickedas, datapackid );
	    }

	    seedpicker->setSelSpec( clickedas );

	    for ( int idx=0; idx<displays.size(); idx++ )
		displays[idx]->freezeBoxPosition( true );  // to do:

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();
	}
    }

    seedpicker->setSowerMode( clickcatcher->sequentSowing() );
    if ( mIsUdf(cureventnr_) && clickcatcher->moreToSow() )
	ctrlshiftclicked = true;  // 1st seed sown is "tracking buffer" only

    beginSeedClickEvent( emobj );

    if ( pid.objectID()!=-1 )
    {
	if ( ctrlshiftclicked )
	{
	    if ( seedpicker->removeSeed( pid, false, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( clickcatcher->info().isCtrlClicked() )
	{
	    if ( seedpicker->removeSeed( pid, true, true ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( clickcatcher->info().isShiftClicked() )
	{
	    if ( seedpicker->removeSeed( pid, true, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else
	{
	    if ( seedpicker->addSeed( seedpos, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    else if ( seedpicker->addSeed(seedpos, ctrlshiftclicked) )
	    engine.updateFlatCubesContainer( newvolume, trackerid, true );

    if ( !clickcatcher->moreToSow() )
	endSeedClickEvent( emobj );
}


void uiMPEMan::beginSeedClickEvent( EM::EMObject* emobj )
{
    if ( mIsUdf(cureventnr_) )
    {
	cureventnr_ = EM::EMM().undo().currentEventID();
	MouseCursorManager::setOverride( MouseCursor::Wait );
	if ( emobj )
	    emobj->setBurstAlert( true );
    }
}


void uiMPEMan::endSeedClickEvent( EM::EMObject* emobj )
{
    clickcatcher->stopSowing();

    if ( !mIsUdf(cureventnr_) )
    {
	if ( emobj )
	    emobj->setBurstAlert( false );

	MouseCursorManager::restoreOverride();
	setUndoLevel( cureventnr_ );
	cureventnr_ = mUdf(int);
    }

    restoreActiveVol();
}


void uiMPEMan::restoreActiveVolume()
{
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

    visSurvey::MPEDisplay* mpedisplay = new visSurvey::MPEDisplay;

    visserv->addObject( mpedisplay, scene->id(), false ); // false or true?
    mpedisplay->setDraggerTransparency( 0 ); // to do: check 0
    mpedisplay->showDragger( toolbar->isOn(moveplaneidx) );
    mpedisplay->setColTabSequence( 0, ColTab::Sequence(
			    ColTab::defSeqName() ), 0 );

    mpedisplay->boxDraggerStatusChange.notify(
	    mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

    mpedisplay->planeOrientationChange.notify(
	    mCB(this,uiMPEMan,planeOrientationChangedCB) );

    return mpedisplay;
}


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher && clickcatcher->isOn();
}


bool uiMPEMan::isPickingWhileSetupUp() const
{
    return isSeedPickingOn() &&
	( visserv->isTrackingSetupActive() );
}


void uiMPEMan::updateOldActiveVol()
{
    if ( oldactivevol.isEmpty() )
    {
	MPE::engine().swapCacheAndItsBackup();
	oldactivevol = MPE::engine().activeVolume();
	oldtrackplane = MPE::engine().trackPlane();
    }
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    polyselstoppedseedpick = false;

    if ( !yn && clickcatcher )
	clickcatcher->setEditor( 0 );

    if ( isSeedPickingOn() == yn )
	return;

    toolbar->turnOn( seedidx, yn );
    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	toolbar->turnOn( polyselectidx, false );
	selectionMode(0);

	visserv->setViewMode(false);
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);

	updateClickCatcher();
	clickcatcher->turnOn( true );

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

	if ( clickcatcher )
	    clickcatcher->turnOn( false );

	restoreActiveVol();
    }

    mGetDisplays( false );
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->enablePicking( yn );

    visserv->sendPickingStatusChangeEvent();
}


void uiMPEMan::updateClickCatcher()
{
    if ( !clickcatcher )
    {
	TypeSet<int> catcherids;
	visserv->findObject( typeid(visSurvey::MPEClickCatcher),
			     catcherids );
	if ( catcherids.size() )
	{
	    visBase::DataObject* dobj = visserv->getObject( catcherids[0] );
	    clickcatcher = reinterpret_cast<visSurvey::MPEClickCatcher*>(dobj);
	}
	else
	{
	    clickcatcher = visSurvey::MPEClickCatcher::create();
	}
	clickcatcher->ref();
	clickcatcher->click.notify(mCB(this,uiMPEMan,seedClick));
	clickcatcher->turnOn( false );
    }

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     surface, visserv->getObject(selectedids[0]) );
    clickcatcher->setEditor( surface ? surface->getEditor() : 0 );

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
	    displays[idx]->updateMPEActiveVolume();  // to do
    }

    toolbar->turnOn( showcubeidx, ison );
}


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = toolbar->isOn( showcubeidx );
    if ( isshown)
	turnSeedPickingOn( false );

    if ( isshown )
    {
	bool isvolflat = false;
	CubeSampling cube = MPE::engine().activeVolume();
	if ( cube.isFlat() )
	{
	    cube = MPE::engine().getDefaultActiveVolume();
	    isvolflat = true;
	}

	if ( cube == MPE::engine().getDefaultActiveVolume() )
	{
	    bool seedincluded = false;
	    MPE::EMTracker* tracker = getSelectedTracker();
	    if ( tracker )
	    {
		EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID());
		if ( emobj )
		{
		    const TypeSet<EM::PosID>* seeds =
			emobj->getPosAttribList( EM::EMObject::sSeedNode() );
		    if ( seeds->size() > 0 )
			seedincluded = true;
		    for ( int idx=0; idx<seeds->size(); idx++ )
		    {
			const Coord3 pos = emobj->getPos( (*seeds)[idx] );
			const BinID bid = SI().transform(pos);
			cube.hrg.include(bid);
			cube.zrg.include((float) pos.z);
		    }
		}
	    }

	    if ( isvolflat || seedincluded )
	    {
		NotifyStopper notifystopper( MPE::engine().activevolumechange );
		MPE::engine().setActiveVolume( cube );
		notifystopper.restore();
		visserv->postponedLoadingData();
		MPE::engine().activevolumechange.trigger();
	    }
	}
    }

    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

    toolbar->setToolTip( showcubeidx, isshown ? tr("Hide track area")
					      : tr("Show track area") );
    MPE::engine().setActiveVolShown( isshown );
}



class uiSetCubePosDlg : public uiDialog
{
public:
uiSetCubePosDlg( uiParent* p, const CubeSampling& cs )
    : uiDialog(p,uiDialog::Setup("Set Cube Position",mNoDlgTitle,
                                    mTODOHelpKey))
{
    selfld_ = new uiSelSubvol( this, false );
    selfld_->setSampling( cs );
    fullbut_ = new uiToolButton( this, "exttofullsurv",
				"Set ranges to full survey",
				 mCB(this,uiSetCubePosDlg,fullPush) );
    fullbut_->attach( rightOf, selfld_ );
}

void fullPush( CallBacker* )
{
    selfld_->setSampling( SI().sampling(false) );
}

CubeSampling getSampling() const
{ return selfld_->getSampling(); }

    uiSelSubvol*	selfld_;
    uiToolButton*	fullbut_;
};


void uiMPEMan::setCubePosCB( CallBacker* )
{
    const CubeSampling& cs = MPE::engine().activeVolume();
    uiSetCubePosDlg dlg( toolbar, cs );
    if ( !dlg.go() ) return;

    NotifyStopper notifystopper( MPE::engine().activevolumechange );
    MPE::engine().setActiveVolume( dlg.getSampling() );
    notifystopper.restore();
    visserv->postponedLoadingData();
    MPE::engine().activevolumechange.trigger();
}


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( toolbar->isOn(seedidx) );
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSeedModeSel()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( seedpicker )
	seedconmodefld->setCurrentItem( seedpicker->getSeedConnectMode() );
}


void uiMPEMan::seedConnectModeSel( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !emobj || !seedpicker )
	return;

    const int oldseedconmode = seedpicker->getSeedConnectMode();
    seedpicker->setSeedConnectMode( seedconmodefld->currentItem() );

    if ( seedpicker->doesModeUseSetup() )
    {
	const SectionTracker* sectiontracker =
	      tracker->getSectionTracker( emobj->sectionID(0), true );
	if ( sectiontracker && !sectiontracker->hasInitializedSetup() )
	    visserv->sendShowSetupDlgEvent();
	if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
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
    if ( visserv->isTrackingSetupActive() )
	return;
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

    attribSel(0);

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

    if ( !tracker )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    tracker->getNeededAttribs(attribspecs);
    if ( attribspecs.isEmpty() )
	return;

    attribSel(0);
}


void uiMPEMan::loadPostponedData()
{
    visserv->loadPostponedData();
    finishMPEDispIntro( 0 );
}


void uiMPEMan::undoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo() );
    if ( emundo )
    {
	EM::ObjectID curid = emundo->getCurrentEMObjectID( false );
	EM::EMObject* emobj = EM::EMM().getObject( curid );
	if ( emobj )
        {
            emobj->ref();
	    emobj->setBurstAlert( true );
        }

        if ( !emundo->unDo(1,true) )
	    uiMSG().error( tr("Could not undo everything.") );

	if ( emobj )
        {
	    emobj->setBurstAlert( false );
            emobj->unRef();
        }
    }

    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo() );
    if ( emundo )
    {
	EM::ObjectID curid = emundo->getCurrentEMObjectID( true );
	EM::EMObject* emobj = EM::EMM().getObject( curid );
        if ( emobj )
        {
            emobj->ref();
	    emobj->setBurstAlert( true );
        }

	if ( !emundo->reDo(1,true) )
	    uiMSG().error( tr("Could not redo everything.") );

	if ( emobj )
        {
	    emobj->setBurstAlert( false );
            emobj->unRef();
        }
    }

    updateButtonSensitivity(0);
}


void uiMPEMan::savePush( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;

    visserv->fireFromMPEManStoreEMObject();
}


MPE::EMTracker* uiMPEMan::getSelectedTracker()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv->isLocked(selectedids[0]) )
	return 0;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
				surface, visserv->getObject(selectedids[0]) );
    if ( !surface ) return 0;
    const EM::ObjectID oid = surface->getObjectID();
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( tracker  && tracker->isEnabled() )
	return tracker;

    return 0;
}


#define mAddSeedConModeItems( seedconmodefld, typ ) \
    if ( emobj && EM##typ##TranslatorGroup::keyword() == emobj->getTypeStr() ) \
    { \
	seedconmodefld->setEmpty(); \
	for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
	{ \
	    seedconmodefld-> \
		    addItem( typ##SeedPicker::seedConModeText(idx,true) ); \
	} \
	if ( typ##SeedPicker::nrSeedConnectModes()<=0 ) \
	    seedconmodefld->addItem(tr("No seed mode")); \
    }


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    toolbar->setSensitive( seedidx, seedpicker );
    seedconmodefld->setSensitive( seedpicker );
    seedconmodefld->setEmpty();

    if ( !seedpicker )
    {
	seedconmodefld->addItem(tr("No seed mode"));
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
    }

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    mAddSeedConModeItems( seedconmodefld, Horizon3D );
    mAddSeedConModeItems( seedconmodefld, Horizon2D );

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
}


void uiMPEMan::visObjectLockedCB( CallBacker* )
{
    updateButtonSensitivity();
}


void uiMPEMan::moveForward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::moveBackward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::trackFromSeedsOnly( CallBacker* cb )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateSeedOnlyPropagation( true );

    trackInVolume( cb );
}


void uiMPEMan::trackFromSeedsAndEdges( CallBacker* cb )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateSeedOnlyPropagation( false );

    trackInVolume( cb );
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
    MouseCursorManager::setOverride( MouseCursor::Wait );
    Executor* exec = engine().trackInVolume();
    if ( exec )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	uiTaskRunner uitr( toolbar );
	if ( !TaskRunner::execute( &uitr, *exec ) )
	{
	    if ( engine().errMsg() )
		uiMSG().error( engine().errMsg() );
	}
	delete exec;	// AutoTracker destructor adds the undo event!
	setUndoLevel(currentevent);
    }

    MouseCursorManager::restoreOverride();
    engine().setTrackMode(tm);
    updateButtonSensitivity();
}


static bool sIsPolySelect = true;

void uiMPEMan::selectionMode( CallBacker* cb )
{
    if ( cb == visserv )
    {
	toolbar->turnOn( polyselectidx, visserv->isSelectionModeOn() );
	sIsPolySelect = visserv->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn( toolbar->isOn(polyselectidx) );
	visserv->setSelectionMode( mode );
    }

    toolbar->setIcon( polyselectidx, sIsPolySelect ?
			"polygonselect" : "rectangleselect" );
    toolbar->setToolTip( polyselectidx,
                         sIsPolySelect ? tr("Polygon Selection mode")
                                       : tr("Rectangle Selection mode") );

    if ( toolbar->isOn(polyselectidx) )
    {
	if ( toolbar->isOn(seedidx) )
	{
	    visserv->turnSeedPickingOn( false );
	    polyselstoppedseedpick = true;
	}
    }
    else if ( polyselstoppedseedpick )
	visserv->turnSeedPickingOn( true );

    updateButtonSensitivity(0);
}


void uiMPEMan::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    sIsPolySelect = itm->getID()==0;
    selectionMode( cb );
}


void uiMPEMan::removeInPolygon( CallBacker* cb )
{
    const Selector<Coord3>* sel = visserv->getCoordSelector( clickablesceneid);

    if ( sel && sel->isOK() )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	mGetDisplays(true);
	uiTaskRunner taskrunner( toolbar );
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->removeSelectionInPolygon( *sel, &taskrunner );

	toolbar->turnOn( polyselectidx, false );
	selectionMode( cb );

	toolbar->turnOn( moveplaneidx, false );
	movePlaneCB( cb );
	setUndoLevel( currentevent );
    }
}


void uiMPEMan::showTracker( bool yn )
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger(yn);
    MouseCursorManager::restoreOverride();
    updateButtonSensitivity();
}


void uiMPEMan::changeTrackerOrientation( int orient )
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->setPlaneOrientation( orient );
    MouseCursorManager::restoreOverride();
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


class uiPropertiesDialog : public uiDialog
{
public:
				uiPropertiesDialog(uiMPEMan*);

    uiVisColTabEd&              editor()        { return *coltbl_; }
    int				selAttrib() const
				{ return attribfld_->currentItem(); }
    void			updateAttribNames();

protected:
    void			transpChg(CallBacker*);
    void			colSeqChange(CallBacker*);
    void			colMapperChange(CallBacker*);
    void			updateSelectedAttrib();
    void			updateDisplayList();

    uiVisColTabEd*		coltbl_;
    uiComboBox*			attribfld_;
    uiSlider*			transfld_;
    uiMPEMan*			mpeman_;

    ObjectSet<visSurvey::MPEDisplay> displays_;
};


uiPropertiesDialog::uiPropertiesDialog( uiMPEMan* mpeman )
    : uiDialog( mpeman->getToolBar(),
	    uiDialog::Setup("QC display properties",uiStrings::sEmptyString(),
                            mNoHelpKey).modal(false))
    , mpeman_(mpeman)
{
    updateDisplayList();
    setCtrlStyle( CloseOnly );

    ColTab::Sequence ctseq( "" );
    uiColorTableGroup* coltabgrp =
	new uiColorTableGroup( this, ctseq, false, false );

    coltbl_ = new uiVisColTabEd( *coltabgrp );
    coltbl_->seqChange().notify( mCB(this,uiPropertiesDialog,colSeqChange) );
    coltbl_->mapperChange().notify( mCB(this,uiPropertiesDialog,
					colMapperChange) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "QC Attribute",
						    "QC Attribute" );
    attribfld_ = lcb->box();
    attribfld_->setToolTip( "QC Attribute" );
    attribfld_->selectionChanged.notify( mCB(mpeman_,uiMPEMan,attribSel) );
    lcb->attach( leftAlignedBelow, coltabgrp );

    transfld_ = new uiSlider( this, uiSlider::Setup(uiStrings::sTransparency())
				    .nrdec(2), "Slider" );
    transfld_->setOrientation( OD::Horizontal );
    transfld_->setMaxValue( 1 );
    transfld_->setToolTip( uiStrings::sTransparency() );
    transfld_->setStretch( 0, 0 );
    transfld_->setValue( displays_[0]->getDraggerTransparency() );
    transfld_->valueChanged.notify( mCB(this,uiPropertiesDialog,transpChg) );
    transfld_->attach( alignedBelow, lcb );

    updateAttribNames();
}


void uiPropertiesDialog::updateDisplayList()
{
    displays_.erase();
    TypeSet<int> scenes;
    mpeman_->visserv->getChildIds( -1, scenes );
    for ( int idx=0; idx<scenes.size(); idx++ )
	displays_ += mpeman_->getDisplay( scenes[idx], false );
}


void uiPropertiesDialog::transpChg( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size() < 1 )
	return;

    for ( int idx=0; idx<displays_.size(); idx++ )
	displays_[idx]->setDraggerTransparency( transfld_->getValue() );
}

void uiPropertiesDialog::colSeqChange( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size()<1 )
	return;

    displays_[0]->setColTabSequence( 0, coltbl_->getColTabSequence(), 0 );
}


void uiPropertiesDialog::colMapperChange( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size()<1 )
	return;

    displays_[0]->setColTabMapperSetup( 0, coltbl_->getColTabMapperSetup(), 0 );
}


void uiPropertiesDialog::updateAttribNames()
{
    BufferString oldsel = attribfld_->text();
    attribfld_->setEmpty();
    attribfld_->addItem( mpeman_->sKeyNoAttrib() );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld_->addItem( spec->userRef() );
    }
    attribfld_->setCurrentItem( oldsel );

    updateSelectedAttrib();
    mpeman_->attribSel(0);
}


void uiPropertiesDialog::updateSelectedAttrib()
{
    updateDisplayList();
    if ( displays_.isEmpty() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays_[0]->getSelSpecUserRef();
    if ( !userref && !attribspecs.isEmpty() )
    {
	for ( int idx=0; idx<displays_.size(); idx++ )
	    displays_[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays_[0]->getSelSpecUserRef();
    }
    else if ( FixedString(userref) == sKey::None() )
	userref = mpeman_->sKeyNoAttrib();

    if ( userref )
	attribfld_->setCurrentItem( userref );
}


void uiMPEMan::attribSel( CallBacker* )
{
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();
    if ( trackerisshown && showtexture_ )
	loadPostponedData();

    if ( propdlg_ )
	showtexture_ = propdlg_->selAttrib() > 0;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    if ( !showtexture_ )
    {
	for ( int idy=0; idy<displays.size(); idy++ )
	{
	    Attrib::SelSpec spec( 0, Attrib::SelSpec::cNoAttrib() );
	    displays[idy]->setSelSpec( 0, spec );
	    if ( trackerisshown )
		displays[idy]->updateSlice();
	}
    }
    else
    {
	ObjectSet<const Attrib::SelSpec> attribspecs;
	engine().getNeededAttribs( attribspecs );
	for ( int idx=0; idx<attribspecs.size(); idx++ )
	{
	    const Attrib::SelSpec* spec = attribspecs[idx];
	    if ( !spec )
		continue;

	    for ( int idy=0; idy<displays.size(); idy++ )
	    {
		displays[idy]->setSelSpec( 0, *spec );
		if ( trackerisshown )
		    displays[idy]->updateSlice();
	    }
	    break;
	}
    }

    if ( displays.size() && propdlg_ )
    {
	propdlg_->editor().setColTab( displays[0], 0, mUdf(int) );
    }
}


void uiMPEMan::setColorbarCB( CallBacker* )
{
    mGetDisplays(false);

    if ( displays.size()<1 )
	return;

    if ( !propdlg_ )
    {
	propdlg_ = new uiPropertiesDialog( this );
	propdlg_->windowClosed.notify( mCB(this,uiMPEMan,onColTabClosing) );
    }

    propdlg_->editor().setColTab( displays[0], 0, mUdf(int) );
    propdlg_->updateAttribNames();
    propdlg_->show();
    toolbar->setSensitive( clrtabidx, false );
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    toolbar->setSensitive( clrtabidx, true );
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = toolbar->isOn( moveplaneidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );

    if ( ison )
    {
	toolbar->setToolTip( moveplaneidx, tr("Hide QC plane") );
	attribSel(0);
    }
}


void uiMPEMan::turnQCPlaneOff()
{
    const bool ison = toolbar->isOn( moveplaneidx );

    if ( ison )
    {
	toolbar->turnOn( moveplaneidx, false );
	movePlaneCB(0);
    }
}


static void updateQCButton( uiToolBar* tb, int butidx, int dim )
{
    BufferString pm, tooltip;
    if ( dim == 0 )
	{ pm = "QCplane-inline"; tooltip = "Display QC plane Inline"; }
    else if ( dim == 1 )
	{ pm = "QCplane-crossline"; tooltip = "Display QC plane Crossline"; }
    else
	{ pm = "QCplane-z"; tooltip = "Display QC plane Z-dir"; }

    tb->setIcon( butidx, pm );
    tb->setToolTip( butidx, tooltip );
}


void uiMPEMan::handleOrientationClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;
    const int dim = itm->getID();
    updateQCButton( toolbar, moveplaneidx, dim );
    changeTrackerOrientation( dim );
    movePlaneCB( cb );
}


void uiMPEMan::planeOrientationChangedCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::MPEDisplay*,disp,cb)
    if ( !disp ) return;

    const int dim = disp->getPlaneOrientation();
    updateQCButton( toolbar, moveplaneidx, dim );
}


void uiMPEMan::showSettingsCB( CallBacker* )
{
    visserv->sendShowSetupDlgEvent();
}


void uiMPEMan::displayAtSectionCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) return;

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv->isLocked(selectedids[0]) )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     surface, visserv->getObject(selectedids[0]) );

    bool ison = toolbar->isOn(displayatsectionidx);

    if ( surface && (surface->getObjectID()== tracker->objectID()) )
	surface->setOnlyAtSectionsDisplay( ison );

    toolbar->setToolTip( displayatsectionidx,
                         ison ? tr("Display full")
                              : tr("Display at section only") );
}


void uiMPEMan::retrackAllCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker) return;

    Undo& undo = EM::EMM().undo();
    int cureventnr = undo.currentEventID();
    undo.setUserInteractionEnd( cureventnr, false );

    MouseCursorManager::setOverride( MouseCursor::Wait );
    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    emobj->setBurstAlert( true );
    emobj->removeAllUnSeedPos();

    CubeSampling realactivevol = MPE::engine().activeVolume();
    ObjectSet<CubeSampling>* trackedcubes =
	MPE::engine().getTrackedFlatCubes(
	    MPE::engine().getTrackerByObject(tracker->objectID()) );
    if ( trackedcubes )
    {
	for ( int idx=0; idx<trackedcubes->size(); idx++ )
	{
	    NotifyStopper notifystopper( MPE::engine().activevolumechange );
	    MPE::engine().setActiveVolume( *(*trackedcubes)[idx] );
	    notifystopper.restore();

	    const CubeSampling curvol =  MPE::engine().activeVolume();
	    if ( curvol.nrInl()==1 || curvol.nrCrl()==1 )
		visserv->fireLoadAttribDataInMPEServ();

	    seedpicker->reTrack();
	}

	emobj->setBurstAlert( false );
	deepErase( *trackedcubes );

	MouseCursorManager::restoreOverride();
	undo.setUserInteractionEnd( undo.currentEventID() );

	MPE::engine().setActiveVolume( realactivevol );

	if ( !(MPE::engine().activeVolume().nrInl()==1) &&
	     !(MPE::engine().activeVolume().nrCrl()==1) )
	    trackInVolume(0);
    }
    else
	seedpicker->reTrack();
	emobj->setBurstAlert( false );
	MouseCursorManager::restoreOverride();
	undo.setUserInteractionEnd( undo.currentEventID() );
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

	displays[idx]->planeOrientationChange.notify(
		mCB(this,uiMPEMan,planeOrientationChangedCB) );

	if ( idx==0 )
	    toolbar->turnOn( showcubeidx, displays[idx]->isBoxDraggerShown() );
    }

    bool showtracker = engine().trackPlane().getTrackMode()!=TrackPlane::None;
    if ( !engine().nrTrackersAlive() )
    {
	engine().setTrackMode( TrackPlane::None );
	showtracker = false;
    }

    showTracker( showtracker );
    updateButtonSensitivity(0);
}


void uiMPEMan::trackInVolume()
{ trackInVolume(0); }


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent != preveventnr )
	    undo.setUserInteractionEnd(currentevent);
}


void uiMPEMan::updateButtonSensitivity( CallBacker* )
{
    //Undo/Redo
    BufferString tooltip("Undo ");
    if ( EM::EMM().undo().canUnDo() )
	tooltip += EM::EMM().undo().unDoDesc();
    toolbar->setToolTip( undoidx, tooltip.buf() );
    toolbar->setSensitive( undoidx, EM::EMM().undo().canUnDo() );

    tooltip = "Redo ";
    if ( EM::EMM().undo().canReDo() )
	tooltip += EM::EMM().undo().reDoDesc();
    toolbar->setToolTip( redoidx, tooltip.buf() );
    toolbar->setSensitive( redoidx, EM::EMM().undo().canReDo() );

    //Seed button
    updateSeedPickState();

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    mDynamicCastGet(MPE::Horizon2DSeedPicker*,sp2d,seedpicker)
    const bool is2d = sp2d;
    toolbar->setSensitive( moveplaneidx, !is2d );
    toolbar->setSensitive( showcubeidx, !is2d );

    const bool isinvolumemode = seedpicker && seedpicker->doesModeUseVolume();
    toolbar->setSensitive( trackinvolidx,
	    !is2d && isinvolumemode && seedpicker );
    toolbar->setSensitive( trackwithseedonlyidx,
	    !is2d && isinvolumemode && seedpicker );
    toolbar->setSensitive( displayatsectionidx, seedpicker );

    toolbar->setSensitive( removeinpolygon, toolbar->isOn(polyselectidx) );

    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    toolbar->setSensitive( trackforwardidx, !is2d && trackerisshown );
    toolbar->setSensitive( trackbackwardidx, !is2d && trackerisshown );
    nrstepsbox->setSensitive( !is2d && trackerisshown );

    //coltab
    const bool hasdlg = propdlg_ && !propdlg_->isHidden();
    toolbar->setSensitive( clrtabidx, !is2d && trackerisshown && !hasdlg );

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    toolbar->turnOn( moveplaneidx, trackerisshown && tm==TrackPlane::Move );

    toolbar->setSensitive( tracker );
    if ( seedpicker &&
	    !(visserv->isTrackingSetupActive() && (seedpicker->nrSeeds()<1)) )
	toolbar->setSensitive( true );
}

