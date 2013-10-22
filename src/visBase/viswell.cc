
/*+
________________________________________________________________________

 (C) dGB B eheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra 
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viswell.h"

#include "visdrawstyle.h"
#include "viscoord.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"

#include "coltabsequence.h"
#include "cubesampling.h"
#include "iopar.h"
#include "indexedshape.h"
#include "ranges.h"
#include "scaler.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "coordvalue.h"

#include <osg/Switch>
#include <osg/Node>
#include <osgGeo/PlaneWellLog>
#include <osgText/Text>


mCreateFactoryEntry( visBase::Well );

namespace visBase
{

static const int cMaxNrLogSamples = 2000;

const char* Well::linestylestr()	{ return "Line style"; }
const char* Well::showwelltopnmstr()	{ return "Show top name"; }
const char* Well::showwellbotnmstr()	{ return "Show bottom name"; }
const char* Well::showmarkerstr()	{ return "Show markers"; }
const char* Well::showmarknmstr()	{ return "Show markername"; }
const char* Well::markerszstr()		{ return "Marker size"; }
const char* Well::showlogsstr()		{ return "Show logs"; }
const char* Well::showlognmstr()	{ return "Show logname"; }
const char* Well::logwidthstr()		{ return "Screen width"; }

Well::Well()
    : VisualObjectImpl( false )
    , showmarkers_(true)
    , showlogs_(true)
    , transformation_(0)
    , zaxistransform_(0)
    , voiidx_(-1)
    , leftlog_( new osgGeo::PlaneWellLog )
    , rightlog_( new osgGeo::PlaneWellLog )
{
    
    markerset_ = MarkerSet::create();
    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMaterial( new Material );

    track_ = PolyLine::create();
    track_->setColorBindType( VertexShape::BIND_OVERALL );
    track_->ref();  

    track_->addPrimitiveSet( Geometry::RangePrimitiveSet::create() );
    addChild( track_->osgNode() );

    track_->setMaterial( new Material );
    welltoptxt_ =  Text2::create();
    wellbottxt_ =  Text2::create();
    welltoptxt_->ref();
    wellbottxt_->ref();
    welltoptxt_->setMaterial( track_->getMaterial() );
    wellbottxt_->setMaterial( track_->getMaterial() );
    addChild( welltoptxt_->osgNode() );
    addChild( wellbottxt_->osgNode() );

    markernames_ = Text2::create();
    markernames_->ref();
    addChild( markernames_->osgNode() );

    constantlogsizefac_ = constantLogSizeFactor();

    leftlog_->ref();
    leftlog_->setLogConstantSizeFactor(constantlogsizefac_);
    addChild( leftlog_ );

    rightlog_->ref();
    rightlog_->setLogConstantSizeFactor(constantlogsizefac_);
    addChild( rightlog_ );
}


Well::~Well()
{
    if ( transformation_ ) transformation_->unRef();

    removeChild( track_->osgNode() );
    track_->unRef();

    markerset_->unRef();

    removeLogs();
    leftlog_->unref();
    rightlog_->unref();
}


void Well::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_==zat ) 
	return;
    
    if ( zaxistransform_ )
    {
	zaxistransform_->unRef();
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
    }
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    CubeSampling cs( false );
    for ( int idx=0; idx<pts.size(); idx++ )
	cs.include( SI().transform(pts[idx]), (float) pts[idx].z );

    if ( zaxistransform_ && zaxistransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_ < 0 )
	    voiidx_ = zaxistransform_->addVolumeOfInterest( cs, true );
	else
	    zaxistransform_->setVolumeOfInterest( voiidx_, cs, true );
	zaxistransform_->loadDataIfMissing( voiidx_ );
    }

    int ptidx = 0;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	if ( !pts[idx].isDefined() )
	    continue;

	track_->getCoordinates()->setPos( ptidx, pts[idx] );
	ptidx++;
    }
    
    RefMan<Geometry::RangePrimitiveSet> rps = 0;
    if ( !track_->nrPrimitiveSets() )
    {
	rps = Geometry::RangePrimitiveSet::create();
	track_->addPrimitiveSet( rps );
	
    }
    else
    {
	rps = (Geometry::RangePrimitiveSet*) track_->getPrimitiveSet( 0 );
    }
    
    rps->setRange( Interval<int>( 0, ptidx-1 ) );
}


void Well::setTrackProperties( Color& col, int width)
{
    LineStyle lst;
    lst.color_ = col;
    lst.width_ = width;
    setLineStyle( lst );
}


void Well::setLineStyle( const LineStyle& lst )
{
    track_->setLineStyle( lst );
}


const LineStyle& Well::lineStyle() const
{
    return track_->lineStyle();
}


void Well::setText( Text* tx, const char* chr, Coord3* pos,
		    const FontData& fnt )
{
    tx->setText( chr );
    tx->setFontData( fnt ); 
    if ( !SI().zRange(true).includes(pos->z, false) ) 
	pos->z = SI().zRange(true).limitValue( pos->z ); 
    tx->setPosition( *pos );
    tx->setJustification( Text::Left );
}


void Well::setWellName( const TrackParams& tp )
{
    BufferString label( tp.name_, " track" );
    track_->setName( label );

    if ( welltoptxt_->nrTexts()<1 )
	welltoptxt_->addText();

    if ( wellbottxt_->nrTexts()<1 )
	wellbottxt_->addText();
   
    setText(welltoptxt_->text(0),tp.isdispabove_ ? tp.name_ : "",tp.toppos_, 
	    tp.font_);
    setText(wellbottxt_->text(0),tp.isdispbelow_ ? tp.name_ : "",tp.botpos_, 
	    tp.font_);

}


void Well::showWellTopName( bool yn )
{
    welltoptxt_->turnOn( yn );
}


void Well::showWellBotName( bool yn )
{
    wellbottxt_->turnOn( yn );
}


bool Well::wellTopNameShown() const
{
    return welltoptxt_->isOn();
}


bool Well::wellBotNameShown() const
{
   return wellbottxt_->isOn();
}


void Well::setMarkerSetParams( const MarkerParams& mp )
{
   MarkerStyle3D markerstyle;
   markerset_->setMarkerHeightRatio( 1.0f );

    switch ( mp.shapeint_ )
    {
    case 0:
	markerstyle = MarkerStyle3D::Cylinder;
	markerset_->setMarkerHeightRatio( ( float )mp.cylinderheight_/mp.size_ );
    	break;
    case 1:
	markerstyle = MarkerStyle3D::Cube;
	break;
    case 2:
	markerstyle = MarkerStyle3D::Sphere;
	break;
    case 3:
	markerstyle = MarkerStyle3D::Cone;
	break;
    default:
	pErrMsg( "Shape not implemented" );
	return;
    }

    markerstyle.size_ = mp.size_;
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMinimumScale( 0 );
    markerset_->setMaximumScale( 25.5f );
    
    markerset_->setAutoRotateMode( visBase::MarkerSet::NO_ROTATION );
}


void Well::addMarker( const MarkerParams& mp )
{
    Coord3 markerpos = *mp.pos_;
    if ( zaxistransform_ )
	  markerpos.z = zaxistransform_->transform( markerpos );
    if ( mIsUdf(markerpos.z) )
	  return;

    const int markerid = markerset_->getCoordinates()->addPos( markerpos );
    markerset_->getMaterial()->setColor( mp.col_,markerid ) ;

    const int textidx = markernames_->addText();
    Text* txt = markernames_->text( textidx );
    txt->setColor( mp.namecol_ );
    setText(txt,mp.name_,mp.pos_,mp.font_);

    return;
}


void Well::removeAllMarkers() 
{
    markerset_->clearMarkers();
    markernames_->removeAll();
}


void Well::setMarkerScreenSize( int size )
{
    markersize_ = size;
    markerset_->setScreenSize( size );
}


int Well::markerScreenSize() const
{
    return markerset_->getScreenSize();
}


bool Well::canShowMarkers() const
{
    return markerset_->getCoordinates()->size(); 
}


void Well::showMarkers( bool yn )
{
   markerset_->turnOn( yn );
}


bool Well::markersShown() const
{
    return markerset_->isOn();
}

void Well::showMarkerName( bool yn )
{ 
    markernames_->turnOn(yn);
}


bool Well::markerNameShown() const
{ 
    return markernames_->isOn();
}

#define mGetLoopSize(nrsamp,step)\
{\
    if ( nrsamp > cMaxNrLogSamples )\
    {\
	step = (float)nrsamp/cMaxNrLogSamples;\
	nrsamp = cMaxNrLogSamples;\
    }\
}


#define mSclogval(val)\
{\
    val += 1;\
    val = ::log( val );\
}


#define FULLFILLVALUE 100

void Well::setLogData(const TypeSet<Coord3Value>& crdvals,
		    const TypeSet<Coord3Value>& crdvalsF, 
		    const LogParams& lp, bool isFilled )
{
    float rgStartF(.0);
    float rgStopF(.0);
    LinScaler scalerF;
    LinScaler scaler;

    float rgStart(.0);
    float rgStop(.0);
    
    getLinScale(lp,scaler,false);
    Interval<float> selrg = lp.range_;
    getLinScaleRange( scaler,selrg,rgStart,rgStop,lp.islogarithmic_ );

    if(isFilled)
    {
	getLinScale(lp,scalerF);
	Interval<float> selrgF = lp.fillrange_;
	getLinScaleRange( scalerF,selrgF,rgStartF,rgStopF,lp.islogarithmic_ );
    }

    const bool rev = lp.range_.start > lp.range_.stop;
    const bool isfullfilled = lp.isleftfilled_ && lp.isrightfilled_ && 
			      lp.iswelllog_;
    const bool fillrev = !isfullfilled &&  
	(  ( lp.side_ == Left  && lp.isleftfilled_  && !rev )
	|| ( lp.side_ == Left  && lp.isrightfilled_ &&  rev )
	|| ( lp.side_ == Right && lp.isrightfilled_ && !rev )
	|| ( lp.side_ == Right && lp.isleftfilled_  &&  rev ) );

    osgGeo::PlaneWellLog* logdisplay = (lp.side_==Left) ? leftlog_ : rightlog_;
    logdisplay->setRevScale( rev );
    logdisplay->setFillRevScale( fillrev );
    logdisplay->setFullFilled( isfullfilled );

    const int nrsamp  = crdvals.size();
    const int nrsampF = crdvalsF.size();
    int longsmp = nrsamp >= nrsampF ? nrsamp : nrsampF;

    float step = 1;
    mGetLoopSize( longsmp, step );

    for ( int idx=0; idx<longsmp; idx++ )
    {
	const int index = mNINT32(idx*step);
	if( idx < nrsamp )
	{
	    const float val = isfullfilled ? FULLFILLVALUE : 
		getValue( crdvals, index, lp.islogarithmic_, scaler );
	    const Coord3& pos = getPos( crdvals, index );

	    if ( mIsUdf( pos.z ) || mIsUdf( val ) )
		continue;  

	    osg::Vec3Array* logPath = logdisplay->getPath();
	    osg::FloatArray* shapeLog = logdisplay->getShapeLog();

	    if(!logPath || !shapeLog)
		continue;

	    logPath->push_back( 
		osg::Vec3d((float) pos.x,(float) pos.y,(float) pos.z) );
	    shapeLog->push_back(val);

	}

	if ( isFilled && nrsampF != 0 && index < nrsampF )
	{
	    const float val = getValue( crdvalsF, 
		  index, lp.islogarithmic_, scalerF );
	    const Coord3& pos = getPos( crdvalsF, index );

	    if ( mIsUdf( pos.z ) || mIsUdf( val ) )
		continue;   
	    osg::FloatArray* fillLog = logdisplay->getFillLogValues();
	    if( !fillLog ) 
	        continue;
	    fillLog->push_back( val );
	    osg::FloatArray* fillLogDepths = logdisplay->getFillLogDepths();
	    fillLogDepths->push_back(pos.z);
	  }

    }

    logdisplay->setMaxFillValue( rgStopF );
    logdisplay->setMinFillValue( rgStartF );
    logdisplay->setMaxShapeValue( rgStop );
    logdisplay->setMinShapeValue( rgStart );
  
    showLog( showlogs_, lp.side_ );

}


void Well::getLinScaleRange( const LinScaler& scaler,Interval<float>& selrg, 
    float& rgstart, float& rgstop, bool islogarithmic_ )
{
    selrg.sort();
    rgstop = scaler.scale( selrg.stop );
    rgstart = scaler.scale( selrg.start );
    if ( islogarithmic_ )
    {
	mSclogval( rgstop ); 
	mSclogval( rgstart ); 
    }
}


void Well::getLinScale( const LogParams& lp,LinScaler& scaler, bool isFill )
{
    Interval<float> rg;
    if( isFill )
	rg = lp.valfillrange_;
    else
	rg = lp.valrange_;
    rg.sort();
    float minval = rg.start;
    float maxval = rg.stop;
    scaler.set( minval, 0, maxval, 100 );
}


Coord3 Well::getPos( const TypeSet<Coord3Value>& crdv, int idx ) const 
{
    const Coord3Value& cv = crdv[idx];
    Coord3 crd = cv.coord;
    if ( zaxistransform_ )
	crd.z = zaxistransform_->transform( crd );
    if ( mIsUdf(crd.z) )
	return crd;

    Coord3 pos;
    if ( transformation_ )
	transformation_->transform( crd, pos );
    return pos;
}


float Well::getValue( const TypeSet<Coord3Value>& crdvals, int idx, 
		      bool sclog, const LinScaler& scaler ) const
{
    const Coord3Value& cv = crdvals[idx];
    float val = (float) scaler.scale( cv.value );
    if ( val < 0 || mIsUdf(val) ) val = 0;
    if ( val > 100 ) val = 100;
    if ( sclog ) mSclogval(val);

    return val;
}


void Well::clearLog( Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    logdisplay->clearLog();
}


void Well::removeLogs()
{
   leftlog_->clearLog();
   rightlog_->clearLog();
}


void Well::setRepeat( int rpt, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    logdisplay->setRepeatNumber( rpt );
}


float Well::constantLogSizeFactor() const
{
   const int inlnr = SI().inlRange( true ).nrSteps();
   const int crlnr = SI().crlRange( true ).nrSteps();
   const float survfac = Math::Sqrt( (float)(crlnr*crlnr + inlnr*inlnr) );
   return survfac * 43; //hack 43 best factor based on F3_Demo
}


void Well::setOverlapp( float ovlap, Side side )
{
    ovlap = 100 - ovlap;
    if ( ovlap < 0.0 || mIsUdf(ovlap)  ) ovlap = 0.0;
    if ( ovlap > 100.0 ) ovlap = 100.0;

    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;

    logdisplay->setRepeatGap( ovlap );
}


void Well::setLogFill( bool fill, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;

    logdisplay->setLogFill( fill );
}


void Well::setLogStyle( bool style, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
        logdisplay->setSeisLogStyle( (1-style) );
}


void Well::setLogColor( const Color& col, Side side )
{
#define col2f(rgb) float(col.rgb())/255

    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
	osg::Vec4 osgCol = Conv::to<osg::Vec4>(col);
	logdisplay->setLineColor( osgCol );

}


const Color& Well::logColor( Side side  ) const
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    static Color color;
    const osg::Vec4d& col = logdisplay->getLineColor();
    const int r = mNINT32(col[0]*255);
    const int g = mNINT32(col[1]*255);
    const int b = mNINT32(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );
    return color;
}


#define scolors2f(rgb) float(lp.seiscolor_.rgb())/255
#define colors2f(rgb) float(col.rgb())/255
void Well::setLogFillColorTab( const LogParams& lp,  Side side  )
{
    int seqidx = ColTab::SM().indexOf( lp.seqname_ );
    if ( seqidx<0 || mIsUdf(seqidx) ) seqidx = 0;
    const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

    osg::ref_ptr<osg::Vec4Array> clrTable = new osg::Vec4Array;
    for ( int idx=0; idx<256; idx++ )
    {
	const bool issinglecol = ( !lp.iswelllog_ || 
	    		(lp.iswelllog_ && lp.issinglcol_ ) );
	float colstep = lp.iscoltabflipped_ ? 1-(float)idx/255 : (float)idx/255;
	Color col = seq->color( colstep );
	float r = issinglecol ? scolors2f(r) : colors2f(r);
	float g = issinglecol ? scolors2f(g) : colors2f(g);
	float b = issinglecol ? scolors2f(b) : colors2f(b);
	clrTable->push_back(osg::Vec4(r,g,b,1.0));
    }

    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
	logdisplay->setFillLogColorTab( clrTable );

}


void Well::setLogLineDisplayed( bool isdisp, Side side )
{
    osgGeo::PlaneWellLog::DisplaySide dispside = side==Left
	? osgGeo::PlaneWellLog::Left
	: osgGeo::PlaneWellLog::Right;

    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;

    logdisplay->setDisplaySide( dispside );
}


bool Well::logLineDisplayed( Side side ) const 
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    return logdisplay->getDisplayStatus();
}


void Well::setLogLineWidth( float width, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    logdisplay->setLineWidth( width );
}


float Well::logLineWidth( Side side ) const
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    return logdisplay->getLineWidth();
}


void Well::setLogWidth( int width, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    logdisplay ->setScreenWidth( width );
}


int Well::logWidth() const
{
    return mNINT32( leftlog_->getScreenWidth() );
}


void Well::showLogs( bool yn )
{
    if( yn == logsShown() )
	return;

    if( yn )
    {
	addChild( leftlog_ );
	addChild( rightlog_ );
    }
    else
    {
	removeChild( leftlog_ );
	removeChild( rightlog_ );
    }
}


void Well::showLog( bool yn, Side side )
{
    osgGeo::PlaneWellLog* logdisplay = side==Left 
	? leftlog_
	: rightlog_;
    logdisplay->setShowLog( yn );
}


bool Well::logsShown() const
{
   return ( childIndex( leftlog_ )!= -1 || childIndex( rightlog_ ) != -1 ) ?
       true : false ;
}


void Well::setLogConstantSize( bool yn )
{
    leftlog_->setLogConstantSize( yn );
    rightlog_->setLogConstantSize( yn );
}


bool Well::logConstantSize() const
{
   return  leftlog_->getDisplayStatus() ? leftlog_->getLogConstantSize() : true;
}

void Well::showLogName( bool yn )
{} 


bool Well::logNameShown() const
{ return false; }


void Well::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    track_->setDisplayTransformation( transformation_ );

    wellbottxt_->setDisplayTransformation( transformation_ );
    welltoptxt_->setDisplayTransformation( transformation_ );
    markernames_->setDisplayTransformation( transformation_ );
    markerset_->setDisplayTransformation( transformation_ );
}


const mVisTrans* Well::getDisplayTransformation() const
{ return transformation_; }


void Well::fillPar( IOPar& par ) const
{
    BufferString linestyle;
    lineStyle().toString( linestyle );
    par.set( linestylestr(), linestyle );

    par.setYN( showwelltopnmstr(), welltoptxt_->isOn() );
    par.setYN( showwellbotnmstr(), wellbottxt_->isOn() );
    par.setYN( showmarkerstr(), markersShown() );
    par.setYN( showmarknmstr(), markerNameShown() );
    par.setYN( showlogsstr(), logsShown() );
    par.setYN( showlognmstr(), logNameShown() );
    par.set( markerszstr(), markersize_ );
    par.set( logwidthstr(), logWidth() );
}


int Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get(linestylestr(),linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

#define mParGetYN(str,func) \
    doshow = true; \
    par.getYN(str,doshow); \
    func( doshow );

    bool doshow;
    mParGetYN(showwelltopnmstr(),showWellTopName);
    mParGetYN(showwellbotnmstr(),showWellBotName);
    mParGetYN(showmarkerstr(),showMarkers);	showmarkers_ = doshow;
    mParGetYN(showmarknmstr(),showMarkerName);  showlogs_ = doshow;
    mParGetYN(showlogsstr(),showLogs);
    mParGetYN(showlognmstr(),showLogName);

    par.get( markerszstr(), markersize_ );
    setMarkerScreenSize( markersize_ );

    return 1;
}

}; // namespace visBase
