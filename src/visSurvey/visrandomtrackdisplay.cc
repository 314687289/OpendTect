/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2003
 ________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visrandomtrackdisplay.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "convmemvalseries.h"
#include "seisdatapack.h"
#include "seisdatapackzaxistransformer.h"
#include "randomlinegeom.h"
#include "mousecursor.h"
#include "polylinend.h"
#include "settings.h"
#include "uistrings.h"

#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vismarkerset.h"
#include "vispolyline.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdragger.h"
#include "vistexturechannels.h"
#include "vistexturepanelstrip.h"
#include "zaxistransform.h"


namespace visSurvey
{

const char* RandomTrackDisplay::sKeyTrack()	    { return "Random track"; }
const char* RandomTrackDisplay::sKeyNrKnots()	    { return "Nr. Knots"; }
const char* RandomTrackDisplay::sKeyKnotPrefix()    { return "Knot "; }
const char* RandomTrackDisplay::sKeyDepthInterval() { return "Depth Interval"; }
const char* RandomTrackDisplay::sKeyLockGeometry()  { return "Lock geometry"; }

RandomTrackDisplay::RandomTrackDisplay()
    : MultiTextureSurveyObject()
    , panelstrip_( visBase::TexturePanelStrip::create() )
    , dragger_( visBase::RandomTrackDragger::create())
    , polyline_( visBase::PolyLine::create())
    , polylinemode_(false)
    , nodemoving_(this)
    , moving_(this)
    , selnodeidx_(-1)
    , ismanip_(false)
    , interactivetexturedisplay_( false )
    , originalresolution_(-1)
    , datatransform_(0)
    , lockgeometry_(false)
    , eventcatcher_(0)
    , depthrg_(SI().zRange(true))
    , voiidx_(-1)
    , markerset_( visBase::MarkerSet::create() )
    , rl_(0)
    , nrgeomchangecbs_(0)
{
    TypeSet<int> randomlines;
    visBase::DM().getIDs( typeid(*this), randomlines );
    int highestnamenr = 0;
    for ( int idx=0; idx<randomlines.size(); idx++ )
    {
	mDynamicCastGet( const RandomTrackDisplay*, rtd,
			 visBase::DM().getObject(randomlines[idx]) );
	if ( rtd==this ) continue;

	if ( rtd==0 )
	{
#	    ifdef __debug__
		pErrMsg( "Invalid random track display." );
#	    endif
	    continue;
	}

	if ( rtd->nameNr()>highestnamenr )
	    highestnamenr = rtd->nameNr();
    }

    namenr_ = highestnamenr+1;
    setName( uiStrings::phrJoinStrings( uiStrings::sRandomLine(),
					toUiString(namenr_)) );

    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );


    dragger_->ref();
    addChild( dragger_->osgNode() );

    mAttachCB( dragger_->motion, RandomTrackDisplay::nodeMoved );
    mAttachCB( dragger_->movefinished,RandomTrackDisplay::draggerMoveFinished );
    mAttachCB( dragger_->rightClicked(),RandomTrackDisplay::draggerRightClick );

    int dragkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelDepthKey(), dragkey );
    dragger_->setTransDragKeys( true, dragkey, 0 );
    dragkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelPlaneKey(), dragkey );
    dragger_->setTransDragKeys( false, dragkey, 0 );
    dragkey = OD::ControlButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelRotateKey(), dragkey );
    dragger_->setTransDragKeys( true, dragkey, 1 );

    panelstrip_->ref();
    addChild( panelstrip_->osgNode() );
    panelstrip_->setTextureChannels( channels_ );
    panelstrip_->swapTextureAxes();

    polyline_->ref();
    addChild( polyline_->osgNode() );
    polyline_->setMaterial( new visBase::Material );
    polyline_->setLineStyle( LineStyle(LineStyle::Solid,1) );

    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMarkersSingleColor( polyline_->getMaterial()->getColor() );

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange(
		    mCast(float,SI().sampling(true).hsamp_.start_.inl()),
		    mCast(float,SI().sampling(true).hsamp_.stop_.inl()),
		    mCast(float,SI().inlStep()) );
    const StepInterval<float> crlrange(
		    mCast(float,SI().sampling(true).hsamp_.start_.crl()),
		    mCast(float,SI().sampling(true).hsamp_.stop_.crl()),
		    mCast(float,SI().crlStep()) );

    const BinID start( mNINT32(inlrange.center()), mNINT32(crlrange.start) );
    const BinID stop(start.inl(), mNINT32(crlrange.stop) );

    Geometry::RandomLine* rl =
	new Geometry::RandomLine( mFromUiStringTodo(name()) );
    setRandomLineID( rl->ID() );
    rl_->addNode( start );
    rl_->addNode( stop );

    setDepthInterval( Interval<float>( survinterval.start,
				       survinterval.stop ));
    dragger_->setLimits(
	    Coord3( inlrange.start, crlrange.start, survinterval.start ),
	    Coord3( inlrange.stop, crlrange.stop, survinterval.stop ),
	    Coord3( inlrange.step, crlrange.step, survinterval.step ) );

    init();		// sets default resolution -> update texture mapping
    updatePanelStripPath();
    setPanelStripZRange( panelstrip_->getZRange() );

    showManipulator( dragger_->isOn() );
}


RandomTrackDisplay::~RandomTrackDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );
    panelstrip_->unRef();
    dragger_->unRef();
    removeChild( polyline_->osgNode() );
    polyline_->unRef();

    removeChild( markerset_->osgNode() );
    markerset_->unRef();

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpm.release( datapackids_[idx] );

    for ( int idx=0; idx<transfdatapackids_.size(); idx++ )
	dpm.release( transfdatapackids_[idx] );

    if ( rl_ )
    {
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB) );
	rl_->unRef();
    }

    setZAxisTransform( 0, 0 );
}


void RandomTrackDisplay::setRandomLineID( int rlid )
{
    if ( rl_ )
    {
	rl_->unRef();
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB) );
    }

    rl_ = Geometry::RLM().get( rlid );
    if ( !rl_ ) return;

    rl_->ref();
    rl_->nodeChanged.notify( mCB(this,RandomTrackDisplay,geomChangeCB) );

    setName( mToUiStringTodo(rl_->name()) );
    TypeSet<BinID> bids;
    rl_->allNodePositions( bids );
    setNodePositions( bids );
}


int RandomTrackDisplay::getRandomLineID() const
{ return rl_ ? rl_->ID() : -1; }


Geometry::RandomLine* RandomTrackDisplay::getRandomLine()
{ return rl_; }


void RandomTrackDisplay::setDisplayTransformation( const mVisTrans* t )
{
    panelstrip_->setDisplayTransformation( t );
    polyline_->setDisplayTransformation( t );
    markerset_->setDisplayTransformation( t );
    dragger_->setDisplayTransformation( t );
}


const mVisTrans* RandomTrackDisplay::getDisplayTransformation() const
{
    return panelstrip_->getDisplayTransformation();
}


/* OSG-TODO: Thorough testing of the (highly inscrutable) datatransform_
   dependency. Other than for Seis2DDisplay, the setting of the Coin-based
   SplitTexture was depending on it. Which is a strange difference! (JCG) */

float RandomTrackDisplay::appliedZRangeStep() const
{
    float step = datatransform_ ? datatransform_->getGoodZStep() : SI().zStep();
    if ( scene_ )
	step = scene_->getTrcKeyZSampling().zsamp_.step;

    return step;
}


TrcKeyZSampling RandomTrackDisplay::getTrcKeyZSampling( int attrib ) const
{
    TrcKeyZSampling cs( false );
    TypeSet<BinID> nodes;
    getAllNodePos( nodes );
    for ( int idx=0; idx<nodes.size(); idx++ )
	cs.hsamp_.include( nodes[idx] );

    cs.zsamp_.setFrom( getDepthInterval() );
    cs.zsamp_.step = appliedZRangeStep();
    cs.hsamp_.survid_ = Survey::GM().default3DSurvID();
    return cs;
}


void RandomTrackDisplay::setDepthInterval( const Interval<float>& intv )
{
    depthrg_ = intv;
    if ( datatransform_ )
	return;

    setPanelStripZRange( intv );
    dragger_->setDepthRange( intv );

    moving_.trigger();
}


Interval<float> RandomTrackDisplay::getDepthInterval() const
{
    return panelstrip_->getZRange();
}


void RandomTrackDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, taskr );

    updatePanelStripPath();
    setPanelStripZRange( panelstrip_->getZRange() );
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{ return depthrg_; }


int RandomTrackDisplay::nrNodes() const
{ return nodes_.size(); }


#define mUpdateRandomLineGeometry( functioncall ) \
{ \
    if ( rl_ && nrgeomchangecbs_==0 ) \
    { \
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB)); \
	rl_->functioncall; \
	rl_->nodeChanged.notify( mCB(this,RandomTrackDisplay,geomChangeCB)); \
    } \
}

void RandomTrackDisplay::addNode( const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	nodes_ += sbid;
	dragger_->insertKnot( nodes_.size()-1, Coord(sbid.inl(),sbid.crl()) );

	if ( ismanip_ )
	    dragger_->showAdjacentPanels( nodes_.size()-1, true );
	else
	    updatePanelStripPath();

	if ( nodes_.size() > 1 && !panelstrip_->isOn() )
	    panelstrip_->turnOn( true );

	moving_.trigger();
    }
}


void RandomTrackDisplay::insertNode( int nodeidx, const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	nodes_.insert( nodeidx, sbid );
	dragger_->insertKnot( nodeidx, Coord(sbid.inl(),sbid.crl()) );

	if ( ismanip_ )
	    dragger_->showAdjacentPanels( nodeidx, true );
	else
	{
	    for ( int idx=0; idx<nrAttribs(); idx++ )
		updateChannels( idx, 0 );
	    updatePanelStripPath();
	}

	moving_.trigger();
    }
}


BinID RandomTrackDisplay::getNodePos( int nodeidx ) const
{
    return nodes_[nodeidx];
}


BinID RandomTrackDisplay::getManipNodePos( int nodeidx ) const
{
    const Coord crd = dragger_->getKnot( nodeidx );
    return BinID( SI().inlRange(false).snap(crd.x),
		  SI().crlRange(false).snap(crd.y) );
}


void RandomTrackDisplay::getAllNodePos( TypeSet<BinID>& nodes ) const
{
    const int nrnodes = nrNodes();
    for ( int idx=0; idx<nrnodes; idx++ )
	nodes += getManipNodePos( idx );
}


void RandomTrackDisplay::setNodePos( int nodeidx, const BinID& bid )
{ setNodePos( nodeidx, bid, true ); }


void RandomTrackDisplay::setNodePos( int nodeidx, const BinID& bid, bool check )
{
    const BinID sbid = snapPosition(bid);
    if ( !check || checkPosition(sbid) )
    {
	nodes_[nodeidx] = sbid;
	dragger_->setKnot( nodeidx, Coord(sbid.inl(),sbid.crl()) );
	mUpdateRandomLineGeometry( setNodePosition(nodeidx,sbid) );
	updatePanelStripPath();
	moving_.trigger();
    }
}


static bool decoincideNodes( const TypeSet<BinID>& nodes,
			     TypeSet<BinID>& uniquenodes )
{
    uniquenodes.erase();
    if ( nodes.isEmpty() )
	return false;
    uniquenodes += nodes[0];

    for ( int idx=1; idx<nodes.size(); idx++ )
    {
	const BinID prev = uniquenodes[uniquenodes.size()-1];
	const BinID biddif = prev - nodes[idx];
	const int nrsteps = mMAX( abs(biddif.inl())/SI().inlStep(),
				  abs(biddif.crl())/SI().crlStep() );
	const Coord dest = SI().transform( nodes[idx] );
	const Coord crddif = SI().transform(prev) - dest;
	for ( int step=0; step<nrsteps; step++ )
	{
	    const BinID newnode = SI().transform( dest+(crddif*step)/nrsteps );
	    if ( !uniquenodes.isPresent(newnode) )
	    {
		uniquenodes += newnode;
		break;
	    }
	}
    }
    return true;
}


bool RandomTrackDisplay::setNodePositions( const TypeSet<BinID>& newbids )
{
    TypeSet<BinID> uniquebids;
   if ( !decoincideNodes( newbids, uniquebids ) ) return false;

    if ( uniquebids.size() < 2 )
	return false;
    while ( nrNodes() > uniquebids.size() )
	removeNode( nrNodes()-1 );

    if ( uniquebids.size() > 50 ) // Workaround when having a lot of nodes
    {				  // TODO: Make better fix
	while ( nrNodes()>0 )
	{
	    nodes_.removeSingle( 0 );
	    dragger_->removeKnot( 0 );
	}

	while ( rl_->nrNodes() > 0 )
	    mUpdateRandomLineGeometry( removeNode(0) );

	for ( int idx=0; idx<uniquebids.size(); idx++ )
	{
	    const BinID sbid = snapPosition( uniquebids[idx] );
	    if ( checkPosition(sbid) )
	    {
		nodes_ += sbid;
		dragger_->insertKnot( nodes_.size()-1,
				      Coord(sbid.inl(), sbid.crl()) );
		mUpdateRandomLineGeometry( addNode(sbid) );
	    }
	}

	updatePanelStripPath();
	moving_.trigger();
	return true;
    }

    for ( int idx=0; idx<uniquebids.size(); idx++ )
    {
	const BinID bid = uniquebids[idx];

	if ( idx < nrNodes() )
	    setNodePos( idx, bid, false );
	else
	    addNode( bid );
    }

    return true;
}


void RandomTrackDisplay::removeNode( int nodeidx )
{
    if ( nrNodes()< 3 )
    {
	pErrMsg("Can't remove node");
	return;
    }

    nodes_.removeSingle(nodeidx);
    dragger_->removeKnot( nodeidx );
    mUpdateRandomLineGeometry( removeNode(nodeidx) );
    updatePanelStripPath();
}


void RandomTrackDisplay::removeAllNodes()
{
    for ( int idx=nodes_.size()-1; idx>=0; idx-- )
    {
	dragger_->removeKnot( idx );
	mUpdateRandomLineGeometry( removeNode(idx) );
    }

    for ( int idx=0; idx<nrAttribs(); idx++ )
	setDataPackID( idx, -1, 0 );

    nodes_.erase();
    updatePanelStripPath();
    panelstrip_->turnOn( false );
    moving_.trigger();
}


void RandomTrackDisplay::getTraceKeyPath( TrcKeyPath& path,
                                          TypeSet<Coord>* crds ) const
{
    if ( crds ) crds->erase();
    
    TypeSet<BinID> nodes;
    getAllNodePos( nodes );

    TypeSet<BinID> bids;
    TypeSet<int> segments;
    Geometry::RandomLine::getPathBids( nodes, bids, false, &segments );

    path.erase();
    const Pos::SurvID survid = s3dgeom_->getSurvID();
    int curlinesegment = -1;
    Line2 curline;
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	path += TrcKey( survid, bids[idx] );
        if ( !crds )
            continue;
        
        if ( curlinesegment!=segments[idx] )
        {
            curlinesegment = -1;
            
            const int cursegment = segments[idx];
            if ( cursegment<nodes.size()-1 )
            {
                const BinID startnode = nodes[segments[idx]];
                const BinID stopnode = nodes[segments[idx]+1];
                const Coord startpos = s3dgeom_->transform ( startnode );
                const Coord stoppos = s3dgeom_->transform( stopnode );
                
                if ( startpos.isDefined() && stoppos.isDefined() &&
                     startpos.sqDistTo( stoppos )>1e-3 )
                {
                    curline = Line2( startpos, stoppos );
                    curlinesegment = cursegment;
                }
            }
        }
        
        Coord pathpos = s3dgeom_->transform(bids[idx]);
        if ( curlinesegment>=0 )
            pathpos = curline.closestPoint( pathpos );
        
        (*crds) += pathpos;
    }
}


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
{ getDataTraceBids( bids, 0 ); }


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids,
					   TypeSet<int>* segments ) const
{
    const_cast<RandomTrackDisplay*>(this)->trcspath_.erase();
    TypeSet<BinID> nodes;
    getAllNodePos( nodes );
    Geometry::RandomLine::getPathBids( nodes, bids, true, segments );
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( !idx || bids[idx]!=trcspath_.last() )
	    const_cast<RandomTrackDisplay*>(this)->trcspath_.add( bids[idx] );
    }
}


TypeSet<Coord> RandomTrackDisplay::getTrueCoords() const
{
    const int nrnodes = nrNodes();
    TypeSet<Coord> coords;
    for ( int kidx=1; kidx<nrnodes; kidx++ )
    {
	BinID start = getNodePos(kidx-1);
	BinID stop = getNodePos(kidx);
	const int nrinl = int(abs(stop.inl()-start.inl()) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl()-start.crl()) / SI().crlStep() + 1);
	const int nrtraces = nrinl > nrcrl ? nrinl : nrcrl;
	const Coord startcoord = SI().transform( start );
	const Coord stopcoord = SI().transform( stop );
	const float delx = (float) ( stopcoord.x - startcoord.x ) / nrtraces;
	const float dely = (float) ( stopcoord.y - startcoord.y ) / nrtraces;

	for ( int idx=0; idx<nrtraces; idx++ )
	{
	    const float x = (float) ( startcoord.x + delx * idx );
	    const float y = (float) ( startcoord.y + dely * idx );
	    coords += Coord( x, y );
	}
    }
    return coords;
}


bool RandomTrackDisplay::setDataPackID( int attrib, DataPack::ID dpid,
					TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack* datapack = dpm.obtain( dpid );
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,datapack);
    if ( !randsdp || randsdp->isEmpty() )
    {
	dpm.release( dpid );
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
	channels_->turnOn( false );
	return false;
    }

    dpm.release( datapackids_[attrib] );
    datapackids_[attrib] = dpid;

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );
    return true;
}


DataPack::ID RandomTrackDisplay::getDataPackID( int attrib ) const
{
    return datapackids_.validIdx(attrib) ? datapackids_[attrib]
					 : DataPack::cNoID();
}


DataPack::ID RandomTrackDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TypeSet<DataPack::ID>& dpids = transfdatapackids_;
	return dpids.validIdx(attrib) ? dpids[attrib] : DataPack::cNoID();
    }

    return getDataPackID( attrib );
}


const ZAxisTransform* RandomTrackDisplay::getZAxisTransform() const
{ return datatransform_; }


bool RandomTrackDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* t )
{
    if ( datatransform_ )
    {
	if ( voiidx_!=-1 )
	    datatransform_->removeVolumeOfInterest(voiidx_);

	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,RandomTrackDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    voiidx_ = -1;
    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, true );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,RandomTrackDisplay,dataTransformCB) );
    }
    return true;
}


void RandomTrackDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	createTransformedDataPack( idx, 0 );
	updateChannels( idx, 0 );
    }
}


void RandomTrackDisplay::updateRanges(bool resetinlcrl, bool resetz )
{
    if ( resetz )
    {
	const Interval<float>& depthrg = datatransform_->getZInterval(false);
	setPanelStripZRange( depthrg );
	dragger_->setDepthRange( depthrg );
	moving_.trigger();
    }
}


void RandomTrackDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    ConstDataPackRef<RandomSeisDataPack> randsdp = dpm.obtain( dpid );
    if ( !randsdp ) return;

    const int nrversions = randsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    int newsz0 = randsdp->data().info().getSize( 1 );
    int newsz1 = randsdp->data().info().getSize( 2 );
    bool differentattribsizes = false, onlycurrent = true;
    if ( nrAttribs() > 1 )
    {
	const int oldchannelsz0 =
	    (channels_->getSize(0,1)+resolution_) / (resolution_+1);
	const int oldchannelsz1 =
	    (channels_->getSize(0,2)+resolution_) / (resolution_+1);

	differentattribsizes = newsz0!=oldchannelsz0 || newsz1!=oldchannelsz1;
	onlycurrent = newsz0<=oldchannelsz0 && newsz1<=oldchannelsz1;
	if ( newsz0 < oldchannelsz0 ) newsz0 = oldchannelsz0;
	if ( newsz1 < oldchannelsz1 ) newsz1 = oldchannelsz1;
    }

    for ( int attribidx=0; attribidx<nrAttribs(); attribidx++ )
    {
	if ( onlycurrent && attribidx!=attrib )
	    continue;

	randsdp = dpm.obtain( getDisplayedDataPackID(attribidx) );
	if ( !randsdp ) continue;

	for ( int idx=0; idx<randsdp->nrComponents(); idx++ )
	{
	    const Array3DImpl<float>& array = randsdp->data( idx );
	    const int sz0 = 1 + (newsz0-1) * (resolution_+1);
	    const int sz1 = 1 + (newsz1-1) * (resolution_+1);
	    const float* arr = array.getData();
	    OD::PtrPolicy cp = OD::UsePtr;

	    if ( !arr || resolution_>0 || differentattribsizes )
	    {
		mDeclareAndTryAlloc( float*, tmparr, float[sz0*sz1] );
		if ( !tmparr ) continue;

		if ( resolution_==0 && !differentattribsizes )
		    array.getAll( tmparr );
		else
		{
		    Array2DSlice<float> slice2d( array );
		    slice2d.setDimMap( 0, 1 );
		    slice2d.setDimMap( 1, 2 );
		    slice2d.setPos( 0, 0 );
		    slice2d.init();

		    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
		    Array2DReSampler<float,float> resampler(
			    slice2d, tmparr, sz0, sz1, true );
		    resampler.setInterpolate( true );
		    TaskRunner::execute( taskr, resampler );
		}

		arr = tmparr;
		cp = OD::TakeOverPtr;
	    }

	    channels_->setSize( attribidx, 1, sz0, sz1 );
	    channels_->setUnMappedData( attribidx, idx, arr, cp, 0,
					interactivetexturedisplay_ );
	}
    }

    setPanelStripZRange( panelstrip_->getZRange() );
    channels_->turnOn( true );
}


void RandomTrackDisplay::createTransformedDataPack(
				int attrib, TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDataPackID( attrib );
    ConstDataPackRef<RandomSeisDataPack> randsdp = dpm.obtain( dpid );
    if ( !randsdp || randsdp->isEmpty() )
	return;

    DataPack::ID outputid = DataPack::cNoID();
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    const TrcKeyPath& path = randsdp->getPath();
	    TrcKeyZSampling tkzs( false );
	    for ( int idx=0; idx<path.size(); idx++ )
		tkzs.hsamp_.include( path[idx].pos() );
	    tkzs.zsamp_ = panelstrip_->getZRange();
	    tkzs.zsamp_.step = scene_ ? scene_->getTrcKeyZSampling().zsamp_.step
				      : datatransform_->getGoodZStep();
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    datatransform_->loadDataIfMissing( voiidx_, taskr );
	}

	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( randsdp.ptr() );
	transformer.setOutput( outputid );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.execute();
    }

    dpm.release( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = outputid;
    dpm.obtain( outputid );
}


void RandomTrackDisplay::updatePanelStripPath()
{
    if ( nodes_.size()<2 || getUpdateStageNr() )
	return;

    TypeSet<BinID> trcbids;
    getDataTraceBids( trcbids );	// Will update trcspath_

    TypeSet<Coord> pathcrds;
    TypeSet<float> mapping;
    pathcrds.setCapacity( nodes_.size(), false );
    mapping.setCapacity( nodes_.size(), false );

    int nodeidx = 0;
    for ( int trcidx=0; trcidx<trcspath_.size(); trcidx++ )
    {
	while ( nodeidx<nodes_.size() && trcspath_[trcidx]==nodes_[nodeidx] )
	{
	    pathcrds += Coord( nodes_[nodeidx].inl(), nodes_[nodeidx].crl() );
	    mapping += mCast( float, trcidx*(resolution_+1) );
	    nodeidx++;
	}
    }

    if ( !trcspath_.isEmpty() )
    {
	if ( mapping.size()!=nodes_.size() ||
	     mNINT64(mapping.last())!=(trcspath_.size()-1)*(resolution_+1) )
	{
	    pErrMsg( "Unexpected state while texture mapping" );
	}
    }

    panelstrip_->setPath( pathcrds );
    panelstrip_->setPath2TextureMapping( mapping );
}


void RandomTrackDisplay::setPanelStripZRange( const Interval<float>& rg )
{
    panelstrip_->unsetZRange2TextureMapping();
    panelstrip_->setZRange( rg );

    if ( getUpdateStageNr() )
    {
	const float mapfactor = updatestageinfo_.mapfactor_;
	const float diff = updatestageinfo_.oldzrgstart_ - rg.start;
	panelstrip_->setZTextureShift( diff * mapfactor );
	const Interval<float> mapping( 0.0f, rg.width() * mapfactor );
	panelstrip_->setZRange2TextureMapping( mapping );
    }
}


void RandomTrackDisplay::annotateNextUpdateStage( bool yn )
{
    if ( !yn )
    {
	panelstrip_->setZTextureShift( 0.0 );
	SurveyObject::annotateNextUpdateStage( false );
	updatePanelStripPath();
	panelstrip_->freezeDisplay( false );

	for ( int idx=0; idx<nrNodes(); idx++ )
	    dragger_->showAdjacentPanels( idx, false );
    }
    else
    {
	if ( !getUpdateStageNr() )
	{
	    updatestageinfo_.oldzrgstart_ = getDepthInterval().start;
	    updatestageinfo_.mapfactor_ =
		(channels_->getSize(0,2)-1) / getDepthInterval().width();
	}
	else
	    panelstrip_->freezeDisplay( false );	// thaw to refreeze

	SurveyObject::annotateNextUpdateStage( true );
	panelstrip_->freezeDisplay( true );
    }
}


bool RandomTrackDisplay::canAddNode( int nodenr ) const
{
    if ( lockgeometry_ ) return false;
    if ( nodenr<0 ) nodenr=0;
    if ( nodenr>nrNodes() ) nodenr=nrNodes();

    const BinID newpos = proposeNewPos(nodenr);
    return checkPosition(newpos);
}


void RandomTrackDisplay::addNode( int nodenr )
{
    if ( nodenr<0 ) nodenr=0;
    if ( nodenr>nrNodes() ) nodenr=nrNodes();

    if ( !canAddNode(nodenr) ) return;

    ismanip_ = true;

    const BinID newpos = proposeNewPos(nodenr);
    rl_->insertNode( nodenr, newpos );
    if ( nodenr!=0 && nodenr!=nrNodes()-1 )
	dragger_->showAdjacentPanels( nodenr, false );
}


BinID RandomTrackDisplay::proposeNewPos(int nodenr ) const
{
    BinID res;
    if ( !nodenr )
	res = getNodePos(0)-(getNodePos(1)-getNodePos(0));
    else if ( nodenr>=nrNodes() )
	res = getNodePos(nrNodes()-1) +
	      (getNodePos(nrNodes()-1)-getNodePos(nrNodes()-2));
    else
    {
	res = getNodePos(nodenr)+getNodePos(nodenr-1);
	res.inl() /= 2;
	res.crl() /= 2;
    }

    res.inl() = mMIN( SI().inlRange(true).stop, res.inl() );
    res.inl() = mMAX( SI().inlRange(true).start, res.inl() );
    res.crl() = mMIN( SI().crlRange(true).stop, res.crl() );
    res.crl() = mMAX( SI().crlRange(true).start, res.crl() );

    SI().snap(res, BinID(0,0) );

    return res;
}


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip_;
}


void RandomTrackDisplay::acceptManipulation()
{
    if ( !datatransform_ )
	setDepthInterval( dragger_->getDepthRange() );
    else
    {
	setPanelStripZRange( dragger_->getDepthRange() );
	moving_.trigger();
    }

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	setNodePos( idx, getManipNodePos(idx), false);
	if ( !getUpdateStageNr() )
	    dragger_->showAdjacentPanels( idx, false );
    }

    updatePanelStripPath();
    dragger_->showAllPanels( false );
    ismanip_ = false;
}


void RandomTrackDisplay::resetManipulation()
{
    if ( !datatransform_ )
	dragger_->setDepthRange( getDepthInterval() );

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	const BinID bid = getNodePos(idx);
	dragger_->setKnot(idx, Coord(bid.inl(),bid.crl()));
	dragger_->showAdjacentPanels( idx, false );
    }

    dragger_->showAllPanels( false );
    ismanip_ = false;
    dragger_->turnOn( false );
}


void RandomTrackDisplay::showManipulator( bool yn )
{
    if ( polylinemode_ ) return;

    if ( lockgeometry_ ) yn = false;
    dragger_->turnOn( yn );
    panelstrip_->enableTraversal(visBase::cDraggerIntersecTraversalMask(),!yn);
}


bool RandomTrackDisplay::isManipulatorShown() const
{ return dragger_->isOn(); }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    int nodeidx = getSelNodeIdx();
    if ( nodeidx >= 0 )
    {
	BinID binid = getManipNodePos( nodeidx );
	str = "Node "; str += nodeidx;
	str += " Inl/Crl: ";
	str += binid.inl(); str += "/"; str += binid.crl();
    }

    return str;
}


void RandomTrackDisplay::geomChangeCB( CallBacker* cb )
{
    nrgeomchangecbs_++;

    mCBCapsuleUnpack(const Geometry::RandomLine::ChangeData&,cd,cb);

    if ( cd.ev_ == Geometry::RandomLine::ChangeData::Added )
    {
	const int nodeidx = cd.nodeidx_;
	if ( cd.nodeidx_ < nrNodes() )
	{
	    const BinID nodepos = rl_->nodePosition( nodeidx );
	    nodes_[nodeidx] = nodepos;
	    dragger_->setKnot( nodeidx, Coord(nodepos.inl(),nodepos.crl()) );
	    dragger_->showAllPanels( true );
	}
	else
	    addNode( rl_->nodePosition(nodeidx) );
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Inserted )
    {
	insertNode( cd.nodeidx_, rl_->nodePosition(cd.nodeidx_) );
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Moving )
    {
	const int nodeidx = cd.nodeidx_;
	const BinID nodepos = rl_->nodePosition( nodeidx );
	nodes_[nodeidx] = nodepos;
	dragger_->setKnot( nodeidx, Coord(nodepos.inl(),nodepos.crl()) );
	dragger_->showAllPanels( true );
	if ( canDisplayInteractively() )
	{
	    if ( originalresolution_ < 0 )
		originalresolution_ = resolution_;

	    resolution_ = 0;
	    interactivetexturedisplay_ = true;
	    acceptManipulation();
	    ismanip_ = true;
	    updateSel();
	}
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Moved )
    {
	if ( originalresolution_ >= 0 )
	{
	    resolution_ = originalresolution_;
	    originalresolution_ = -1;
	}

	draggerMoveFinished(0);
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Removed )
    {
	removeNode( cd.nodeidx_ );
    }

    nrgeomchangecbs_--;
}


void RandomTrackDisplay::nodeMoved( CallBacker* cb )
{
    ismanip_ = true;
    mCBCapsuleUnpack(int,sel,cb);
    selnodeidx_ = sel;

    for ( int idx=abs(sel); idx>=abs(sel)-1; idx-- )
    {
	const BinID bid = getManipNodePos( idx );
	dragger_->showAdjacentPanels( idx, getNodePos(idx)!=bid );
	mUpdateRandomLineGeometry( setNodePosition(idx,bid,true) );

	if ( sel>=0 ) break;
    }

    if ( getDepthInterval().isEqual(dragger_->getDepthRange(),1e-5) )
	dragger_->showAllPanels( false );
    else
	dragger_->showAllPanels( true );

    nodemoving_.trigger();

    if ( canDisplayInteractively() )
    {
	if ( originalresolution_ < 0 )
	    originalresolution_ = resolution_;

	resolution_ = 0;
	interactivetexturedisplay_ = true;
	updateSel();
    }
}


bool RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    const TrcKeySampling& hs = SI().sampling(true).hsamp_;
    if ( !hs.includes(binid) )
	return false;

    BinID snapped( binid );
    SI().snap( snapped, BinID(0,0) );
    if ( snapped != binid )
	return false;

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	if ( getNodePos(idx) == binid )
	    return false;
    }

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const TrcKeySampling& hs = SI().sampling(true).hsamp_;
    if ( binid.inl() < hs.start_.inl() ) binid.inl() = hs.start_.inl();
    if ( binid.inl() > hs.stop_.inl() ) binid.inl() = hs.stop_.inl();
    if ( binid.crl() < hs.start_.crl() ) binid.crl() = hs.start_.crl();
    if ( binid.crl() > hs.stop_.crl() ) binid.crl() = hs.stop_.crl();

    SI().snap( binid, BinID(0,0) );
    return binid;
}


SurveyObject::AttribFormat RandomTrackDisplay::getAttributeFormat( int ) const
{ return SurveyObject::Traces; }


int RandomTrackDisplay::getClosestPanelIdx( const Coord& xypos ) const
{
    TypeSet<Coord> coords;
    for ( int idx=0; idx<nodes_.size(); idx++ )
	coords += SI().transform( nodes_[idx] );

    const PolyLineND<Coord> polyline( coords );
    int panelidx = -1;
    polyline.distTo( xypos, &panelidx );

    return panelidx;
}


#define mFindTrc(inladd,crladd) \
    if ( idx<0 ) \
    { \
	BinID bid( binid.inl() + step.inl() * (inladd),\
		   binid.crl() + step.crl() * (crladd) );\
	idx = bids.indexOf( bid ); \
    }
Coord3 RandomTrackDisplay::getNormal( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    TypeSet<int> segments;
    getDataTraceBids( bids, &segments );
    int idx = bids.indexOf(binid);
    if ( idx==-1 )
    {
	const BinID step( SI().inlStep(), SI().crlStep() );
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( idx==-1 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

	if ( idx<0 )
	    return Coord3::udf();
    }

    const TypeSet<Coord>& coords = panelstrip_->getPath();
    const Coord pos0 = coords[segments[idx]];
    const Coord pos1 = coords[segments[idx]+1];
    const BinID bid0( mNINT32(pos0.x), mNINT32(pos0.y));
    const BinID bid1( mNINT32(pos1.x), mNINT32(pos1.y));

    const Coord dir = SI().transform(bid0)-SI().transform(bid1);
    const float dist = (float) dir.abs();

    if ( dist<=mMIN(SI().inlDistance(),SI().crlDistance()) )
	return Coord3::udf();

    return Coord3( dir.y, -dir.x, 0 );
}

#undef mFindTrc


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    // TODO: Compared to calcDist(.) of 2d lines, this implementation can't
    // be right. I wonder whether this is actually used somewhere? (JCG)

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    if ( !bids.isPresent(binid) )
	return mUdf(float);

    float zdiff = 0;
    const Interval<float> intv = getDataTraceRange();
    if ( xytpos.z < intv.start )
	zdiff = (float) ( intv.start - xytpos.z );
    else if ( xytpos.z > intv.stop )
	zdiff = (float) ( xytpos.z - intv.stop );

    return zdiff;
}


void RandomTrackDisplay::lockGeometry( bool yn )
{
    lockgeometry_ = yn;
    if ( yn ) showManipulator( false );
}


bool RandomTrackDisplay::isGeometryLocked() const
{ return lockgeometry_; }


SurveyObject* RandomTrackDisplay::duplicate( TaskRunner* taskr ) const
{
    RandomTrackDisplay* rtd = new RandomTrackDisplay;
    rtd->setDepthInterval( getDataTraceRange() );
    TypeSet<BinID> positions;
    for ( int idx=0; idx<nrNodes(); idx++ )
	positions += getNodePos( idx );
    rtd->setNodePositions( positions );
    rtd->lockGeometry( isGeometryLocked() );
    rtd->setZAxisTransform( datatransform_, taskr );

    while ( nrAttribs() > rtd->nrAttribs() )
	rtd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const Attrib::SelSpec* selspec = getSelSpec( idx );
	if ( selspec ) rtd->setSelSpec( idx, *selspec );
	rtd->setDataPackID( idx, getDataPackID(idx), taskr );
	const ColTab::MapperSetup* mappersetup = getColTabMapperSetup( idx );
	if ( mappersetup )
	    rtd->setColTabMapperSetup( idx, *mappersetup, taskr );
	const ColTab::Sequence* colseq = getColTabSequence( idx );
	if ( colseq ) rtd->setColTabSequence( idx, *colseq, taskr );
    }

    return rtd;
}


MultiID RandomTrackDisplay::getMultiID() const
{
    return rl_ ? rl_->getMultiID() : MultiID::udf();
}


void RandomTrackDisplay::fillPar( IOPar& par ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par );

    const Interval<float> depthrg = getDataTraceRange();
    par.set( sKeyDepthInterval(), depthrg );

    const int nrnodes = nrNodes();
    par.set( sKeyNrKnots(), nrnodes );

    for ( int idx=0; idx<nrnodes; idx++ )
    {
	BufferString key = sKeyKnotPrefix(); key += idx;
	par.set( key, getNodePos(idx) );
    }

    par.set( sKey::Version(), 3 );
    par.setYN( sKeyLockGeometry(), lockgeometry_ );
}


bool RandomTrackDisplay::usePar( const IOPar& par )
{
    if ( !visSurvey::MultiTextureSurveyObject::usePar( par ) )
	return false;

    par.getYN( sKeyLockGeometry(), lockgeometry_ );

    Interval<float> intv;
    if ( par.get( sKeyDepthInterval(), intv ) )
	setDepthInterval( intv );

    int nrnodes = 0;
    par.get( sKeyNrKnots(), nrnodes );

    BufferString key; BinID pos;
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	key = sKeyKnotPrefix(); key += idx;
	par.get( key, pos );
	if ( idx < 2 )
	    setNodePos( idx, pos );
	else
	    addNode( pos );
    }

    return true;
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo& ei,
					  IOPar& par ) const
{
    MultiTextureSurveyObject::getMousePosInfo( ei, par );
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo&,
					  Coord3& pos, BufferString& val,
					  BufferString& info ) const
{
    info = mFromUiStringTodo(name());
    getValueString( pos, val );
}


bool RandomTrackDisplay::getCacheValue( int attrib,int version,
					const Coord3& pos,float& val ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    ConstDataPackRef<RandomSeisDataPack> randsdp = dpm.obtain( dpid );
    if ( !randsdp || randsdp->isEmpty() )
	return false;

    const BinID bid( SI().transform(pos) );
    const TrcKey trckey = Survey::GM().traceKey(
	    Survey::GM().default3DSurvID(), bid.inl(), bid.crl() );
    const int trcidx = randsdp->getNearestGlobalIdx( trckey );
    const int sampidx = randsdp->getZRange().nearestIndex( pos.z );
    const Array3DImpl<float>& array = randsdp->data( version );
    if ( !array.info().validPos(0,trcidx,sampidx) )
	return false;

    val = array.get( 0, trcidx, sampidx );
    return true;
}


void RandomTrackDisplay::addCache()
{
    datapackids_ += DataPack::cNoID();
    transfdatapackids_ += DataPack::cNoID();
}


void RandomTrackDisplay::removeCache( int attrib )
{
    DPM(DataPackMgr::SeisID()).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );

    DPM(DataPackMgr::SeisID()).release( transfdatapackids_[attrib] );
    transfdatapackids_.removeSingle( attrib );
}


void RandomTrackDisplay::swapCache( int a0, int a1 )
{
    datapackids_.swap( a0, a1 );
    transfdatapackids_.swap( a0, a1 );
}


void RandomTrackDisplay::emptyCache( int attrib )
{
    DPM(DataPackMgr::SeisID()).release( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).release( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = DataPack::cNoID();

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::CopyPtr, 0 );
}


bool RandomTrackDisplay::hasCache( int attrib ) const
{
    return datapackids_[attrib] != DataPack::cNoID();
}


void RandomTrackDisplay::setSceneEventCatcher( visBase::EventCatcher* evnt )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
			    mCB(this,RandomTrackDisplay,pickCB) );
	eventcatcher_->eventhappened.remove(
			    mCB(this,RandomTrackDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = evnt;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
			    mCB(this,RandomTrackDisplay,pickCB) );
	eventcatcher_->eventhappened.notify(
			    mCB(this,RandomTrackDisplay,updateMouseCursorCB) );
    }
}


void RandomTrackDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !isManipulatorShown() || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
    {
	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(false,0),
				 mousecursor_ );

	if ( mousecursor_.shape_ != MouseCursor::GreenArrow )
	    return;

	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(true,1),
				 mousecursor_ );

	if ( mousecursor_.shape_ != MouseCursor::GreenArrow )
	    mousecursor_.shape_ = MouseCursor::Rotator;
    }
}


void RandomTrackDisplay::pickCB( CallBacker* cb )
{
    if ( !polylinemode_ ) return;

     mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

     if ( !eventinfo.pressed && eventinfo.type==visBase::MouseClick &&
			    OD::leftMouseButton( eventinfo.buttonstate_ ) )
    {
	const Coord3 pos = eventinfo.worldpickedpos;
	Coord3 inlcrlpos(pos);
	BinID bid = SI().transform( inlcrlpos );
	inlcrlpos.x = bid.inl(); inlcrlpos.y = bid.crl();

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::altKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{

	    if ( !eventinfo.pickedobjids.isPresent(markerset_->id()) )
		return;

	    const mVisTrans* transform = markerset_->getDisplayTransformation();

	    if ( transform )
		transform->transform( inlcrlpos );

	    removePickPos( inlcrlpos );
	    eventcatcher_->setHandled();
	    return;
	}

	if ( !checkValidPick( eventinfo, pos ) )
	    return;
	setPickPos( inlcrlpos );
	eventcatcher_->setHandled();
    }
}


void RandomTrackDisplay::setPolyLineMode( bool mode )
{
    polylinemode_ = mode;
    polyline_->turnOn( polylinemode_ );
    markerset_->turnOn( polylinemode_ );
    panelstrip_->turnOn( !polylinemode_ );
    dragger_->turnOn( !polylinemode_ );
}


bool RandomTrackDisplay::checkValidPick( const visBase::EventInfo& evi,
					 const Coord3& pos ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( eventid==-1 && pickedobj->isPickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( !so || !so->allowsPicks() )
	    continue;

	mDynamicCastGet(const HorizonDisplay*,hd,so);
	mDynamicCastGet(const PlaneDataDisplay*,pdd,so);
	validpicksurface = hd ||
		(pdd && pdd->getOrientation() == OD::ZSlice);

	if ( eventid!=-1 )
	    break;
    }

    return validpicksurface;
 }


void RandomTrackDisplay::setPickPos( const Coord3& pos )
{
    polyline_->addPoint( pos );
    markerset_->addPos( pos );
}


void RandomTrackDisplay::removePickPos( const Coord3& pickpos )
{
    const int markeridx = markerset_->findClosestMarker( pickpos, true );
    if ( markeridx == -1 )
	return;
    polyline_->removePoint( markeridx );
    markerset_->removeMarker( markeridx );
}


void RandomTrackDisplay::setColor( Color color )
{
    polyline_->getMaterial()->setColor( color );
    markerset_->setMarkersSingleColor( color );
}


bool RandomTrackDisplay::createFromPolyLine()
{
    TypeSet<BinID> bids;
    for ( int idx=0; idx<polyline_->size(); idx++ )
    {
	Coord pos = polyline_->getPoint( idx );
	bids += BinID( (int)pos.x, (int)pos.y );
    }

    return setNodePositions( bids );
}


void RandomTrackDisplay::setPixelDensity( float dpi )
{
    MultiTextureSurveyObject::setPixelDensity( dpi );

    if ( polyline_ )
	polyline_->setPixelDensity( dpi );

    if ( markerset_ )
	markerset_->setPixelDensity( dpi );
}


void RandomTrackDisplay::draggerMoveFinished( CallBacker* )
{
    if ( originalresolution_ >= 0 )
    {
	resolution_ = originalresolution_;
	originalresolution_ = -1;
    }

    interactivetexturedisplay_ = false;
    ismanip_ = true;
    updateSel();
}


void RandomTrackDisplay::draggerRightClick( CallBacker* )
{
    triggerRightClick( dragger_->rightClickedEventInfo() );
}

} // namespace visSurvey
