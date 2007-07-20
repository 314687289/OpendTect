/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.351 2007-07-20 14:12:29 cvskris Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribsel.h"
#include "binidvalset.h"
#include "iopar.h"
#include "oddirs.h"
#include "seisbuf.h"
#include "separstr.h"
#include "survinfo.h"
#include "uivispickretriever.h"
#include "zaxistransform.h"

#include "viscolortab.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vismpeseedcatcher.h"
#include "visobject.h"
#include "visselman.h"
#include "vissurvobj.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"

#include "uiattribtransdlg.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uifiledlg.h"
#include "uimaterialdlg.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uimpeman.h"
#include "uislicepos.h"
#include "uitoolbar.h"
#include "uibinidsubsel.h"
#include "uizscaledlg.h"


const int uiVisPartServer::evUpdateTree			= 0;
const int uiVisPartServer::evSelection			= 1;
const int uiVisPartServer::evDeSelection		= 2;
const int uiVisPartServer::evGetNewData			= 3;
const int uiVisPartServer::evMouseMove			= 4;
const int uiVisPartServer::evInteraction		= 5;
const int uiVisPartServer::evSelectAttrib		= 6;
const int uiVisPartServer::evViewAll			= 9;
const int uiVisPartServer::evToHomePos			= 10;
const int uiVisPartServer::evPickingStatusChange	= 11;
const int uiVisPartServer::evViewModeChange		= 12;
const int uiVisPartServer::evShowSetupDlg		= 13;
const int uiVisPartServer::evLoadPostponedData		= 14;
const int uiVisPartServer::evToggleBlockDataLoad	= 15;


const char* uiVisPartServer::sKeyAppVel()		{ return "AppVel"; }
const char* uiVisPartServer::sKeyWorkArea()		{ return "Work Area"; }


static const int sResetManipIdx = 800;
static const int sColorIdx = 700;
static const int sPropertiesIdx = 600;
static const int sResolutionIdx = 500;

uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , menu_( *new uiMenuHandler(appserv().parent(),-1) )
    , resetmanipmnuitem_("Reset Manipulation",sResetManipIdx)
    , changecolormnuitem_("Color...",sColorIdx)
    , changematerialmnuitem_("Properties ...",sPropertiesIdx)
    , resmnuitem_("Resolution",sResolutionIdx)
    , eventmutex_(*new Threads::Mutex)
    , viewmode_(false)
    , issolomode_(false)
    , eventobjid_(-1)
    , eventattrib_(-1)
    , selattrib_(-1)
    , mouseposstr_("")
    , blockmenus_(false)
    , xytmousepos_(Coord3::udf())
    , zfactor_(1)
    , mpetools_(0)
    , slicepostools_(0)
    , pickretriever_( new uiVisPickRetriever )
    , nrsceneschange_( this )
{
    menu_.ref();
    menu_.createnotifier.notify( mCB(this,uiVisPartServer,createMenuCB) );
    menu_.handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

    visBase::DM().selMan().selnotifier.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.notify( 
	mCB(this,uiVisPartServer,deselectObjCB) );

    vismgr_ = new uiVisModeMgr(this);
    pickretriever_->ref();
    PickRetriever::setInstance( pickretriever_ );
}


void uiVisPartServer::unlockEvent()
{ eventmutex_.unlock(); }


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );

    deleteAllObjects();
    delete vismgr_;

    delete &eventmutex_;
    delete mpetools_;
    menu_.unRef();
    pickretriever_->unRef();
}


const char* uiVisPartServer::name() const  { return "Visualisation"; }


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( !obj ) return 0;
    return obj->name();
}


int uiVisPartServer::addScene( visSurvey::Scene* newscene )
{
    if ( !newscene ) newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    newscene->ref();
    scenes_ += newscene;
    pickretriever_->addScene( newscene );
    nrsceneschange_.trigger();
    return newscene->id();
}


void uiVisPartServer::removeScene( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
	pickretriever_->removeScene( scene );
	scene->unRef();
	scenes_ -= scene;
	nrsceneschange_.trigger();
	return;
    }
}


bool uiVisPartServer::clickablesInScene( const char* trackertype, 
					 int sceneid ) const
{
    TypeSet<int> sceneobjids;
    getChildIds( sceneid, sceneobjids );
    for ( int idx=0; idx<sceneobjids.size(); idx++ )
    {
	const int objid = sceneobjids[idx];
	if ( visSurvey::MPEClickCatcher::isClickable(trackertype,objid) )
	{
	    if ( !hasAttrib(objid) )
		return true;

	    for ( int attrid=getNrAttribs(objid)-1; attrid>=0; attrid-- )
	    {
		if ( isAttribEnabled(objid,attrid) )
		    return true;
	    }
	}
    }
    
    uiMSG().warning( "The scene yet contains no object on which seeds\n"
		     "for a '", trackertype, "' can be picked." );
    return false;
}


bool uiVisPartServer::disabMenus( bool yn )
{
    const bool res = blockmenus_;
    blockmenus_ = yn;
    return res;
}


void uiVisPartServer::createToolBars()
{
    mpetools_ = new uiMPEMan( appserv().parent(), this );
    slicepostools_ = new uiSlicePos( appserv().parent() );
}


bool uiVisPartServer::disabToolBars( bool yn )
{
    bool res = false;
    if ( mpetools_ )
    {
	res = !mpetools_->getToolBar()->sensitive();
	mpetools_->getToolBar()->setSensitive( !yn );
    }

    if ( slicepostools_ )
    {
	res = !slicepostools_->getToolBar()->sensitive();
	slicepostools_->getToolBar()->setSensitive( !yn );
    }
    return res;
}


bool uiVisPartServer::blockMouseSelection( bool yn )
{
    if ( scenes_.isEmpty() ) return false;

    const bool res = scenes_[0]->blockMouseSelection( yn );
    for ( int idx=1; idx<scenes_.size(); idx++ )
	scenes_[idx]->blockMouseSelection( yn );

    return res;
}


bool uiVisPartServer::showMenu( int id, int menutype, const TypeSet<int>* path,
				const Coord3& pickedpos )
{
    if ( blockmenus_ )
	return true;

    menu_.setMenuID( id );
    menu_.setPickedPos(pickedpos);
    return menu_.executeMenu(menutype,path);
}


MenuHandler* uiVisPartServer::getMenuHandler()
{ return &menu_; }


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    if ( !dobj ) return;

    scene->addObject( dobj );
    eventmutex_.lock();
    sendEvent( evUpdateTree );
}


void uiVisPartServer::triggerTreeUpdate()
{
    eventmutex_.lock();
    sendEvent( evUpdateTree );
}


void uiVisPartServer::findObject( const std::type_info& ti, TypeSet<int>& res )
{
    visBase::DM().getIds( ti, res );
}


void uiVisPartServer::findObject( const MultiID& mid, TypeSet<int>& res )
{
    res.erase();
    const int highestid = highestID();
    for ( int idx=0; idx<=highestid; idx++ )
    {
	const MultiID vismid = getMultiID(idx);
	if ( vismid==-1 )
	    continue;

	if ( vismid==mid )
	    res += idx;
    }
}


int uiVisPartServer::highestID() const
{
    return visBase::DM().highestID();
}


visBase::DataObject* uiVisPartServer::getObject( int id ) const
{
    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    return dobj;
}


void uiVisPartServer::addObject( visBase::DataObject* dobj, int sceneid,
				 bool saveinsessions  )
{
    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    scene->addObject( dobj );
    //TODO: Handle saveinsessions

    setUpConnections( dobj->id() );
    if ( isSoloMode() )
    {
	int typesetidx = scenes_.indexOf(scene);
	displayids_[typesetidx] += dobj->id();
	turnOn( dobj->id(), true, true );
    }
}


bool uiVisPartServer::isLocked( int objectid ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(objectid));
    if ( !so )
	return false;

    return so->isLocked();
}


void uiVisPartServer::removeObject( visBase::DataObject* dobj, int sceneid )
{
    removeObject( dobj->id(), sceneid );
}


NotifierAccess& uiVisPartServer::removeAllNotifier()
{
    return visBase::DM().removeallnotify;
}


MultiID uiVisPartServer::getMultiID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->getMultiID();

    return MultiID(-1);
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


int uiVisPartServer::getSelAttribNr() const
{ return selattrib_; }


void uiVisPartServer::setSelObjectId( int id, int attrib )
{
    visBase::DM().selMan().select( id );
    if ( id==-1 )
	return;

    selattrib_ = attrib;

    eventmutex_.lock();
    eventobjid_ = id;
    sendEvent( evSelection );
    sendEvent( evPickingStatusChange );
}


int uiVisPartServer::getSceneID( int visid ) const
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> childids;
	getChildIds( sceneids[idx], childids );
	if ( childids.indexOf(visid) > -1 )
	    return sceneids[idx];
    }

    return -1;
}


const char* uiVisPartServer::getDepthDomainKey( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? scene->getDepthDomainKey() : 0;
}


void uiVisPartServer::getChildIds( int id, TypeSet<int>& childids ) const
{
    childids.erase();

    if ( id==-1 )
    {
	for ( int idx=0; idx<scenes_.size(); idx++ )
	    childids += scenes_[idx]->id();

	return;
    }

    const visSurvey::Scene* scene = getScene( id );
    if ( scene )
    {
	for ( int idx=0; idx<scene->size(); idx++ )
	{
	    childids += scene->getObject( idx )->id();
	}

	return;
    }

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->getChildren( childids );
}


uiVisPartServer::AttribFormat uiVisPartServer::getAttributeFormat(int id) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    switch ( so ? so->getAttributeFormat() : visSurvey::SurveyObject::None )
    {
	case visSurvey::SurveyObject::None		: return None;
	case visSurvey::SurveyObject::Cube		: return Cube;
	case visSurvey::SurveyObject::Traces		: return Traces;
	case visSurvey::SurveyObject::RandomPos		: return RandomPos;
	case visSurvey::SurveyObject::OtherFormat	: return OtherFormat;
    }

    return None;
}


bool uiVisPartServer::canHaveMultipleAttribs( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canHaveMultipleAttribs() : false;
}


bool uiVisPartServer::canAddAttrib( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canAddAttrib() : false;
}


int uiVisPartServer::addAttrib( int id )
{
    if ( !canHaveMultipleAttribs(id) )
	return -1;

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return -1;
    
    return so->addAttrib() ? so->nrAttribs()-1 : -1;
}


void uiVisPartServer::removeAttrib( int id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;
    
    so->removeAttrib( attrib );
}

int uiVisPartServer::getNrAttribs( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return 0;

    return so->nrAttribs();
}


bool uiVisPartServer::swapAttribs( int id, int attrib0, int attrib1 )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    return so->swapAttribs( attrib0, attrib1 );
}


void uiVisPartServer::showAttribTransparencyDlg( int id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    uiAttribTransDlg dlg( appserv().parent(), *so, attrib );
    dlg.go();
}


unsigned char uiVisPartServer::getAttribTransparency( int id,int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return 0;

    return so->getAttribTransparency( attrib );
}


void uiVisPartServer::setAttribTransparency( int id, int attrib,
					     unsigned char nv )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) 
	so->setAttribTransparency( attrib, nv );
}


CubeSampling uiVisPartServer::getCubeSampling( int id, int attribid ) const
{
    CubeSampling res;
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) res = so->getCubeSampling( attribid );
    return res;
}


DataPack::ID uiVisPartServer::getDataPackID( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackID( attrib ) : -1;
}


DataPackMgr::ID	uiVisPartServer::getDataPackMgrID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackMgrID() : -1;
}


bool uiVisPartServer::setDataPackID( int id, int attrib, DataPack::ID dpid )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiCursorChanger cursorlock( uiCursor::Wait );
    return so->setDataPackID( attrib, dpid );
}


const Attrib::DataCubes* uiVisPartServer::getCachedData(
						    int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getCacheVolume( attrib ) : 0;
}


bool uiVisPartServer::setCubeData( int id, int attrib, 
				   const Attrib::DataCubes* attribdata )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiCursorChanger cursorlock( uiCursor::Wait );
    return so->setDataVolume( attrib, attribdata );
}


bool uiVisPartServer::canHaveMultipleTextures(int id) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->canHaveMultipleTextures();
    return false;
}


int uiVisPartServer::nrTextures( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->nrTextures( attrib );
    return 0;
}


void uiVisPartServer::selectTexture( int id, int attrib, int textureidx )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->selectTexture( attrib, textureidx );
}


int uiVisPartServer::selectedTexture( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->selectedTexture( attrib ) : 0;
}


void uiVisPartServer::getRandomPos( int id,
				    ObjectSet<BinIDValueSet>& bivs ) const
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->getRandomPos( bivs );
}


void uiVisPartServer::getRandomPosCache( int id, int attrib,
				    ObjectSet<const BinIDValueSet>& bivs ) const
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->getRandomPosCache( attrib, bivs );
}


void uiVisPartServer::setRandomPosData( int id, int attrib,
					const ObjectSet<BinIDValueSet>* bp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setRandomPosData( attrib, bp );
}


void uiVisPartServer::getDataTraceBids( int id, TypeSet<BinID>& bids ) const
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->getDataTraceBids( bids );
}
	

Interval<float> uiVisPartServer::getDataTraceRange( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataTraceRange() : Interval<float>(0,0);
}


void uiVisPartServer::setTraceData( int id, int attrib, SeisTrcBuf& data )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setTraceData( attrib, data );
    else
	data.deepErase();
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos_ : inlcrlmousepos_; }


BufferString uiVisPartServer::getMousePosVal() const
{ return mouseposval_; }


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getManipulationString() : BufferString("");
}


void uiVisPartServer::getObjectInfo( int id, BufferString& info ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so ) so->getObjectInfo( info );
}


int uiVisPartServer::getColTabId( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getColTabID( attrib ) : -1;
}


void uiVisPartServer::setClipRate( int id, int attrib, float cr )
{
    mDynamicCastGet(visBase::VisColorTab*,coltab,
	            getObject(getColTabId(id, attrib)));
    if ( coltab ) coltab->setClipRate( cr );
}


const TypeSet<float>* uiVisPartServer::getHistogram( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getHistogram( attrib ) : 0;
}


int uiVisPartServer::getEventObjId() const { return eventobjid_; }


int uiVisPartServer::getEventAttrib() const { return eventattrib_; }


const Attrib::SelSpec* uiVisPartServer::getSelSpec( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpec( attrib ) : 0;
}


bool uiVisPartServer::isClassification( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isClassification( attrib ) : false;
}


void uiVisPartServer::setClassification( int id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    so->setClassification( attrib, yn );
}


bool uiVisPartServer::isAngle( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isAngle( attrib ) : false;
}


void uiVisPartServer::setAngleFlag( int id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    so->setAngleFlag( attrib, yn );
}


bool uiVisPartServer::isAttribEnabled( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isAttribEnabled( attrib ) : false;
}


void uiVisPartServer::enableAttrib( int id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->enableAttrib( attrib, yn );
}


bool uiVisPartServer::deleteAllObjects()
{
    mpetools_->deleteVisObjects();

    while ( scenes_.size() )
	removeScene( scenes_[0]->id() );

    scenes_.erase();
    nrsceneschange_.trigger();
    return visBase::DM().removeAll();
}


void uiVisPartServer::setViewMode( bool yn, bool notify)
{
    if ( yn==viewmode_ ) return;
    viewmode_ = yn;
    toggleDraggers();
    if ( notify )
	sendEvent(evViewModeChange);
}


bool uiVisPartServer::isViewMode() const { return viewmode_; }


bool uiVisPartServer::isSoloMode() const { return issolomode_; }


void uiVisPartServer::setSoloMode( bool yn, TypeSet< TypeSet<int> > dispids,
				   int selid )
{
    issolomode_ = yn;
    displayids_ = dispids;
    issolomode_ ? updateDisplay( true, selid ) : updateDisplay( false, selid );
}


int uiVisPartServer::getTypeSetIdx( int selid )
{
    int typesetidx=0;
    bool found = false;
    for ( int idx=0; idx<displayids_.size(); idx++ )
    {
	for ( int idy=0; idy<displayids_[idx].size(); idy++ )
	{
	    if ( displayids_[idx][idy] == selid )
	    {
		typesetidx = idx;
		found = true;
		break;
	    }
	}
	if ( found )
	    break;
    }

    return typesetidx;
}


void uiVisPartServer::updateDisplay( bool doclean, int selid, int refid )
{
    int typesetidx;
    typesetidx = getTypeSetIdx( selid != -1 ? selid : refid );
    
    if ( doclean && displayids_.size()>typesetidx )
    {
	for ( int idx=0; idx<displayids_[typesetidx].size(); idx++ )
	    if ( isOn( displayids_[typesetidx][idx] ) 
		 && displayids_[typesetidx][idx] != selid )
		turnOn(displayids_[typesetidx][idx], false);

	if ( !isOn(selid) && selid >= 0 ) turnOn( selid, true);
    }
    else
    {
	bool isselchecked = false;
	for ( int ids=0; ids<displayids_.size(); ids++ )
	{
	    for ( int idx=0; idx<displayids_[ids].size(); idx++ )
	    {
		turnOn(displayids_[ids][idx], true);
		if ( displayids_[ids][idx] == selid )
		    isselchecked = true;
	    }
	}

	if ( !isselchecked ) turnOn(selid, false);
    }
}


void uiVisPartServer::toggleDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes_.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes_[sceneidx];

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && !viewmode_;

	    mDynamicCastGet(visSurvey::SurveyObject*,so,obj)
	    if ( so ) so->showManipulator(isdraggeron);
	}
    }
}
    


void uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.vwallcb = mCB(this,uiVisPartServer,vwAll);
    dlg.homecb = mCB(this,uiVisPartServer,toHome);
    dlg.go();
}


void uiVisPartServer::vwAll( CallBacker* )
{ eventmutex_.lock(); sendEvent( evViewAll ); }

void uiVisPartServer::toHome( CallBacker* )
{ eventmutex_.lock(); sendEvent( evToHomePos ); }


bool uiVisPartServer::setWorkingArea()
{
    uiBinIDSubSelDlg dlg( appserv().parent(), uiBinIDSubSel::Setup()
					       .withz(true)
					       .withstep(false)
					       .showsurvinfo(true)
					       .rangeonly(true) );
    if ( !dlg.go() ) return false;
    CubeSampling cs = dlg.getInput().cs_;
    cs.limitTo( SI().sampling(false) );
    const_cast<SurveyInfo&>(SI()).setWorkRange( cs );

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObject( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene ) scene->setAnnotationCube( SI().sampling(true) );
    }

    return true;
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    BufferString res;
    if ( par.get( sKeyWorkArea(), res ) )
    {
	FileMultiString fms(res);
	CubeSampling cs;
	HorSampling& hs = cs.hrg; StepInterval<float>& zrg = cs.zrg;
	hs.start.inl = atoi(fms[0]); hs.stop.inl = atoi(fms[1]);
	hs.start.crl = atoi(fms[2]); hs.stop.crl = atoi(fms[3]);
	zrg.start = atof(fms[4]); zrg.stop = atof(fms[5]);
	const_cast<SurveyInfo&>(SI()).setRange( cs, true );
    }

    if ( !visBase::DM().usePar( par ) )
	return false;

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    TypeSet<int> hasconnections;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
		(visSurvey::Scene*) visBase::DM().getObject( sceneids[idx] );
	addScene( newscene );

	float appvel;
	if ( par.get(sKeyAppVel(),appvel) )
	    newscene->setZScale( appvel/1000 );

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    int childid = children[idy];
	    if ( hasconnections.indexOf( childid ) >= 0 ) continue;

	    setUpConnections( childid );
	    hasconnections += childid;

	    turnOn( childid, isOn(childid) );
	}
    }

    mpetools_->initFromDisplay();

    return true;
}


void uiVisPartServer::calculateAllAttribs()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	TypeSet<int> children;
	getChildIds( scenes_[idx]->id(), children );
	for ( int idy=0; idy<children.size(); idy++ )
	{
	    const int childid = children[idy];
	    visBase::DataObject* dobj = visBase::DM().getObject( childid );
	    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
	    if ( !so ) continue;
	    for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
		calculateAttrib( childid, attrib, false );
	}
    }
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes_.size(); idx++ )
	storids += scenes_[idx]->id();

    const CubeSampling& cs = SI().sampling( true );
    FileMultiString fms;
    fms += cs.hrg.start.inl; fms += cs.hrg.stop.inl; fms += cs.hrg.start.crl;
    fms += cs.hrg.stop.crl; fms += cs.zrg.start; fms += cs.zrg.stop;
    par.set( sKeyWorkArea(), fms );

    visBase::DM().fillPar( par, storids );
}


void uiVisPartServer::turnOn( int id, bool yn, bool doclean )
{
    if ( !yn || !vismgr_->allowTurnOn(id,doclean) )
	yn = false;

    visBase::DataObject* dobj = visBase::DM().getObject( id );
    if ( !dobj ) return;

    mDynamicCastGet(visBase::VisualObject*,vo,dobj)
    if ( vo ) vo->turnOn( yn );

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* scene =
		(visSurvey::Scene*) visBase::DM().getObject( sceneids[idx] );
	if ( scene ) scene->objectMoved(dobj);
    }
}


bool uiVisPartServer::isOn( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(const visBase::VisualObject*,vo,dobj)
    return vo ? vo->isOn() : false;
}


bool uiVisPartServer::canDuplicate( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canDuplicate();
}    


int uiVisPartServer::duplicateObject( int id, int sceneid )
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return -1;

    visSurvey::SurveyObject* newso = so->duplicate();
    if ( !newso ) return -1;

    mDynamicCastGet(visBase::DataObject*,doobj,newso)
    addObject( doobj, sceneid, true );

    return doobj->id();
}


bool uiVisPartServer::sendShowSetupDlgEvent()
{
    return sendEvent( evShowSetupDlg );
}


bool uiVisPartServer::sendPickingStatusChangeEvent()
{
   return sendEvent( evPickingStatusChange );
}


void uiVisPartServer::turnSeedPickingOn( bool yn )
{
    mpetools_->turnSeedPickingOn( yn );
}


bool uiVisPartServer::isPicking() const
{
    if ( isViewMode() )
	return false;

    if ( mpetools_ && mpetools_->isSeedPickingOn() ) 
	return true;

    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    if ( sel.size()!=1 ) return false;

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(sel[0]));
    if ( !so ) return false;

    return so->isPicking();
}


void uiVisPartServer::loadPostponedData() const
{
    sendEvent( evLoadPostponedData );
}


void uiVisPartServer::toggleBlockDataLoad() const
{
    sendEvent( evToggleBlockDataLoad );
}


#define mGetScene( prepostfix ) \
prepostfix visSurvey::Scene* \
uiVisPartServer::getScene( int sceneid ) prepostfix \
{ \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
    { \
	if ( scenes_[idx]->id()==sceneid ) \
	{ \
	    return scenes_[idx]; \
	} \
    } \
 \
    return 0; \
}

mGetScene( );
mGetScene( const ); 


void uiVisPartServer::removeObject( int id, int sceneid )
{
    removeConnections( id );

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const int idx = scene->getFirstIdx( id );
    if ( idx!=-1 ) 
	scene->removeObject( idx );
}


bool uiVisPartServer::hasAttrib( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->getAttributeFormat() != visSurvey::SurveyObject::None;
}
    

bool uiVisPartServer::selectAttrib( int id, int attrib )
{
    eventmutex_.lock();
    eventobjid_ = id;
    eventattrib_ = attrib;
    return sendEvent( evSelectAttrib );
}


void uiVisPartServer::setSelSpec( int id, int attrib,
				  const Attrib::SelSpec& myattribspec )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setSelSpec( attrib, myattribspec );
}


bool uiVisPartServer::calculateAttrib( int id, int attrib, bool newselect )
{
    if ( isLocked(id) )
    {
	resetManipulation(id);
	return true;
    }


    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    if ( so->isManipulated() )
	so->acceptManipulation();

    const Attrib::SelSpec* as = so->getSelSpec( attrib );
    if ( !as ) return false;
    if ( as->id()==Attrib::SelSpec::cNoAttrib() )
	return true;

    if ( newselect || ( as->id()==Attrib::SelSpec::cAttribNotSel() ) )
    {
	if ( !selectAttrib( id, attrib ) )
	    return false;
    }

    eventmutex_.lock();
    eventobjid_ = id;
    eventattrib_ = attrib;
    return sendEvent( evGetNewData );
}


bool uiVisPartServer::hasMaterial( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->allowMaterialEdit();
}


bool uiVisPartServer::setMaterial( int id )
{
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !hasMaterial(id) || !vo ) return false;

    uiPropertiesDlg dlg( appserv().parent(),
	    dynamic_cast<visSurvey::SurveyObject*>(vo) );

    dlg.go();

    return true;
}


bool uiVisPartServer::dumpOI( int id ) const
{
    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(), "*.iv",
			  "Select output file" );
    if ( filedlg.go() )
    {
	visBase::DataObject* obj = visBase::DM().getObject( id );
	if ( !obj ) return false;
	return obj->dumpOIgraph( filedlg.fileName() );
    }

    return false;
}


bool uiVisPartServer::resetManipulation( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->resetManipulation();

    eventmutex_.lock();
    eventobjid_ = id;
    sendEvent( evInteraction );
    sendEvent( evUpdateTree );

    return so;
}


bool uiVisPartServer::isManipulated( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isManipulated();
}


void uiVisPartServer::acceptManipulation( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->acceptManipulation();
}


void uiVisPartServer::setUpConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;
    if ( na ) na->notify( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : 0;
    if ( na ) na->notify( mCB(mpetools_,uiMPEMan,visObjectLockedCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->notify(mCB(this,uiVisPartServer,rightClickCB));

    if ( so )
    {
	for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
	{
	    mDynamicCastGet(visBase::VisColorTab*,coltab,
			    getObject(getColTabId(id,attrib)));
	    if ( coltab )
		coltab->sequencechange.notify(mCB(this,uiVisPartServer,
						  colTabChangeCB));
	}
    }
}


void uiVisPartServer::removeConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;
    if ( na ) na->remove( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : 0;
    if ( na ) na->remove( mCB(mpetools_,uiMPEMan,visObjectLockedCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id));
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->remove( mCB(this,uiVisPartServer,rightClickCB) );
    if ( !so ) return;

    for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
    {
	mDynamicCastGet(visBase::VisColorTab*,coltab,
			getObject(getColTabId(id,attrib)));
	if ( coltab )
	    coltab->sequencechange.remove(
				    mCB(this,uiVisPartServer,colTabChangeCB) );
    }
}


void uiVisPartServer::rightClickCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,cb);
    const int id = dataobj ? dataobj->id() : getSelObjectId();
    if ( id==-1 )
	return;

    Coord3 pickedpos = Coord3::udf();
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id));
    if ( vo && vo->rightClickedEventInfo() && vo->getDisplayTransformation() )
    {
	pickedpos = vo->getDisplayTransformation()->transformBack(
			vo->rightClickedEventInfo()->pickedpos );
	pickedpos = visSurvey::STM().currentScene()->
			getZScaleTransform()->transformBack( pickedpos );
    }

    showMenu( id, uiMenuHandler::fromScene, 
	      dataobj ? dataobj->rightClickedPath() : 0, pickedpos );
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    visBase::DataObject* dobj = visBase::DM().getObject( sel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !viewmode_ && so )
	so->showManipulator(true);

    selattrib_ = -1;

    eventmutex_.lock();
    eventobjid_ = sel;
    sendEvent( evSelection );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    visBase::DataObject* dobj = visBase::DM().getObject( oldsel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj)
    if ( so )
    {    
	if ( so->isManipulated() )
	{
	    if ( isLocked(oldsel) )
		resetManipulation( oldsel );
	    else
	    {
		so->acceptManipulation();
		for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
		    calculateAttrib( oldsel, attrib, false );
	    }
	}

	if ( !viewmode_ )
	    so->showManipulator(false);
    }

    eventmutex_.lock();
    eventobjid_ = oldsel;
    selattrib_ = -1;
    sendEvent( evDeSelection );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange );
}


void uiVisPartServer::interactionCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,cb)
    if ( dataobj )
    {
	eventmutex_.lock();
	eventobjid_ = dataobj->id();
	sendEvent( evInteraction );
    }
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb)
    if ( !scene ) return;

    Coord3 worldpos = xytmousepos_ = scene->getMousePos(true);
    if ( xytmousepos_.isDefined() && scene->getDataTransform() )
	worldpos.z = scene->getDataTransform()->transformBack( xytmousepos_ );

    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]==scene )
	    scenes_[idx]->setMarkerPos( Coord3::udf() );
	else
	    scenes_[idx]->setMarkerPos( worldpos );
    }

    eventmutex_.lock();
    inlcrlmousepos_ = scene->getMousePos(false);
    mouseposval_ = scene->getMousePosValue();
    mouseposstr_ = scene->getMousePosString();
    zfactor_ = scene->getDataTransform() ? 1 : SI().zFactor();
    sendEvent( evMouseMove );
}


void uiVisPartServer::createMenuCB(CallBacker* cb)
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(menu->menuID()));
    if ( !so ) return;

    mAddMenuItemCond( menu, &resetmanipmnuitem_, 
	    	      so->isManipulated() && !isLocked(menu->menuID()), false,
		      so->canResetManipulation() );
    //mAddMenuItemCond( menu, &changecolormnuitem_, true, false, so->hasColor() );
    changecolormnuitem_.id = -1;

    mAddMenuItemCond( menu, &changematerialmnuitem_, true, false, 
		      so->allowMaterialEdit() );

    resmnuitem_.id = -1;
    resmnuitem_.removeItems();
    if ( so->nrResolutions()>1 )
    {
	BufferStringSet resolutions;
	for ( int idx=0; idx<so->nrResolutions(); idx++ )
	    resolutions.add( so->getResolutionName(idx) );

	resmnuitem_.createItems( resolutions );
	for ( int idx=0; idx<resmnuitem_.nrItems(); idx++ )
	    resmnuitem_.getItem(idx)->checkable = true;
	resmnuitem_.getItem(so->getResolution())->checked = true;
	mAddMenuItem(menu, &resmnuitem_, true, false );
    }
    else
	mResetMenuItem(&resmnuitem_);
}


void uiVisPartServer::handleMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 ) return;

    mDynamicCastGet(uiMenuHandler*,menu,caller);
    const int id = menu->menuID();
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( !so ) return;
    
    if ( mnuid==resetmanipmnuitem_.id )
    {
	resetManipulation(id);
	menu->setIsHandled(true);
    }
    else if ( mnuid==changecolormnuitem_.id )
    {
	Color col = so->getColor();
	if ( selectColor(col,appserv().parent(),"Color selection",false) )
	    so->setColor( col );
	menu->setIsHandled(true);
    }
    else if ( mnuid==changematerialmnuitem_.id )
    {
	setMaterial(id);
	menu->setIsHandled(true);
    }
    else if ( resmnuitem_.id!=-1 && resmnuitem_.itemIndex(mnuid)!=-1 )
    {
	uiCursorChanger cursorlock( uiCursor::Wait );
	so->setResolution(resmnuitem_.itemIndex(mnuid));
	menu->setIsHandled(true);
    }
}


void uiVisPartServer::colTabChangeCB( CallBacker* )
{
    triggerTreeUpdate();
}


void uiVisPartServer::showMPEToolbar( bool yn )
{
    if ( yn )
    {
	updateMPEToolbar();
	if ( !mpetools_->getToolBar()->isShown() )
	    mpetools_->getToolBar()->display( true );
    }
    else if ( mpetools_->getToolBar()->isShown() )
	mpetools_->getToolBar()->display( false );
}


void uiVisPartServer::updateMPEToolbar()
{
    mpetools_->validateSeedConMode();
    mpetools_->updateAttribNames();
}


void uiVisPartServer::introduceMPEDisplay()
{ mpetools_->introduceMPEDisplay(); }


uiToolBar* uiVisPartServer::getTrackTB() const
{ return mpetools_->getToolBar(); }


void uiVisPartServer::initMPEStuff()
{
    TypeSet<int> emobjids;
    findObject( typeid(visSurvey::EMObjectDisplay), emobjids );
    for ( int idx=0; idx<emobjids.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			getObject(emobjids[idx]))
	emod->updateFromMPE();
    }
    
    mpetools_->initFromDisplay();
}


bool uiVisPartServer::canBDispOn2DViewer( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canBDispOn2DViewer() : false;
}


bool uiVisPartServer::isVerticalDisp( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isVerticalPlane() : true;
}


uiVisModeMgr::uiVisModeMgr( uiVisPartServer* p )
    : visserv(*p)
{
}


bool uiVisModeMgr::allowTurnOn( int id, bool doclean )
{
    if ( !visserv.issolomode_ && !doclean ) return true;
    if ( !visserv.issolomode_ ) return false;
    const TypeSet<int>& selected = visBase::DM().selMan().selected();
    for ( int idx=0; idx<selected.size(); idx++ )
    {
	if ( selected[idx] == id )
	{
	    if ( doclean ) visserv.updateDisplay(doclean, -1, id);
	    return true;
	}
    }

    return false;

}
