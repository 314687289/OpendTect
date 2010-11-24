/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.131 2010-11-24 14:00:54 cvsbruno Exp $";

#include "viswelldisplay.h"

#include "basemap.h"
#include "dataclipper.h"
#include "draw.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "viswell.h"

#include "welldisp.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#define		mPickSz 	3
#define         mPickType	3

#define mGetWD(act) Well::Data* wd = getWD(); if ( !wd ) act;
#define mMeter2Feet(val) val *= mToFeetFactor;
#define dpp(param) wd->displayProperties().param


mCreateFactoryEntry( visSurvey::WellDisplay );

namespace visSurvey
{

class WellDisplayBaseMapObject : public BaseMapObject
{
public:
			WellDisplayBaseMapObject(WellDisplay* wd);

    const char*		getType() const;
    void		updateGeometry();
    int			nrShapes() const;
    const char*		getShapeName(int);
    void		getPoints(int,TypeSet<Coord>& res) const;
    char		connectPoints(int) const;
    const Color*	getColor(int) const;

protected:
    WellDisplay*	wd_;
};


WellDisplayBaseMapObject::WellDisplayBaseMapObject( WellDisplay* wd )
    : BaseMapObject( wd->name() )
    , wd_( wd )
{}


const char* WellDisplayBaseMapObject::getType() const
{ return "Well"; }


void WellDisplayBaseMapObject::updateGeometry()
{ changed.trigger(); }


int WellDisplayBaseMapObject::nrShapes() const
{ return 1; }


const char* WellDisplayBaseMapObject::getShapeName(int)
{ return wd_->name(); }


void WellDisplayBaseMapObject::getPoints( int, TypeSet<Coord>& res ) const
{
    if ( wd_->getWD() )
	res += wd_->getWD()->info().surfacecoord;
}


char WellDisplayBaseMapObject::connectPoints( int ) const
{ return cDontConnect(); }

const Color* WellDisplayBaseMapObject::getColor(int) const
{
    static Color col;
    col = wd_->getColor();
    return &col;
}


const char* WellDisplay::sKeyEarthModelID      = "EarthModel ID";
const char* WellDisplay::sKeyWellID	       = "Well ID";

WellDisplay::WellDisplay()
    : VisualObjectImpl(true)
    , well_(0)
    , wd_(0)
    , wellid_(-1)
    , zistime_(SI().zIsTime())
    , zinfeet_(SI().zInFeet())
    , eventcatcher_(0)
    , changed_(this)
    , logsnumber_(0)
    , transformation_(0)
    , picksallowed_(false)
    , group_(0)
    , pseudotrack_(0)
    , needsave_(false)
    , dispprop_(0)
{
    setMaterial(0);
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    removeChild( well_->getInventorNode() );
    well_->unRef();
    setSceneEventCatcher(0);
    if ( transformation_ ) transformation_->unRef();
    if ( group_ )
	removeChild( group_->getInventorNode() );

    wd_ = 0;
    mGetWD(return);
    wd->tobedeleted.remove( mCB(this,WellDisplay,welldataDelNotify) );
    delete dispprop_;
    delete Well::MGR().release( wellid_ );

    setBaseMap( 0 );
}


void WellDisplay::welldataDelNotify( CallBacker* )
{
    saveDispProp( wd_ );
    wd_ = 0;
}


Well::Data* WellDisplay::getWD() const
{
    if ( !wd_ )
    {
	WellDisplay* self = const_cast<WellDisplay*>( this );
	Well::Data* wd = Well::MGR().get( wellid_, false );
	self->wd_ = wd;
	if ( wd )
	{
	    wd->tobedeleted.notify(
		    mCB(self,WellDisplay,welldataDelNotify) );
	    wd->trackchanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    wd->markerschanged.notify( mCB(self,WellDisplay,updateMarkers) );
	    wd->dispparschanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    if ( zistime_ )
		wd->d2tchanged.notify( mCB(self,WellDisplay,fullRedraw) );
	}
    }
    return wd_;
}


void WellDisplay::saveDispProp( const Well::Data* wd )
{
    if ( !wd ) return;
    dispprop_ = new Well::DisplayProperties( wd->displayProperties() );
}


void WellDisplay::restoreDispProp()
{
    if ( !dispprop_ )
	return;
    mGetWD();
    wd->displayProperties() = *dispprop_;
}


void WellDisplay::setWell( visBase::Well* well )
{
    if ( well_ )
    {
	removeChild( well_->getInventorNode() );
	well_->unRef();
    }
    well_ = well;
    well_->ref();
    addChild( well_->getInventorNode() );
}


void WellDisplay::fillTrackParams( visBase::Well::TrackParams& tp )
{
    mGetWD(return);
    tp.col_ 		= dpp( track_.color_ );
    tp.isdispabove_ 	= dpp( track_.dispabove_ );
    tp.isdispbelow_ 	= dpp( track_.dispbelow_ );
    tp.namesz_ 		= dpp( track_.nmsize_ );
    tp.size_ 		= dpp( track_.size_ );
}


void WellDisplay::fillMarkerParams( visBase::Well::MarkerParams& mp )
{
    mGetWD(return);
    mp.col_ 		= dpp( markers_.color_  );
    mp.shapeint_ 	= dpp( markers_.shapeint_ );
    mp.cylinderheight_ 	= dpp( markers_.cylinderheight_ );
    mp.issinglecol_ 	= dpp( markers_.issinglecol_ );
    mp.issamenmcol_ 	= dpp( markers_.samenmcol_ );
    mp.namesize_ 	= dpp( markers_.nmsize_ );
    mp.namecol_ 	= dpp( markers_.nmcol_ );
    mp.size_ 		= dpp( markers_.size_ );
}


#define dppl(lognr,par) lognr==1? dpp(left_.par) : dpp(right_.par)
void WellDisplay::fillLogParams( visBase::Well::LogParams& lp, int lognr )
{
    mGetWD(return);
    lp.cliprate_ 	= dppl( lognr, cliprate_ );
    lp.col_ 	 	= dppl( lognr, color_);
    lp.fillname_ 	= dppl( lognr, fillname_ );
    lp.fillrange_ 	= dppl( lognr, fillrange_ );
    lp.isdatarange_ 	= dppl( lognr, isdatarange_ );
    lp.isleftfilled_ 	= dppl( lognr, isleftfill_ );
    lp.isrightfilled_ 	= dppl( lognr, isrightfill_ );
    lp.issinglcol_	= dppl( lognr, issinglecol_);
    lp.iswelllog_	= dppl( lognr, iswelllog_ );
    lp.islogarithmic_ 	= dppl( lognr, islogarithmic_ );
    lp.logwidth_ 	= dppl( lognr, logwidth_ );
    lp.name_	 	= dppl( lognr, name_ );
    lp.ovlap_ 	 	= dppl( lognr, repeatovlap_ );
    lp.range_ 		= dppl( lognr, range_ );
    lp.repeat_ 	 	= dppl( lognr, repeat_);
    lp.seqname_	 	= dppl( lognr, seqname_ );
    lp.size_	 	= dppl( lognr, size_ );
    lp.seiscolor_	= dppl( lognr, seiscolor_ );
    lp.iscoltabflipped_	= dppl( lognr, iscoltabflipped_ );
}


#define mTryDispLog( side, Side )\
{ \
    BufferString& logname = dpp( side.name_ );\
    if ( wd->logs().indexOf( logname ) >= 0 )\
	display##Side##Log();\
}
void WellDisplay::fullRedraw( CallBacker* )
{
    mGetWD(return);
    if ( !well_ ) return;

    const bool waslogconstsize = well_->logConstantSize();

    TypeSet<Coord3> trackpos = getTrackPos( wd );
    if ( trackpos.isEmpty() ) return;
    visBase::Well::TrackParams tp;
    fillTrackParams( tp );
    tp.toppos_ = &trackpos[0]; tp.botpos_ = &trackpos[trackpos.size()-1];
    tp.name_ = wd->name();
    logsnumber_ = mMAX( dpp( right_.repeat_ ), dpp( left_.repeat_ ) );
    updateMarkers(0);

    well_->setTrack( trackpos );
    well_->setTrackProperties( tp.col_, tp.size_ );
    well_->setWellName( tp );
    well_->removeLogs();
    well_->setRepeat( logsnumber_ );
    well_->setLogConstantSize( waslogconstsize );

    mTryDispLog( left_, Left );
    mTryDispLog( right_, Right );
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool WellDisplay::setMultiID( const MultiID& multiid )
{
    Well::Data* oldwd = getWD();
    if ( oldwd )
	Well::MGR().release( wellid_ );

    wellid_ = multiid; wd_ = 0;
    mGetWD(return false);

    const Well::D2TModel* d2t = wd->d2TModel();
    if ( zistime_ )
    {
	if ( !d2t )
	    mErrRet( "No depth to time model defined" )
    }

    wellid_ = multiid;
    fullRedraw(0);

    if ( basemapobj_ )
	basemapobj_->updateGeometry();

    return true;
}


TypeSet<Coord3> WellDisplay::getTrackPos( const Well::Data* wd )
{
    TypeSet<Coord3> trackpos;
    const Well::D2TModel* d2t = wd->d2TModel();
    setName( wd->name() );

    if ( wd->track().size() < 1 ) return trackpos;
    PtrMan<Well::Track> ttrack = 0;
    if ( zistime_ )
    {
	mTryAlloc( ttrack, Well::Track( wd->track() ) );
	ttrack->toTime( *d2t );
    }
    const Well::Track& track = zistime_ ? *ttrack : wd->track();

    Coord3 pt;
    for ( int idx=0; idx<track.size(); idx++ )
    {
	pt = track.pos( idx );
	if ( zinfeet_ )
	    mMeter2Feet(pt.z);

	if ( !mIsUdf(pt.z) )
	    trackpos += pt;
    }

    return trackpos;
}


void WellDisplay::updateMarkers( CallBacker* )
{
    well_->removeAllMarkers();
    mGetWD(return);

    visBase::Well::MarkerParams mp;
    fillMarkerParams( mp );

    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 pos = wd->track().getPos( wellmarker->dah() );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( wellmarker->dah() );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)
	mp.pos_ = &pos;	mp.name_ = wellmarker->name();	

	if ( !mp.issinglecol_ ) mp.col_  = wellmarker->color();
	if ( mp.issamenmcol_ ) mp.namecol_  = mp.col_;

	well_->addMarker( mp );
    }
}


void WellDisplay::setMarkerScreenSize( int sz )
{ well_->setMarkerScreenSize( sz ); }


int WellDisplay::markerScreenSize() const
{ return well_->markerScreenSize(); }


#define mShowFunction( showObj, objShown ) \
void WellDisplay::showObj( bool yn ) \
{ \
    well_->showObj( yn ); \
} \
\
bool WellDisplay::objShown() const \
{ \
    return well_->objShown(); \
}

bool WellDisplay::canShowMarkers() const
{ return well_->canShowMarkers(); }

mShowFunction( showWellTopName, wellTopNameShown )
mShowFunction( showWellBotName, wellBotNameShown )
mShowFunction( showMarkers, markersShown )
mShowFunction( showMarkerName, markerNameShown )
mShowFunction( showLogName, logNameShown )


const LineStyle* WellDisplay::lineStyle() const
{
    return &well_->lineStyle();
}


void WellDisplay::setLineStyle( LineStyle lst )
{
    well_->setLineStyle( lst );
}


void WellDisplay::setLogData( visBase::Well::LogParams& lp, bool isfilled )
{
    mGetWD(return);

    const int logidx = isfilled ? lp.filllogidx_ : lp.logidx_;
    Well::Log& wl = wd->logs().getLog( logidx );
    if ( wl.isEmpty() ) return;
    const int logsz = wl.size();
    const Well::Track& track = wd->track();
    const Interval<float> range = lp.range_;
    float minval, maxval; minval = maxval =  mUdf( float );
    TypeSet<Coord3Value> crdvals;
    for ( int idx=0; idx<logsz; idx++ )
    {
	const float dah = wl.dah(idx);
	const float val = wl.value(idx);

	if ( mIsUdf( val ) || !range.includes( val ) )
	    continue;

	Coord3 pos = track.getPos( dah );
	if ( !pos.x && !pos.y && !pos.z ) 
	    continue;

	if ( mIsUdf( minval ) || val < minval )
	    minval = val;
	if ( mIsUdf( maxval ) || val > maxval )
	    maxval = val;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( dah );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)

	Coord3Value cv( pos, val );
	crdvals += cv;
    }
    if ( crdvals.isEmpty() )
	return;

    ( isfilled ? lp.valfillrange_ : lp.valrange_ ).set( minval, maxval );
    if ( isfilled )
	well_->setFilledLogData( crdvals, lp );
    else
	well_->setLogData( crdvals, lp );
}


void WellDisplay::setLogDisplay( int lognr )
{
    mGetWD(return);

    BufferString& logname = lognr == 1 ? dpp(left_.name_) : dpp(right_.name_);
    if ( wd->logs().isEmpty() ) return;
    const int logidx = wd->logs().indexOf( logname );
    if( logidx<0 )
    {
	well_->clearLog( lognr );
	well_->showLog( false, lognr );
	return;
    }

    visBase::Well::LogParams lp;
    fillLogParams( lp, lognr );
    lp.logidx_ = logidx;  lp.lognr_ = lognr;
    setLogProperties( lp );

    setLogData( lp, false );

    if ( lp.isleftfilled_ || lp.isrightfilled_ )
    {
	lp.filllogidx_ = wd->logs().indexOf( lp.fillname_ );
	if ( lp.filllogidx_ >= 0 ) setLogData( lp, true );
    }

    well_->showLog( true, lognr );
    if ( lp.repeat_<logsnumber_ )
	well_->hideUnwantedLogs( lognr, lp.repeat_ );
}


void WellDisplay::displayRightLog()
{ setLogDisplay( 2 ); }


void WellDisplay::displayLeftLog()
{ setLogDisplay( 1 ); }


void WellDisplay::setOneLogDisplayed(bool yn)
{ onelogdisplayed_ = yn; }


void WellDisplay::setLogConstantSize(bool yn)
{ well_->setLogConstantSize( yn ); }


bool WellDisplay::logConstantSize() const
{ return well_->logConstantSize(); }


void WellDisplay::setLogProperties( visBase::Well::LogParams& lp )
{
    const int lognr = lp.lognr_;

    well_->setOverlapp( lp.ovlap_, lognr );
    well_->setLogStyle( lp.iswelllog_, lognr );
    well_->setLogFill( ( lp.isleftfilled_ || lp.isrightfilled_ ), lognr );
    well_->setLogFillColorTab( lp, lognr );
    well_->setLogLineDisplayed( lp.size_ > 0, lognr );

    setLogColor( lp.col_, lognr );
    setLogLineWidth( lp.size_, lognr );
    setLogWidth( lp.logwidth_, lognr );

    if ( lp.cliprate_ && lp.logidx_ >= 0 )
	calcClippedRange( lp.cliprate_, lp.range_, lp.logidx_ );
}


void WellDisplay::calcClippedRange( float rate, Interval<float>& rg, int lidx )
{
    mGetWD(return);

    Well::Log& wl = wd->logs().getLog( lidx );
    if ( rate > 100 ) rate = 100;
    if ( mIsUdf(rate) || rate < 0 ) rate = 0;
    rate /= 100;
    const int logsz = wl.size();
    DataClipper dataclipper;
    dataclipper.setApproxNrValues( logsz );
    dataclipper.putData( wl.valArr(), logsz );
    dataclipper.calculateRange( rate, rg );
}


const Color& WellDisplay::logColor( int lognr ) const
{ return well_->logColor( lognr ); }


void WellDisplay::setLogColor( const Color& col, int lognr )
{ well_->setLogColor( col, lognr ); }


float WellDisplay::logLineWidth( int lognr ) const
{ return well_->logLineWidth( lognr ); }


void WellDisplay::setLogLineWidth( float width, int lognr )
{ well_->setLogLineWidth( width, lognr ); }


int WellDisplay::logWidth() const
{ return well_->logWidth(); }


void WellDisplay::setLogWidth( int width, int lognr )
{ well_->setLogWidth( width, lognr ); }


bool WellDisplay::logsShown() const
{ return well_->logsShown(); }


void WellDisplay::showLogs( bool yn )
{
    well_->showLog( yn, 1);
    well_->showLog( yn, 2);
}


Color WellDisplay::getColor() const
{
    return well_->lineStyle().color_;
}


void WellDisplay::getMousePosInfo( const visBase::EventInfo&,
				   Coord3& pos,
				   BufferString& val,
				   BufferString& info ) const
{
    val.setEmpty(); info.setEmpty();
    mGetWD(return);

    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    const float zstep2 = zfactor * SI().zStep()/2;
    const float mousez = pos.z*zfactor; 

    info = "Well: "; info += wd->name();
    info += ", MD ";

    const float dah = zistime_ ? wd->d2TModel()->getDah( mousez*0.001 ) 
			       : wd->track().getDahForTVD( mousez );

    const bool zdispinft = SI().depthsInFeetByDefault();
    info += zdispinft ? "(ft): " : "(m): ";
    const float zfac = SI().depthsInFeetByDefault() ? mToFeetFactor : 1;
    info += toString( mNINT(dah*zfac) );

    setLogInfo( info, val, dah, true );
    setLogInfo( info, val, dah, false );

    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	if ( !mIsEqual(wellmarker->dah(),dah,zstep2) )
	    continue;

	info += ", Marker: ";
	info += wellmarker->name();
	break;
    }
}


void WellDisplay::setLogInfo( BufferString& info, BufferString& val,
				float dah, bool isleft ) const
{
    mGetWD(return);
    const Well::DisplayProperties& disp = wd->displayProperties();
    const char* lognm = isleft ? disp.left_.name_ : disp.right_.name_;
    if ( lognm && strcmp(lognm,"None") && strcmp(lognm,"none") )
    {
	info += isleft ? ", Left: " : ", Right: ";
	info += lognm;
	const Well::Log* log = wd->logs().getLog( lognm );
	if (log)
	{
	    if ( val.size() ) val += " / ";
	    val += toString( log->getValue( dah ) );
	    val += " ";
	    val += log->unitMeasLabel();
	}
    }
}


void WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID, wellid_ );

    const int viswellid = well_->id();
    par.set( sKeyWellID, viswellid );
    if ( saveids.indexOf(viswellid) == -1 ) saveids += viswellid;

    mGetWD(return);
    wd->displayProperties().fillPar( par );

    fillSOPar( par, saveids );
}


int WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    int viswellid;
    if ( par.get(sKeyWellID,viswellid) )
    {
	DataObject* dataobj = visBase::DM().getObject( viswellid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::Well*,well,dataobj)
	if ( !well ) return -1;
	setWell( well );
    }
    else
    {
	setWell( visBase::Well::create() );
	viswellid = well_->id();
    }

    MultiID newmid;
    if ( !par.get(sKeyEarthModelID,newmid) )
	return -1;

    if ( !setMultiID(newmid) )
    {
	return 1;
    }

    mGetWD(return -1);
    wd->displayProperties().usePar( par );
    displayLeftLog();
    displayRightLog();

// Support for old sessions
    BufferString linestyle;
    if ( par.get(visBase::Well::linestylestr(),linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

    return useSOPar( par );
}


void WellDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    well_->setDisplayTransformation( nt );
    setDisplayTransformForPicks( nt );
    fullRedraw(0);
}


visBase::Transformation* WellDisplay::getDisplayTransformation()
{ return well_->getDisplayTransformation(); }


void WellDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !picksallowed_ || !group_ || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( dataobj->selectable() )
	{
	    eventid = eventinfo.pickedobjids[idx];
	    break;
	}
    }

    if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	mousepressposition_ = eventid==-1
	    ? Coord3::udf() : eventinfo.displaypickedpos;
	eventcatcher_->setHandled();
    }
    else
    {
	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{
	    const float zfactor = scene_ ? scene_->getZScale(): SI().zScale();
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		int removeidx = group_->getFirstIdx(mousepressid_);
		if ( removeidx != -1 )
		{
		    group_->removeObject( removeidx );
		    pseudotrack_->removePoint( removeidx );
		    TypeSet<Coord3> wcoords;
		    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
		    {
			wcoords += pseudotrack_->pos(idx);
			wcoords[idx].z /= zfactor;
		    }

		    well_->setTrack(wcoords);
		    needsave_ = true;
		    changed_.trigger();
		}
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		  !OD::altKeyboardButton(eventinfo.buttonstate_) &&
		  !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		const int sz = eventinfo.pickedobjids.size();
		bool validpicksurface = false;

		for ( int idx=0; idx<sz; idx++ )
		{
		    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
		    mDynamicCastGet(const SurveyObject*,so,pickedobj)
		    if ( so && so->allowsPicks() )
		    {
			validpicksurface = true;
			break;
		    }
		}

		if ( validpicksurface )
		{
		    Coord3 newpos = eventinfo.worldpickedpos;
		    mDynamicCastGet(SurveyObject*,so,
				    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addPick( newpos );
		}
	    }

	    eventcatcher_->setHandled();
	}
    }
}


void WellDisplay::addPick( Coord3 pos )
{
    int insertidx = -1;
    if ( pseudotrack_ )
    {
	TypeSet<Coord3> wcoords;
	const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
	insertidx = pseudotrack_->insertPoint( Coord(pos.x, pos.y),
					       pos.z * zfactor  );
	for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
	{
	    wcoords += pseudotrack_->pos(idx);
	    wcoords[idx].z /= zfactor;
	}

	well_->setTrack(wcoords);
	needsave_ = true;
	changed_.trigger();
    }

    if ( insertidx > -1 )
    {
	visBase::Marker* marker = visBase::Marker::create();
	group_->insertObject( insertidx, marker );

	marker->setDisplayTransformation( transformation_ );
	marker->setCenterPos( pos );
        marker->setScreenSize( mPickSz );
	marker->setType( (MarkerStyle3D::Type)mPickSz );
	marker->getMaterial()->setColor( lineStyle()->color_ );
    }
}


void WellDisplay::setDisplayTransformForPicks( visBase::Transformation* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    if ( !group_ ) return;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	marker->setDisplayTransformation( transformation_ );
    }
}


void WellDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,WellDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,WellDisplay,pickCB));
	eventcatcher_->ref();
    }
}


void WellDisplay::setupPicking( bool yn )
{
    picksallowed_ = yn;
    if ( !group_ )
    {
	group_ = visBase::DataObjectGroup::create();
	mTryAlloc( pseudotrack_, Well::Track() );
	addChild( group_->getInventorNode() );
    }

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	marker->turnOn( yn );
    }
}


void WellDisplay::showKnownPositions()
{
    mGetWD(return);

    TypeSet<Coord3> trackpos = getTrackPos( wd );
    if ( trackpos.isEmpty() )
	return;

    for ( int idx=0; idx<trackpos.size(); idx++ )
	addPick( trackpos[idx] );
}


TypeSet<Coord3> WellDisplay::getWellCoords() const
{
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();

    TypeSet<Coord3> coords;
    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
    {
	coords += pseudotrack_->pos(idx);
	coords[idx].z /= zfactor;
    }

    return coords;
}


BaseMapObject* WellDisplay::createBaseMapObject()
{ return new WellDisplayBaseMapObject( this ); }

}; // namespace visSurvey

