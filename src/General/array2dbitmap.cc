/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: array2dbitmap.cc,v 1.27 2008-06-05 15:09:12 cvshelene Exp $";

#include "array2dbitmapimpl.h"
#include "arraynd.h"
#include "sorting.h"
#include "interpol2d.h"
#include "statrand.h"
#include "envvars.h"
#include <math.h>

const char A2DBitMapGenPars::cNoFill		= -127;
const char WVAA2DBitMapGenPars::cZeroLineFill	= -126;
const char WVAA2DBitMapGenPars::cWiggFill	= -125;
const char WVAA2DBitMapGenPars::cLeftFill	= -124;
const char WVAA2DBitMapGenPars::cRightFill	= -123;
const char VDA2DBitMapGenPars::cMinFill		= -120;
const char VDA2DBitMapGenPars::cMaxFill		= 120;

#define cNrFillSteps 241
#define mMaxNrStatPts 5000
#define mXPMStartLn '"'
#define mXPMEndLn "\",\n"


Interval<float> A2DBitMapInpData::scale( const Interval<float>& clipratio,
					 float midval ) const
{
    Interval<float> res;
    if ( mIsUdf(midval) )
    {
	if ( mIsUdf(clipratio.stop) )
	    clipper_.getRange( clipratio.start, res );
	else
	    clipper_.getRange( clipratio.start, clipratio.stop, res );
    }
    else
	clipper_.getSymmetricRange( clipratio.start, midval, res );

    return res;
}


float A2DBitMapInpData::midVal() const
{
    const TypeSet<float>& statpts = clipper_.statPts();
    return statpts.size() ? statpts[statpts.size()/2] : mUdf(float);
}


void A2DBitMapInpData::collectData()
{
    clipper_.reset();
    clipper_.setApproxNrValues( data_.info().getTotalSz(), mMaxNrStatPts );
    clipper_.putData( data_ );
    clipper_.fullSort();
}


A2DBitMapPosSetup::A2DBitMapPosSetup( const Array2DInfo& i, float* p )
    	: szdim0_(i.getSize(0))
    	, szdim1_(i.getSize(1))
	, nrxpix_(0)
	, nrypix_(0)
	, dim0pos_(0)
{
    setDim0Positions( p );
    setDim1Positions( 0, szdim1_-1 );
}


A2DBitMapPosSetup::~A2DBitMapPosSetup()
{
    delete [] dim0pos_;
}


void A2DBitMapPosSetup::setDim0Positions( float* p )
{
    if ( szdim0_ < 1 ) return;

    delete [] dim0pos_; dim0pos_ = p;

    if ( !dim0pos_ )
    {
	dim0pos_ = new float [szdim0_];
	for ( int idx=0; idx<szdim0_; idx++ )
	    dim0pos_[idx] = idx;
    }

    Interval<float> posbounds( dim0pos_[0], dim0pos_[szdim0_-1] );
    dim0avgdist_ = 1;
    if ( dim0pos_[0] != dim0pos_[szdim0_-1] )
    {
	float totdist = 0;
	for ( int idx=1; idx<szdim0_; idx++ )
	    totdist += fabs(dim0pos_[idx] - dim0pos_[idx-1]);
	dim0avgdist_ = totdist / (szdim0_ - 1);
    }

    posbounds.sort();
    dim0rg_.start = posbounds.start - dim0avgdist_ * 0.5;
    dim0rg_.stop = posbounds.stop + dim0avgdist_ * 0.5;
}


void A2DBitMapPosSetup::setDim1Positions( float start, float stop )
{
    dim1pos_.start = start; dim1pos_.stop = stop;
    dim1pos_.sort();
    dim1avgdist_ = szdim1_ > 1 ? dim1pos_.width() / (szdim1_ - 1) : 1;
    dim1rg_.start = dim1pos_.start - dim1avgdist_ * 0.5;
    dim1rg_.stop = dim1pos_.stop + dim1avgdist_ * 0.5;
}


void A2DBitMapPosSetup::setBitMapSizes( int n0, int n1 ) const
{
    A2DBitMapPosSetup& self = *(const_cast<A2DBitMapPosSetup*>(this));
    // Don't like to declare all mutable as they shld be const everyhwere else
    self.nrxpix_ = n0; self.nrypix_ = n1;
    self.dim0rg_.sort(); self.dim1rg_.sort();
    self.pixperdim0_ = nrxpix_ / dim0rg_.width();
    self.pixperdim1_ = nrypix_ / dim1rg_.width();
}


int A2DBitMapPosSetup::getPix( int dim, float pos, bool rev ) const
{
    const float fpix = getPixOffs( dim, pos, rev );

    int pix = mNINT(fpix);
    if ( pix < 0 )
	pix = 0;
    else if ( pix >= (dim?nrypix_:nrxpix_) )
	pix = (dim?nrypix_:nrxpix_) - 1;

    return pix;
}


bool A2DBitMapPosSetup::isInside( int dim, float pos ) const
{
    const float fpix = getPixOffs( dim, pos, false );

    return fpix + 1e-6 > 0 && fpix - 1e-6 < (dim?nrypix_:nrxpix_) - 1;
}


A2DBitMapGenerator::A2DBitMapGenerator( const A2DBitMapInpData& dat,
					const A2DBitMapPosSetup& setp,
					A2DBitMapGenPars& gp )
	: data_(dat)
    	, setup_(setp)
    	, pars_(gp)
    	, bitmap_(0)
{
}


void A2DBitMapGenerator::initBitMap( A2DBitMap& bm )
{
    const od_uint64 totsz = bm.info().getTotalSz();
    if ( totsz > 0 )
	memset( bm.getData(), A2DBitMapGenPars::cNoFill, totsz*sizeof(char) );
}


int A2DBitMapGenerator::bitmapSize( int dim ) const
{
    return bitmap_ ? bitmap_->info().getSize( dim ? 1 : 0 ) : 0;
}


void A2DBitMapGenerator::setBitMap( A2DBitMap& bm )
{
    bitmap_ = &bm;
    setup_.setBitMapSizes( bitmapSize(0), bitmapSize(1) );
}


void A2DBitMapGenerator::fill()
{
    if ( !bitmap_ ) return;
    setBitMap( *bitmap_ );
    if ( !setup_.nrXPix() || !setup_.nrYPix() )
	return;

    szdim0_ = setup_.dimSize( 0 );
    szdim1_ = setup_.dimSize( 1 );
    dim0pos_ = setup_.dim0Positions();
    dim1pos_ = setup_.dim1Positions();
    dim0rg_ = setup_.dimRange( 0 );
    dim1rg_ = setup_.dimRange( 1 );
    dim0perpix_ = 1. / setup_.getPixPerDim(0);
    dim1perpix_ = 1. / setup_.getPixPerDim(1);

    scalerg_ = pars_.autoscale_ ? data_.scale( pars_.clipratio_, pars_.midvalue_ )
				: pars_.scale_;
    scalewidth_ = scalerg_.stop - scalerg_.start;
    if ( mIsZero(scalewidth_,1e-8) )
    {
	scalewidth_ = 1e-8;
	scalerg_.start -= scalewidth_ / 2;
	scalerg_.stop += scalewidth_ / 2;
    }

    doFill();
}


static inline int gtPrettyBMVal( char c )
{
    static const float rgmax = 1000;
    float v = (c - VDA2DBitMapGenPars::cMinFill) * (rgmax + 1)
	    / (VDA2DBitMapGenPars::cMaxFill-VDA2DBitMapGenPars::cMinFill) - .5;
    int ret = mNINT(v);
    return ret < 0 ? 0 : (ret > rgmax+.5 ? (int)rgmax : ret);
}

bool A2DBitMapGenerator::dump( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    if ( !bitmap_ || nrxpix == 0 || nrypix == 0 )
	return false;

    if ( !GetEnvVarYN("OD_DUMP_A2DBITMAP_AS_NUMBERS" ) && dumpXPM(strm) )
	return true;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << gtPrettyBMVal( bitmap_->get(0,iy) );
	for ( int ix=1; ix<nrxpix; ix++ )
	    strm << '\t' << gtPrettyBMVal( bitmap_->get(ix,iy) );
	strm << std::endl;
    }
    return true;
}

//---


WVAA2DBitMapGenerator::WVAA2DBitMapGenerator( const A2DBitMapInpData& d,
					      const A2DBitMapPosSetup& su )
    	: A2DBitMapGenerator(d,su,*new WVAA2DBitMapGenPars)
{
}


int WVAA2DBitMapGenerator::dim0SubSampling() const
{
    const float nrpixperdim0 = setup_.nrXPix() / ((float)szdim0_);
    float fret = gtPars().minpixperdim0_ / nrpixperdim0;
    int ret = mNINT( fret );
    return ret < 2 ? 1 : ret;
}


void WVAA2DBitMapGenerator::doFill()
{
    const int dispeach = dim0SubSampling();
    stripwidth_ = (1 + wvapars().overlap_) * dispeach * setup_.avgDist(0);

    for ( int idim0=0; idim0<szdim0_; idim0+=dispeach )
    {
	if ( setup_.isInside(0,dim0pos_[idim0]) )
	    drawTrace( idim0 );
    }
}


void WVAA2DBitMapGenerator::drawTrace( int idim0 )
{
    int previdim1 = mUdf(int);
    float prevval = mUdf(float);
    Interpolate::PolyReg1DWithUdf<float> pr1d;
    const Array2D<float>& inpdata = data_.data();

    float midval = wvapars().midvalue_;
    if ( mIsUdf(midval) ) midval = data_.midVal();
    const float midratio = (midval - scalerg_.start) / scalewidth_;
    const float offs = (midratio-0.5) * stripwidth_;
    const float middim0pos = dim0pos_[idim0] + (pars_.fliplr_ ? -offs : offs);
    const float dim1fac = (szdim1_ - 1) / dim1pos_.width();

    for ( int iy=0; iy<setup_.nrYPix(); iy++ )
    {
	const float dim1pos = pars_.fliptb_ ? dim1rg_.stop - iy * dim1perpix_
					    : dim1rg_.start + iy * dim1perpix_;
	if ( !setup_.isInside(1,dim1pos) )
	    continue;

	const float fdim1 = (dim1pos - dim1pos_.start) * dim1fac;
	const int idim1 = (int)floor( fdim1 + 1e-6 );
	const float dim1offs = fdim1 - idim1;

	const float v0 = inpdata.get( idim0, idim1 );
	const float v1 = idim1 < szdim1_-1
		       ? inpdata.get( idim0, idim1+1 ) : v0;
	if ( !pars_.nointerpol_ && idim1 != previdim1 )
	{
	    pr1d.set( idim1 > 0 ? inpdata.get( idim0, idim1-1 ) : v0,
		      v0,
		      v1,
		      idim1 < szdim1_-2 ? inpdata.get( idim0, idim1+2 ) : v1 );
	}

	float val = pars_.nointerpol_ ? (dim1offs < 0.5 ? v0 : v1)
	    			      : pr1d.apply( dim1offs );
	drawVal( idim0, iy, val, prevval, midval, middim0pos );

	prevval = val;
	previdim1 = idim1;
    }
}

#define mApplyValClipping(val) \
    if ( val < scalerg_.start ) val = scalerg_.start; \
    else if ( val > scalerg_.stop ) val = scalerg_.stop

#define mPrepVal() \
    if ( mIsUdf(val) ) return; \
    mApplyValClipping(val)


void WVAA2DBitMapGenerator::drawVal( int idim0, int iy, float val,
				     float prevval,
				     float midval, float middim0pos )
{
    mPrepVal();

    const float centerdim0pos = dim0pos_[idim0];
    const float valratio = (val - scalerg_.start) / scalewidth_;
    const float valdim0offs = (valratio-0.5) * stripwidth_;
    const float valdim0pos = centerdim0pos
			   + (pars_.fliplr_ ? -valdim0offs : valdim0offs);

    const bool isleft = val < midval;
    const int midpix = setup_.getPix( 0, middim0pos, pars_.fliplr_ );
    const int valpix = setup_.getPix( 0, valdim0pos, pars_.fliplr_ );

    if ( isleft && wvapars().fillleft_ )
    {
	for ( int ix=valpix; ix<=midpix; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitMapGenPars::cLeftFill );
    }

    if ( !isleft && wvapars().fillright_ )
    {
	for ( int ix=midpix; ix<=valpix; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitMapGenPars::cRightFill );
    }

    if ( wvapars().drawmid_ && midpix >= 0 && midpix < setup_.nrXPix() )
	bitmap_->set( midpix, iy, WVAA2DBitMapGenPars::cZeroLineFill );

    if ( wvapars().drawwiggles_ && setup_.isInside(0,valdim0pos) )
    {
	if ( mIsUdf(prevval) ) prevval = val;
	mApplyValClipping( prevval );

	const float prevvalratio = (prevval - scalerg_.start) / scalewidth_;
	const float prevvaldim0offs = (prevvalratio-0.5) * stripwidth_;
	const float prevvaldim0pos = centerdim0pos
		       + (pars_.fliplr_ ? -prevvaldim0offs : prevvaldim0offs);
	const int prevvalpix = setup_.getPix( 0, prevvaldim0pos, pars_.fliplr_);

	int from, to;
	if ( prevvalpix < valpix )
	{
	    from = prevvalpix; to = valpix;
	    if ( from != to ) from++;
	}
	else
	{
	    from = valpix; to = prevvalpix;
	    if ( from != to ) to--;
	}

	for ( int ix=from; ix<=to; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitMapGenPars::cWiggFill );
    }
}


bool WVAA2DBitMapGenerator::dumpXPM( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();

    strm << "/* XPM */\nstatic char*wva[]={\n";
    strm << '"' << nrxpix << ' ' << nrypix << ' ' << "5 1"
		<< mXPMEndLn;
    strm << "\"l c #0000ff" << mXPMEndLn;
    strm << "\"r c #ff0000" << mXPMEndLn;
    strm << "\"0 c #00ff00" << mXPMEndLn;
    strm << "\"w c #000000" << mXPMEndLn;
    strm << "\"e c #ffffff" << mXPMEndLn;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << mXPMStartLn;
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    char c = bitmap_->get( ix, iy );
	    switch ( c )
	    {
	    case WVAA2DBitMapGenPars::cWiggFill:	strm << 'w';	break;
	    case WVAA2DBitMapGenPars::cZeroLineFill:	strm << '0';	break;
	    case WVAA2DBitMapGenPars::cLeftFill:	strm << 'l';	break;
	    case WVAA2DBitMapGenPars::cRightFill:	strm << 'r';	break;
	    default:					strm << 'e';	break;
	    }
	}
	strm << mXPMEndLn;
    }

    return true;
}


//---



float VDA2DBitMapGenPars::offset( char c )
{
    return (c - cMinFill) / ((float)(cNrFillSteps - 1));
}


VDA2DBitMapGenerator::VDA2DBitMapGenerator( const A2DBitMapInpData& d,
					    const A2DBitMapPosSetup& su )
    	: A2DBitMapGenerator(d,su,*new VDA2DBitMapGenPars)
{
}


void VDA2DBitMapGenerator::doFill()
{
    const float avgdim0dist = setup_.avgDist( 0 );
    strippixs_ = avgdim0dist * setup_.getPixPerDim(0);

    stripstodraw_.erase();

    for ( int idim0=0; idim0<szdim0_; idim0++ )
    {
	float pos2chk = dim0pos_[idim0];
	if ( pos2chk < dim0rg_.start ) pos2chk += avgdim0dist*.5;
	if ( pos2chk > dim0rg_.stop ) pos2chk -= avgdim0dist*.5;

	if ( setup_.isInside(0,pos2chk) )
	    stripstodraw_ += idim0;
    }

    execute();
}


bool VDA2DBitMapGenerator::doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop; idx++ )
    {
	drawStrip( stripstodraw_[idx] );
    }

    return true;
}


int VDA2DBitMapGenerator::totalNr() const
{ return stripstodraw_.size(); }



void VDA2DBitMapGenerator::drawStrip( int idim0 )
{
    const float curpos = dim0pos_[idim0];
    float stripmidpix = setup_.getPixOffs( 0, curpos, pars_.fliplr_ );
    float halfstrippixs = strippixs_ / 2;
    Interval<int> pixs2do( (int)floor(stripmidpix-halfstrippixs+1e-6),
	    		   (int)ceil( stripmidpix+halfstrippixs-1e-6) );
    if ( pixs2do.start < 0 ) pixs2do.start = 0;
    if ( pixs2do.stop >= setup_.nrXPix() ) pixs2do.stop = setup_.nrXPix() - 1;

    // The problem is: some of the 'to do' pixels may in fact be nearer a
    // neighbouring position. That doesn't only cost performance, it
    // will also introduce a directional overprinting.
    // Therefore, the pixs2do we have now are the maximum pixs2do

    if ( idim0 > 0 )
    {
	const float prevpos = dim0pos_[idim0-1];
	float prevstripmidpix = setup_.getPixOffs( 0, prevpos, pars_.fliplr_ );
	float halfwaypix = (prevstripmidpix + stripmidpix) * .5;
	if ( !pars_.fliplr_ && halfwaypix > pixs2do.start )
	    pixs2do.start = (int)ceil(halfwaypix-1e-6);
	if ( pars_.fliplr_ && halfwaypix < pixs2do.stop )
	    pixs2do.stop = (int)floor(halfwaypix+1e-6);
    }
    if ( idim0 < szdim0_-1 )
    {
	const float nextpos = dim0pos_[idim0+1];
	float nextstripmidpix = setup_.getPixOffs( 0, nextpos, pars_.fliplr_ );
	float halfwaypix = (nextstripmidpix + stripmidpix) * .5;
	if ( !pars_.fliplr_ && halfwaypix < pixs2do.stop )
	    pixs2do.stop = (int)floor(halfwaypix-1e-6);
	if ( pars_.fliplr_ && halfwaypix > pixs2do.start )
	    pixs2do.start = (int)ceil(halfwaypix+1e-6);
    }

    drawPixLines( idim0, pixs2do );
}


#define mV00Val \
    inpdata.get( idim0, idim1 )
#define mV10Val \
    idim0 < szdim0_-1 ? inpdata.get( idim0+1, idim1 ) : v[0]
#define mV01Val \
    idim1 < szdim1_-1 ? inpdata.get( idim0, idim1+1 ) : v[0]
#define mV11Val \
    idim0 < szdim0_-1 ? (idim1 < szdim1_-1 \
		      ? inpdata.get( idim0+1, idim1+1 ) : v[2]) : v[1]

void VDA2DBitMapGenerator::drawPixLines( int stripdim0,
					 const Interval<int>& xpixs2do )
{
    PtrMan< Interpolate::Applier2D<float> > interp = 0;
    if ( vdpars().lininterp_ )
	interp = new Interpolate::LinearReg2DWithUdf<float>;
    else
    {
	const float pixperval0 = setup_.nrXPix() / ((float)szdim0_);
	const float pixperval1 = setup_.nrYPix() / ((float)szdim1_);
	const float xstretch = pixperval0 / pixperval1;
	interp = new Interpolate::PolyReg2DWithUdf<float>( xstretch );
    }

    const Array2D<float>& inpdata = data_.data();
    const float dim1fac = (szdim1_ - 1) / dim1pos_.width();
    int previdim1 = mUdf(int);

    for ( int ix=xpixs2do.start; ix<=xpixs2do.stop; ix++ )
    {
	const float dim0pos = pars_.fliplr_ ? dim0rg_.stop - ix * dim0perpix_
					    : dim0rg_.start + ix * dim0perpix_;
	int idim0 = stripdim0;
	if ( dim0pos_[stripdim0] > dim0pos && stripdim0 > 0 )
	    idim0--;
	const float v0dim0pos = dim0pos_[idim0];
	const float v1dim0pos = idim0<szdim0_-1 ? dim0pos_[idim0+1] : dim0pos+1;
	const float dim0offs = (dim0pos-v0dim0pos) / (v1dim0pos-v0dim0pos);

	for ( int iy=0; iy<setup_.nrYPix(); iy++ )
	{
	    const float dim1pos = pars_.fliptb_ ? dim1rg_.stop - iy*dim1perpix_
					: dim1rg_.start + iy*dim1perpix_;
	    if ( !setup_.isInside(1,dim1pos) )
		continue;

	    const float fdim1 = (dim1pos - dim1pos_.start) * dim1fac;
	    const int idim1 = (int)floor( fdim1 + 1e-6 );
	    const float dim1offs = fdim1 - idim1;

	    if ( !pars_.nointerpol_ && idim1 != previdim1 )
		fillInterpPars( *interp, idim0, idim1 );

	    float val;
	    if ( !pars_.nointerpol_ )
		val = interp->apply( dim0offs, dim1offs );
	    else
	    {
		float v[4]; val = v[0] = mV00Val;
		if ( dim0offs > 0.5 || dim1offs > 0.5 )
		{
		    v[1] = mV01Val; v[2] = mV10Val;
		    if ( dim0offs > 0.5 && dim1offs > 0.5 )
			val = mV11Val;
		    else
			val = dim0offs > 0.5 ? v[2] : v[1];
		}
	    }
	    drawVal( ix, iy, val );

	    previdim1 = idim1;
	}
    }
}


void VDA2DBitMapGenerator::fillInterpPars(
	Interpolate::Applier2D<float>& interp, int idim0, int idim1 )
{
    float v[12];
    const Array2D<float>& inpdata = data_.data();
    const ValueSeries<float>* storage = inpdata.getStorage();
    const float* storageptr = storage ? storage->arr() : 0;

    if ( storageptr )
    {
	const Array2DInfo& info = inpdata.info();
	storageptr += info.getOffset( idim0, idim1 );
	int offset11 = 0;
	int offset01 = 0;
	int offset10 = 0;
	if ( idim1 < szdim1_-1 ) { offset01++; offset11++; }
	if ( idim0 < szdim0_-1 ) { offset10 += szdim1_; offset11 += szdim1_; }

	v[0] = *storageptr;
	v[1] = storageptr[offset01];
	v[2] = storageptr[offset10];
	v[3] = storageptr[offset11];

	if ( !vdpars().lininterp_ )
	{
	    v[4] = idim0 > 0 ? storageptr[-szdim1_] : v[0];
	    v[5] = idim0 > 0 ? (idim1 < szdim1_-1
			     ? storageptr[-szdim1_+1] : v[4]) : v[1];
	    v[6] = idim1 > 0 ? storageptr[-1] : v[0];
	    v[7]  = idim1 < szdim1_-2 ? storageptr[2] : v[1];
	    v[8] = idim0 < szdim0_-1 ? (idim1 > 0
			     ? storageptr[szdim1_-1] : v[4]) : v[2];
	    v[9]  = idim0 < szdim0_-1 ? (idim1 < szdim1_-2
			     ? storageptr[szdim1_+2] : v[3] ) : v[7];
	    v[10]  = idim0 < szdim0_-2
			     ? storageptr[szdim1_*2] : v[2];
	    v[11]  = idim0 < szdim0_-2 ? (idim1 < szdim1_-1 
			     ? storageptr[szdim1_*2+1] : v[2]) : v[10];
	}
    }
    else
    {
	v[0] = mV00Val;
	v[1] = mV01Val;
	v[2] = mV10Val;
	v[3] = mV11Val;

	if ( !vdpars().lininterp_ )
	{
	    v[4] = idim0 > 0
			     ? inpdata.get(idim0-1,idim1) : v[0];
	    v[5] = idim0 > 0 ? (idim1 < szdim1_-1
			     ? inpdata.get(idim0-1,idim1+1) : v[4]) : v[1];
	    v[6] = idim1 > 0
			     ? inpdata.get(idim0,idim1-1) : v[0];
	    v[7]  = idim1 < szdim1_-2
			     ? inpdata.get(idim0,idim1+2) : v[1];
	    v[8] = idim0 < szdim0_-1 ? (idim1 > 0
			     ? inpdata.get(idim0+1,idim1-1) : v[4]) : v[2];
	    v[9]  = idim0 < szdim0_-1 ? (idim1 < szdim1_-2
			     ? inpdata.get(idim0+1,idim1+2) : v[3]) : v[7];
	    v[10]  = idim0 < szdim0_-2
			     ? inpdata.get(idim0+2,idim1) : v[2];
	    v[11]  = idim0 < szdim0_-2 ? (idim1 < szdim1_-1
			     ? inpdata.get(idim0+2,idim1+1) : v[2]) : v[10];
	}
    }

    interp.set( v );
}


void VDA2DBitMapGenerator::drawVal( int ix, int iy, float val )
{
    mPrepVal();

    const float valratio = (val - scalerg_.start) / scalewidth_;
    const char bmval = (char)(VDA2DBitMapGenPars::cMinFill
	    		      + valratio * cNrFillSteps - .5);
    bitmap_->set( ix, iy, bmval );
}


static void getColValHex( int idx, char* ptr )
{
    int major = idx / 15;
    int minor = idx % 16;
    ptr[0] = major < 10 ? '0' + major : 'a' + major - 10;
    ptr[1] = minor < 10 ? '0' + minor : 'a' + minor - 10;
}


bool VDA2DBitMapGenerator::dumpXPM( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    const float fac = ((float)51) / (cNrFillSteps - 1);
    char prevc = -1; int nrcols = 0;
    for ( int idx=0; idx<cNrFillSteps; idx++ )
    {
	char c = (char)(fac * idx + .5);
	if ( c != prevc ) nrcols++;
	prevc = c;
    }

    strm << "/* XPM */\nstatic char*vd[]={\n";
    strm << '"' << nrxpix << ' ' << nrypix << ' ' << nrcols+1 << " 1"
		<< mXPMEndLn;
    strm << '"' << ". c #00ff00" << mXPMEndLn;

    char buf[7];
    for ( int idx=0; idx<cNrFillSteps; idx++ )
    {
	char c = (char)(fac * idx + .5);
	if ( c == prevc ) continue;
	prevc = c;

	if ( c < 26 )	c += 'a';
	else		c += 'A' - 26;

	getColValHex( idx, buf );
	getColValHex( idx, buf+2 );
	getColValHex( idx, buf+4 );
	buf[6] = '\0';

	strm << '"' << c << " c #" << buf << mXPMEndLn;
    }

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << mXPMStartLn;
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    int c = ((int)bitmap_->get(ix,iy)) - VDA2DBitMapGenPars::cMinFill;
	    if ( c < 0 || c >= cNrFillSteps )
		strm << '.';
	    else
	    {
		char out = (char)(fac * c + .5);
		if ( out < 26 )	out += 'a';
		else		out += 'A' - 26;
		strm << out;
	    }
	}
	strm << mXPMEndLn;
    }

    return true;
}
