/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID = "$Id: survinfo.cc,v 1.158 2011-09-16 11:33:48 cvskris Exp $";

#include "survinfo.h"
#include "ascstream.h"
#include "filepath.h"
#include "cubesampling.h"
#include "latlong.h"
#include "undefval.h"
#include "safefileio.h"
#include "separstr.h"
#include "strmprov.h"
#include "oddirs.h"
#include "iopar.h"
#include "errh.h"
#include "zdomain.h"
#include <math.h>
#include <iostream>

#define KEYSTRS_IMPL 1
#include "keystrs.h"


static const char* sKeySI = "Survey Info";
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";
static const char* sKeyDefsFile = ".defs";
static const char* sKeySurvDefs = "Survey defaults";
static const char* sKeyLatLongAnchor = "Lat/Long anchor";
const char* SurveyInfo::sKeyInlRange()	    { return "In-line range"; }
const char* SurveyInfo::sKeyCrlRange()	    { return "Cross-line range"; }
const char* SurveyInfo::sKeyXRange()	    { return "X range"; }
const char* SurveyInfo::sKeyYRange()	    { return "Y range"; }
const char* SurveyInfo::sKeyZRange()	    { return "Z range"; }
const char* SurveyInfo::sKeyWSProjName()    { return "Workstation Project Name"; }
const char* SurveyInfo::sKeyDpthInFt()	    { return "Show depth in feet"; }
const char* SurveyInfo::sKeyXYInFt()	    { return "XY in feet"; }
const char* SurveyInfo::sKeySurvDataType()  { return "Survey Data Type"; }


SurveyInfo* SurveyInfo::theinst_ = 0;

DefineEnumNames(SurveyInfo,Pol2D,0,"Survey Type")
{ "Only 3D", "Both 2D and 3D", "Only 2D", 0 };

const SurveyInfo& SI()
{
    if ( !SurveyInfo::theinst_ || !SurveyInfo::theinst_->valid_ )
    {
	if ( SurveyInfo::theinst_ )
	{
	    SurveyInfo * myinst = SurveyInfo::theinst_;
	    SurveyInfo::theinst_ = 0;
	    delete myinst;
	}
	SurveyInfo::theinst_ = SurveyInfo::read( GetDataDir() );
    }
    return *SurveyInfo::theinst_;
}


void SurveyInfo::deleteInstance()
{
    delete theinst_;
    theinst_ = 0;
}


void SurveyInfo::setInvalid() const
{
    SurveyInfo* myself = const_cast<SurveyInfo*>(this);
    myself->valid_ = false;
}


SurveyInfo::SurveyInfo()
    : cs_(*new CubeSampling(false))
    , wcs_(*new CubeSampling(false))
    , valid_(false)
    , zistime_(true)
    , zinfeet_(false)
    , xyinfeet_(false)
    , pars_(*new IOPar(sKeySurvDefs))
    , ll2c_(*new LatLong2Coord)
    , workRangeChg(this)
    , survdatatype_(Both2DAnd3D)
    , survdatatypeknown_(false)
{
    rdxtr_.b = rdytr_.c = 1;
    set3binids_[2].crl = 0;

    // To get a 'reasonable' transform even when no proper SI is yet defined
    // Then, DataPointSets need to work
    RCol2Coord::RCTransform xtr, ytr;
    xtr.b = 1000; xtr.c = 0;
    ytr.b = 0; ytr.c = 1000;
    b2c_.setTransforms( xtr, ytr );
}


SurveyInfo::SurveyInfo( const SurveyInfo& si )
    : cs_(*new CubeSampling(false))
    , wcs_(*new CubeSampling(false))
    , pars_(*new IOPar(sKeySurvDefs))
    , ll2c_(*new LatLong2Coord)
    , workRangeChg(this)
{
    *this = si;
}


SurveyInfo::~SurveyInfo()
{
    delete &pars_;
    delete &ll2c_;
    delete &cs_;
    delete &wcs_;
}



SurveyInfo& SurveyInfo::operator =( const SurveyInfo& si )
{
    if ( &si == this ) return *this;

    setName( si.name() );
    valid_ = si.valid_;
    datadir_ = si.datadir_;
    dirname_ = si.dirname_;
    wsprojnm_ = si.wsprojnm_;
    wspwd_ = si.wspwd_;
    zistime_ = si.zistime_;
    zinfeet_ = si.zinfeet_;
    xyinfeet_ = si.xyinfeet_;
    b2c_ = si.b2c_;
    survdatatype_ = si.survdatatype_;
    survdatatypeknown_ = si.survdatatypeknown_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids_[idx] = si.set3binids_[idx];
	set3coords_[idx] = si.set3coords_[idx];
    }
    cs_ = si.cs_; wcs_ = si.wcs_; pars_ = si.pars_; ll2c_ = si.ll2c_;

    return *this;
}


SurveyInfo* SurveyInfo::read( const char* survdir )
{
    FilePath fpsurvdir( survdir );
    FilePath fp( fpsurvdir ); fp.add( ".survey" );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return new SurveyInfo;

    ascistream astream( sfio.istrm() );
    static bool errmsgdone = false;
    if ( !astream.isOfFileType(sKeySI) )
    {
	BufferString errmsg( "Survey definition file cannot be read.\n"
			     "Survey file '" );
	errmsg += fp.fullPath(); errmsg += "' has file type '";
	errmsg += astream.fileType();
	errmsg += "'.\nThe file may be corrupt or not accessible.";
	ErrMsg( errmsg );
	errmsgdone = true;
	sfio.closeFail();
	return new SurveyInfo;
    }
    errmsgdone = false;

    astream.next();
    BufferString keyw = astream.keyWord();
    SurveyInfo* si = new SurveyInfo;

    si->dirname_ = fpsurvdir.fileName();
    si->datadir_ = fpsurvdir.pathOnly();
    if ( !survdir || si->dirname_.isEmpty() ) return si;

    while ( !atEndOfSection(astream) )
    {
	keyw = astream.keyWord();
	if ( keyw == sKey::Name )
	    si->setName( astream.value() );
	else if ( keyw == sKeyWSProjName() )
	    si->wsprojnm_ = astream.value();
	else if ( keyw == sKeyInlRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.hrg.start.inl = toInt(fms[0]);
	    si->cs_.hrg.stop.inl = toInt(fms[1]);
	    si->cs_.hrg.step.inl = toInt(fms[2]);
	}
	else if ( keyw == sKeyCrlRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.hrg.start.crl = toInt(fms[0]);
	    si->cs_.hrg.stop.crl = toInt(fms[1]);
	    si->cs_.hrg.step.crl = toInt(fms[2]);
	}
	else if ( keyw == sKeyZRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->cs_.zrg.start = toFloat(fms[0]);
	    si->cs_.zrg.stop = toFloat(fms[1]);
	    si->cs_.zrg.step = toFloat(fms[2]);
	    if ( Values::isUdf(si->cs_.zrg.step)
	      || mIsZero(si->cs_.zrg.step,mDefEps) )
		si->cs_.zrg.step = 0.004;
	    if ( fms.size() > 3 )
	    {
		si->zistime_ = *fms[3] == 'T';
		si->zinfeet_ = *fms[3] == 'F';
	    }
	}
	else if ( keyw == sKeySurvDataType() )
	{
	    Pol2D var;
	    if ( !parseEnumPol2D( astream.value(), var ) )
		var = Both2DAnd3D;

	    si->setSurvDataType( var );
	    si->survdatatypeknown_ = true;
	}
	else if ( keyw == sKeyXYInFt() )
	    si->xyinfeet_ = astream.getYN();
	else
	    si->handleLineRead( keyw, astream.value() );

	astream.next();
    }
    si->cs_.normalise();
    si->wcs_ = si->cs_;

    char buf[1024];
    while ( astream.stream().getline(buf,1024) )
    {
	if ( !si->comment_.isEmpty() )
	    si->comment_ += "\n";
	si->comment_ += buf;
    }
    sfio.closeSuccess();

    if ( si->wrapUpRead() )
	si->valid_ = true;

    fp = fpsurvdir; fp.add( sKeyDefsFile );
    si->getPars().read( fp.fullPath(), sKeySurvDefs, true );
    si->getPars().setName( sKeySurvDefs );
    return si;
}


bool SurveyInfo::wrapUpRead()
{
    if ( set3binids_[2].crl == 0 )
	get3Pts( set3coords_, set3binids_, set3binids_[2].crl );
    b2c_.setTransforms( rdxtr_, rdytr_ );
    return b2c_.isValid();
}


void SurveyInfo::handleLineRead( const BufferString& keyw, const char* val )
{
    if ( keyw == sKeyXTransf )
	setTr( rdxtr_, val );
    else if ( keyw == sKeyYTransf )
	setTr( rdytr_, val );
    else if ( keyw == sKeyLatLongAnchor )
	ll2c_.use( val );
    else if ( matchString("Set Point",(const char*)keyw) )
    {
	const char* ptr = strchr( (const char*)keyw, '.' );
	if ( !ptr ) return;
	int ptidx = toInt( ptr + 1 ) - 1;
	if ( ptidx < 0 ) ptidx = 0;
	if ( ptidx > 3 ) ptidx = 2;
	FileMultiString fms( val );
	if ( fms.size() < 2 ) return;
	set3binids_[ptidx].use( fms[0] );
	set3coords_[ptidx].use( fms[1] );
    }
}


StepInterval<int> SurveyInfo::inlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hrg.get( ret, dum );
    return ret;
}


StepInterval<int> SurveyInfo::crlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hrg.get( dum, ret );
    return ret;
}


const StepInterval<float>& SurveyInfo::zRange( bool work ) const
{
    return sampling(work).zrg;
}

int SurveyInfo::maxNrTraces( bool work ) const
{
    return sampling(work).hrg.nrInl() * sampling(work).hrg.nrCrl();
}


int SurveyInfo::inlStep() const { return cs_.hrg.step.inl; }
int SurveyInfo::crlStep() const { return cs_.hrg.step.crl; }


float SurveyInfo::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return c00.distTo(c10);
}


float SurveyInfo::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return c00.distTo(c01);
}


float SurveyInfo::computeArea( const Interval<int>& inlrg,
			       const Interval<int>& crlrg ) const 
{
    const BinID step = sampling(false).hrg.step;
    const Coord c00 = transform( BinID(inlrg.start,crlrg.start) );
    const Coord c01 = transform( BinID(inlrg.start,crlrg.stop+step.crl) );
    const Coord c10 = transform( BinID(inlrg.stop+step.inl,crlrg.start) );

    const float scale = xyInFeet() ? mFromFeetFactor : 1; 
    const double d01 = c00.distTo( c01 ) * scale;
    const double d10 = c00.distTo( c10 ) * scale;

    return d01*d10;
}


float SurveyInfo::computeArea( bool work ) const 
{ return computeArea( inlRange( work ), crlRange( work ) ); }



float SurveyInfo::zStep() const { return cs_.zrg.step; }


Coord3 SurveyInfo::oneStepTranslation( const Coord3& planenormal ) const
{
    Coord3 translation( 0, 0, 0 ); 

    if ( fabs(planenormal.z) > 0.5 )
    {
	translation.z = SI().zStep();
    }
    else
    {
	Coord norm2d = planenormal;
	norm2d.normalize();

	if ( fabs(norm2d.dot(SI().binID2Coord().rowDir())) > 0.5 )
	   translation.x = inlDistance();
	else
	    translation.y = crlDistance();
    }

    return translation;
}


void SurveyInfo::setRange( const CubeSampling& cs, bool work )
{
    if ( work )
	wcs_ = cs;
    else
	cs_ = cs;

    wcs_.limitTo( cs_ );
    wcs_.hrg.step = cs_.hrg.step;
    wcs_.zrg.step = cs_.zrg.step;
}


void SurveyInfo::setWorkRange( const CubeSampling& cs )
{
    setRange( cs, true);
    workRangeChg.trigger();
}


Interval<int> SurveyInfo::reasonableRange( bool inl ) const
{
    const Interval<int> rg = inl
      ? Interval<int>( cs_.hrg.start.inl, cs_.hrg.stop.inl )
      : Interval<int>( cs_.hrg.start.crl, cs_.hrg.stop.crl );

    const int w = rg.stop - rg.start;

    return Interval<int>( rg.start - 3*w, rg.stop +3*w );
}


bool SurveyInfo::isReasonable( const BinID& b ) const
{
    return reasonableRange( true ).includes( b.inl,false ) &&
	   reasonableRange( false ).includes( b.crl,false );
}


bool SurveyInfo::isReasonable( const Coord& crd ) const
{
    if ( Values::isUdf(crd.x) || Values::isUdf(crd.y) )
	return false;

    return isReasonable( transform(crd) ); 
}


#define mChkCoord(c) \
    if ( c.x < minc.x ) minc.x = c.x; if ( c.y < minc.y ) minc.y = c.y;

Coord SurveyInfo::minCoord( bool work ) const
{
    const CubeSampling& cs = sampling(work);
    Coord minc = transform( cs.hrg.start );
    Coord c = transform( cs.hrg.stop );
    mChkCoord(c);
    BinID bid( cs.hrg.start.inl, cs.hrg.stop.crl );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
    c = transform( bid );
    mChkCoord(c);
    return minc;
}


#undef mChkCoord
#define mChkCoord(c) \
    if ( c.x > maxc.x ) maxc.x = c.x; if ( c.y > maxc.y ) maxc.y = c.y;

Coord SurveyInfo::maxCoord( bool work ) const
{
    const CubeSampling& cs = sampling(work);
    Coord maxc = transform( cs.hrg.start );
    Coord c = transform( cs.hrg.stop );
    mChkCoord(c);
    BinID bid( cs.hrg.start.inl, cs.hrg.stop.crl );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
    c = transform( bid );
    mChkCoord(c);
    return maxc;
}


void SurveyInfo::checkInlRange( Interval<int>& intv, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hrg.start.inl ) intv.start = cs.hrg.start.inl;
    if ( intv.start > cs.hrg.stop.inl )  intv.start = cs.hrg.stop.inl;
    if ( intv.stop > cs.hrg.stop.inl )   intv.stop = cs.hrg.stop.inl;
    if ( intv.stop < cs.hrg.start.inl )  intv.stop = cs.hrg.start.inl;
    BinID bid( intv.start, 0 );
    snap( bid, BinID(1,1) ); intv.start = bid.inl;
    bid.inl = intv.stop; snap( bid, BinID(-1,-1) ); intv.stop = bid.inl;
}

void SurveyInfo::checkCrlRange( Interval<int>& intv, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hrg.start.crl ) intv.start = cs.hrg.start.crl;
    if ( intv.start > cs.hrg.stop.crl )  intv.start = cs.hrg.stop.crl;
    if ( intv.stop > cs.hrg.stop.crl )   intv.stop = cs.hrg.stop.crl;
    if ( intv.stop < cs.hrg.start.crl )  intv.stop = cs.hrg.start.crl;
    BinID bid( 0, intv.start );
    snap( bid, BinID(1,1) ); intv.start = bid.crl;
    bid.crl = intv.stop; snap( bid, BinID(-1,-1) ); intv.stop = bid.crl;
}



void SurveyInfo::checkZRange( Interval<float>& intv, bool work ) const
{
    const StepInterval<float>& rg = sampling(work).zrg;
    intv.sort();
    if ( intv.start < rg.start ) intv.start = rg.start;
    if ( intv.start > rg.stop )  intv.start = rg.stop;
    if ( intv.stop > rg.stop )   intv.stop = rg.stop;
    if ( intv.stop < rg.start )  intv.stop = rg.start;
    snapZ( intv.start, 1 );
    snapZ( intv.stop, -1 );
}


bool SurveyInfo::includes( const BinID& bid, const float z, bool work ) const
{
    const CubeSampling& cs = sampling(work);
    const float eps = 1e-8;
    return cs.hrg.includes( bid )
	&& cs.zrg.start < z + eps && cs.zrg.stop > z - eps;
}


SurveyInfo::Unit SurveyInfo::xyUnit() const
{ return xyinfeet_ ? Feet : Meter; }


SurveyInfo::Unit SurveyInfo::zUnit() const
{
    if ( zistime_ ) return Second;
    return zinfeet_ ? Feet : Meter;
}


void SurveyInfo::putZDomain( IOPar& iop ) const
{
    iop.set( ZDomain::sKey(), zistime_ ? ZDomain::sKeyTime()
	    			       : ZDomain::sKeyDepth() );
}

static const char* SIDistUnitString( bool feet, bool wb )
{
    if ( feet )
	return wb ? "(ft)" : "ft";

    return wb ? "(m)" : "m";
}


const char* SurveyInfo::getXYUnitString( bool wb ) const
{
    return SIDistUnitString( xyinfeet_, wb );
}


const char* SurveyInfo::getZUnitString( bool wb ) const
{
    if ( zistime_ )
	return wb ? "(ms)" : "ms";

    return SIDistUnitString( depthsInFeetByDefault(), wb );
}


void SurveyInfo::setZUnit( bool istime, bool infeet )
{
    zistime_ = istime;
    zinfeet_ = istime ? false : infeet;
}

float SurveyInfo::defaultXYtoZScale( Unit zunit, Unit xyunit )
{
    if ( zunit==xyunit )
	return 1;

    if ( zunit==Second )
    {
	if ( xyunit==Meter )
	    return 1000;

	//xyunit==feet	
	return 3048;
    }
    else if ( zunit==Feet && xyunit==Meter )
	return mFromFeetFactor;

    //  zunit==Meter && xyunit==Feet
    return mToFeetFactor;
}


int SurveyInfo::zFactor() const
{ return zFactor ( zIsTime() ); }


int SurveyInfo::zFactor( bool time )
{ return time ? 1000 : 1; }


float SurveyInfo::zScale() const
{ return defaultXYtoZScale( zUnit(), xyUnit() ); }


bool SurveyInfo::depthsInFeetByDefault() const
{
    bool ret = zIsTime() ? xyInFeet() : zInFeet();
    if ( !ret && zIsTime() )
	pars().getYN( sKeyDpthInFt(), ret );
    return ret;
}


BinID SurveyInfo::transform( const Coord& c ) const
{
    if ( !valid_ ) return BinID(0,0);
    static StepInterval<int> inlrg, crlrg;
    cs_.hrg.get( inlrg, crlrg );
    return b2c_.transformBack( c, &inlrg, &crlrg );
}


void SurveyInfo::get3Pts( Coord c[3], BinID b[2], int& xline ) const
{
    if ( set3binids_[0].inl )
    {
	b[0] = set3binids_[0]; c[0] = set3coords_[0];
	b[1] = set3binids_[1]; c[1] = set3coords_[1];
	c[2] = set3coords_[2]; xline = set3binids_[2].crl;
    }
    else
    {
	b[0] = cs_.hrg.start; c[0] = transform( b[0] );
	b[1] = cs_.hrg.stop; c[1] = transform( b[1] );
	BinID b2 = cs_.hrg.stop; b2.inl = b[0].inl;
	c[2] = transform( b2 ); xline = b2.crl;
    }
}


const char* SurveyInfo::set3Pts( const Coord c[3], const BinID b[2],
				 int xline )
{
    if ( b[1].inl == b[0].inl )
        return "Need two different in-lines";
    if ( b[0].crl == xline )
        return "No Cross-line range present";

    if ( !b2c_.set3Pts( c[0], c[1], c[2], b[0], b[1], xline ) )
	return "Cannot construct a valid transformation matrix from this input"
	       "\nPlease check whether the data is on a single straight line.";

    set3binids_[0] = b[0];
    set3binids_[1] = b[1];
    set3binids_[2] = BinID( b[0].inl, xline );
    set3coords_[0] = c[0];
    set3coords_[1] = c[1];
    set3coords_[2] = c[2];
    return 0;
}


void SurveyInfo::gen3Pts()
{
    set3binids_[0] = cs_.hrg.start;
    set3binids_[1] = cs_.hrg.stop;
    set3binids_[2] = BinID( cs_.hrg.start.inl, cs_.hrg.stop.crl );
    set3coords_[0] = transform( set3binids_[0] );
    set3coords_[1] = transform( set3binids_[1] );
    set3coords_[2] = transform( set3binids_[2] );
}


static void doSnap( int& idx, int start, int step, int dir )
{
    if ( step < 2 ) return;
    int rel = idx - start;
    int rest = rel % step;
    if ( !rest ) return;
 
    idx -= rest;
 
    if ( !dir ) dir = rest > step / 2 ? 1 : -1;
    if ( rel > 0 && dir > 0 )      idx += step;
    else if ( rel < 0 && dir < 0 ) idx -= step;
}


void SurveyInfo::snap( BinID& binid, BinID rounding ) const
{
    const CubeSampling& cs = sampling( false );
    const BinID& stp = cs.hrg.step;
    if ( stp.inl == 1 && stp.crl == 1 ) return;
    doSnap( binid.inl, cs.hrg.start.inl, stp.inl, rounding.inl );
    doSnap( binid.crl, cs.hrg.start.crl, stp.crl, rounding.crl );
}


void SurveyInfo::snapStep( BinID& s, BinID rounding ) const
{
    const BinID& stp = cs_.hrg.step;
    if ( s.inl < 0 ) s.inl = -s.inl;
    if ( s.crl < 0 ) s.crl = -s.crl;
    if ( s.inl < stp.inl ) s.inl = stp.inl;
    if ( s.crl < stp.crl ) s.crl = stp.crl;
    if ( s == stp || (stp.inl == 1 && stp.crl == 1) )
	return;

    int rest;
#define mSnapStep(ic) \
    rest = s.ic % stp.ic; \
    if ( rest ) \
    { \
	int hstep = stp.ic / 2; \
	bool upw = rounding.ic > 0 || (rounding.ic == 0 && rest > hstep); \
	s.ic -= rest; \
	if ( upw ) s.ic += stp.ic; \
    }

    mSnapStep(inl)
    mSnapStep(crl)
}


void SurveyInfo::snapZ( float& z, int dir ) const
{
    const StepInterval<float>& zrg = cs_.zrg;
    const float eps = 1e-8;

    if ( z < zrg.start + eps )
	{ z = zrg.start; return; }
    if ( z > zrg.stop - eps )
	{ z = zrg.stop; return; }

    const float relidx = zrg.getfIndex( z );
    int targetidx = mNINT(relidx);
    const float zdiff = z - zrg.atIndex( targetidx );
    if ( !mIsZero(zdiff,eps) && dir )
	targetidx = (int)( dir < 0 ? floor(relidx) : ceil(relidx) );
    z = zrg.atIndex( targetidx );;
    if ( z > zrg.stop - eps )
	 z = zrg.stop;
}


void SurveyInfo::setTr( RCol2Coord::RCTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = toDouble(fms[0]); tr.b = toDouble(fms[1]); tr.c = toDouble(fms[2]);
}


void SurveyInfo::putTr( const RCol2Coord::RCTransform& tr,
			  ascostream& astream, const char* key ) const
{
    char buf[1024];
    sprintf( buf, "%.10lg`%.10lg`%.10lg", tr.a, tr.b, tr.c );
    astream.put( key, buf );
}


bool SurveyInfo::isClockWise() const
{
    float xinl = b2c_.getTransform(true).b;
    float xcrl = b2c_.getTransform(true).c;
    float yinl = b2c_.getTransform(false).b;
    float ycrl = b2c_.getTransform(false).c;

    float det = xinl*ycrl - xcrl*yinl;
    return det < 0;
}


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !valid_ ) return false;
    if ( !basedir ) basedir = GetBaseDataDir();

    FilePath fp( basedir ); fp.add( dirname_ ).add( ".survey" );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open survey info file for write!" );
	if ( *sfio.errMsg() )
	    { msg += "\n\t"; msg += sfio.errMsg(); }
	ErrMsg( msg );
	return false;
    }

    std::ostream& strm = sfio.ostrm();
    ascostream astream( strm );
    if ( !astream.putHeader(sKeySI) )
    {
	ErrMsg( "Cannot write to survey info file!" );
	return false;
    }

    astream.put( sKey::Name, name() );
    astream.put( sKeySurvDataType(), getPol2DString( survDataType()) );
    FileMultiString fms;
    fms += cs_.hrg.start.inl; fms += cs_.hrg.stop.inl; fms += cs_.hrg.step.inl;
    astream.put( sKeyInlRange(), fms );
    fms = "";
    fms += cs_.hrg.start.crl; fms += cs_.hrg.stop.crl; fms += cs_.hrg.step.crl;
    astream.put( sKeyCrlRange(), fms );
    fms = ""; fms += cs_.zrg.start; fms += cs_.zrg.stop;
    fms += cs_.zrg.step; fms += zistime_ ? "T" : ( zinfeet_ ? "F" : "D" );
    astream.put( sKeyZRange(), fms );

    if ( !wsprojnm_.isEmpty() )
	astream.put( sKeyWSProjName(), wsprojnm_ );

    writeSpecLines( astream );

    astream.newParagraph();
    const char* ptr = (const char*)comment_;
    if ( *ptr )
    {
	while ( 1 )
	{
	    char* nlptr = const_cast<char*>( strchr( ptr, '\n' ) );
	    if ( !nlptr )
	    {
		if ( *ptr )
		    strm << ptr;
		break;
	    }
	    *nlptr = '\0';
	    strm << ptr << '\n';
	    *nlptr = '\n';
	    ptr = nlptr + 1;
	}
	strm << '\n';
    }

    if ( strm.fail() )
    {
	sfio.closeFail();
	ErrMsg( "Error during write of survey info file!" );
	return false;
    }
    else if ( !sfio.closeSuccess() )
    {
	BufferString msg( "Error closing survey info file:\n" );
	msg += sfio.errMsg();
	ErrMsg( msg );
	return false;
    }

    fp.set( basedir ); fp.add( dirname_ );
    savePars( fp.fullPath() );
    return true;
}


void SurveyInfo::writeSpecLines( ascostream& astream ) const
{
    putTr( b2c_.getTransform(true), astream, sKeyXTransf );
    putTr( b2c_.getTransform(false), astream, sKeyYTransf );
    FileMultiString fms;
    for ( int idx=0; idx<3; idx++ )
    {
	SeparString ky( "Set Point", '.' );
	ky += idx + 1;
	char buf[80];
	set3binids_[idx].fill( buf ); fms = buf;
	set3coords_[idx].fill( buf ); fms += buf;
	astream.put( (const char*)ky, (const char*)fms );
    }

    if ( ll2c_.isOK() )
    {
	char buf[1024];
	ll2c_.fill( buf );
	astream.put( sKeyLatLongAnchor, buf );
    }
    astream.putYN( sKeyXYInFt(), xyinfeet_ );
}


#define uiErrMsg(s) \
{   FilePath msgfp( GetBinPlfDir() ); \
    msgfp.add( "od_DispMsg" ); \
    BufferString cmd = msgfp.fullPath(); \
    cmd += " --err "; \
    cmd += " Could not write to "; \
    cmd += s; \
    cmd += " Please check the file permission"; \
    ExecOSCmd( cmd.buf() ); } \

void SurveyInfo::savePars( const char* basedir ) const
{
    if ( pars_.isEmpty() ) return;

    if ( !basedir || !*basedir )
	basedir = GetDataDir();

    FilePath fp( basedir ); fp.add( sKeyDefsFile );
    if ( !pars_.write( fp.fullPath(), sKeySurvDefs ) )
	uiErrMsg( fp.fullPath() );
}


bool SurveyInfo::has2D() const
{ return survdatatype_ == Only2D || survdatatype_ == Both2DAnd3D; }


bool SurveyInfo::has3D() const
{ return survdatatype_ == No2D || survdatatype_ == Both2DAnd3D; }


float SurveyInfo::computeAngleXInl() const
{
    Coord xy1 = transform( BinID(inlRange(false).start, crlRange(false).start));
    Coord xy2 = transform( BinID(inlRange(false).stop, crlRange(false).start) );
    const double xdiff = xy2.x - xy1.x;
    const double ydiff = xy2.y - xy1.y;
    return atan2( ydiff, xdiff );
}


bool SurveyInfo::isInside( const BinID& bid, bool work ) const
{
    const Interval<int> inlrg( inlRange(work) );
    const Interval<int> crlrg( crlRange(work) );
    return inlrg.includes(bid.inl,false) && crlrg.includes(bid.crl,false);
}
