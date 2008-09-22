/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2007
 RCS:           $Id: emhor2dto3d.cc,v 1.8 2008-09-22 13:07:32 cvskris Exp $
________________________________________________________________________

-*/

#include "emhor2dto3d.h"

#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emrowcoliterator.h"
#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "cubesampling.h"
#include "survinfo.h"

namespace EM
{

Hor2DTo3D::Setup::Setup( bool b )
    : dogrid_(b)
    , hs_(SI().sampling(true).hrg)
    , nrsteps_(0)
    , srchrad_(10*SI().inlDistance())
{
}


struct Hor2DTo3DSectionData
{

Hor2DTo3DSectionData( EM::SectionID sid,
		      const BinID& minbid, const BinID& maxbid,
		      const BinID& step )
    : arr_( getSz(minbid.inl,maxbid.inl,step.inl),
	    getSz(minbid.crl,maxbid.crl,step.crl) )
    , count_( getSz(minbid.inl,maxbid.inl,step.inl),
	      getSz(minbid.crl,maxbid.crl,step.crl) )
    , sid_(sid)
{
    inlsz_ = count_.info().getSize( 0 );
    crlsz_ = count_.info().getSize( 1 );
    if ( !arr_.getData() ) { inlsz_ = crlsz_ = 0; }

    for ( int iinl=0; iinl<inlsz_; iinl++ )
    {
	for ( int icrl=0; icrl<crlsz_; icrl++ )
	{
	    count_.set( iinl, icrl, 0 );
	    arr_.set( iinl, icrl, mUdf(float) );
	}
    }

    hs_.start = minbid;
    hs_.step = step;
    hs_.stop.inl = hs_.start.inl + hs_.step.inl * (inlsz_ - 1);
    hs_.stop.crl = hs_.start.crl + hs_.step.crl * (crlsz_ - 1);
}


int getSz( int start, int stop, int step ) const
{
    int diff = stop - start;
    int ret = diff / step + 1;
    if ( diff % step ) ret += 1;
    return ret;
}


void add( const BinID& bid, float z )
{
    float inldist = (bid.inl - hs_.start.inl) / ((float)hs_.step.inl);
    float crldist = (bid.crl - hs_.start.crl) / ((float)hs_.step.crl);
    const int inlidx = mNINT(inldist); const int crlidx = mNINT(crldist);
    if ( inlidx < 0 || inlidx >= inlsz_ || crlidx < 0 || crlidx >= crlsz_ )
	return;

    float curz = arr_.get( inlidx, crlidx );
    if ( mIsUdf(curz) )
	arr_.set( inlidx, crlidx, z );
    else
    {
	int curcount = count_.get( inlidx, crlidx );
	z = (curcount*curz + z) / (curcount + 1);
	arr_.set( inlidx, crlidx, z );
	count_.set( inlidx, crlidx, curcount+1 );
    }
}

    Array2DImpl<int>	count_;
    Array2DImpl<float>	arr_;
    HorSampling		hs_;
    EM::SectionID	sid_;
    int			inlsz_;
    int			crlsz_;
};


Hor2DTo3D::Hor2DTo3D( const Horizon2D& h2d, const Setup& setup, Horizon3D& h3d )
    : Executor( "Converting 2D horizon to 3D" )
    , hor2d_(h2d)
    , hor3d_(h3d)
    , cursectnr_(0)
    , curinterp_(0)
    , setup_(setup)
{
    addSections( setup_.hs_ );
    fillSections();

    if ( sd_.isEmpty() )
	msg_ = "No data in selected area";
    else
    {
	curinterp_ = new Array2DInterpolator<float>( sd_[cursectnr_]->arr_ );
	curinterp_->setDist( true, SI().crlDistance() );
	curinterp_->setDist( false, SI().inlDistance() );
	curinterp_->pars().useextension_ = !setup_.dogrid_;
	curinterp_->pars().extrapolate_ = true;
	curinterp_->pars().maxholesize_ = -1;
	curinterp_->pars().srchrad_ = setup_.srchrad_;
	if ( mIsUdf(setup_.nrsteps_) || setup_.nrsteps_ < 1 )
	    setup_.nrsteps_ = -1;
	curinterp_->pars().maxnrsteps_ = setup_.nrsteps_;
	msg_ = curinterp_->message();
    }

    hor3d_.removeAll();
}


void Hor2DTo3D::addSections( const HorSampling& hs )
{
    for ( int isect=0; isect<hor2d_.nrSections(); isect++ )
    {
	const EM::SectionID sid( hor2d_.sectionID(isect) );

	BinID minbid( mUdf(int), 0 ), maxbid;
	for ( EM::RowColIterator iter(hor2d_,sid); ; )
	{
	    const EM::PosID posid = iter.next();
	    if ( posid.objectID() == -1 )
		break;

	    const Coord coord = hor2d_.getPos( posid );
	    const BinID bid = SI().transform( coord );
	    if ( mIsUdf(minbid.inl) )
		minbid = maxbid = bid;
	    else
	    {
		if ( minbid.inl > bid.inl ) minbid.inl = bid.inl;
		if ( minbid.crl > bid.crl ) minbid.crl = bid.crl;
		if ( maxbid.inl < bid.inl ) maxbid.inl = bid.inl;
		if ( maxbid.crl < bid.crl ) maxbid.crl = bid.crl;
	    }
	}
	if ( mIsUdf(minbid.inl) || minbid == maxbid )
	    continue;

	sd_ += new Hor2DTo3DSectionData( sid, minbid, maxbid, hs.step );
    }
}


void Hor2DTo3D::fillSections()
{
    for ( int isd=0; isd<sd_.size(); isd++ )
    {
	Hor2DTo3DSectionData& sd = *sd_[isd];
	for ( EM::RowColIterator iter(hor2d_,sd.sid_); ; )
	{
	    const EM::PosID posid = iter.next();
	    if ( posid.objectID() == -1 )
		break;

	    const Coord3 coord = hor2d_.getPos( posid );
	    const BinID bid = SI().transform( coord );

	    sd.add( bid, coord.z );
	}
    }
}


Hor2DTo3D::~Hor2DTo3D()
{
    delete curinterp_;
    deepErase( sd_ );
}


const char* Hor2DTo3D::nrDoneText() const
{
    return curinterp_ ? curinterp_->nrDoneText() : "";
}


od_int64 Hor2DTo3D::nrDone() const
{
    return curinterp_ ? curinterp_->nrDone() : -1;
}


int Hor2DTo3D::nextStep()
{
    if ( sd_.isEmpty() )
	{ msg_ = "No data in selected area"; return Executor::ErrorOccurred; }
    else if ( !curinterp_ )
	return Executor::Finished;

    int ret = curinterp_->doStep();
    msg_ = curinterp_->message();
    if ( ret != Executor::Finished )
	return ret;

    const Hor2DTo3DSectionData& sd = *sd_[cursectnr_];

    const bool geowaschecked = hor3d_.enableGeometryChecks( false );

    SectionID sid = hor3d_.geometry().addSection( "", false );
    hor3d_.geometry().sectionGeometry(sid)->
				expandWithUdf( sd.hs_.start, sd.hs_.stop ); 

    for ( int inlidx=0; inlidx<sd.inlsz_; inlidx++ )
    {
	for ( int crlidx=0; crlidx<sd.crlsz_; crlidx++ )
	{
	    const BinID bid( sd.hs_.inlRange().atIndex(inlidx), 
			     sd.hs_.crlRange().atIndex(crlidx) );
	    const Coord3 pos( SI().transform(bid), sd.arr_.get(inlidx,crlidx) );

	    if ( pos.isDefined() ) 
		hor3d_.setPos( sid, bid.getSerialized(), pos, false );
	}
    }

    hor3d_.geometry().sectionGeometry(sid)->trimUndefParts(); 
    hor3d_.enableGeometryChecks( geowaschecked );
    
    cursectnr_++;
    if ( cursectnr_ >= sd_.size() )
	return Executor::Finished;
    else
    {
	Array2DInterpolator<float>* newinterp =
		    new Array2DInterpolator<float>( sd_[cursectnr_]->arr_ );
	newinterp->pars() = curinterp_->pars();
	delete curinterp_; curinterp_ = newinterp;
    }

    return Executor::MoreToDo;
}

}
