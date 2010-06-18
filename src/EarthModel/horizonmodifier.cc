/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizonmodifier.cc,v 1.8 2010-06-18 12:23:27 cvskris Exp $";


#include "horizonmodifier.h"

#include "cubesampling.h"
#include "emhorizon2d.h"
#include "emmanager.h"


HorizonModifier::HorizonModifier( bool is2d )
    : tophor_(0)
    , bothor_(0)
    , topisstatic_(true)
    , is2d_(is2d)
    , linenames_(0)
    , iter_(0)
{
}


HorizonModifier::~HorizonModifier()
{
    if ( tophor_ ) tophor_->unRef();
    if ( bothor_ ) bothor_->unRef();
}


bool HorizonModifier::setHorizons( const MultiID& mid1, const MultiID& mid2 )
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid1 );
    mDynamicCastGet(EM::Horizon*,tophor,EM::EMM().getObject(objid))
    tophor_ = tophor;

    objid = EM::EMM().getObjectID( mid2 );
    mDynamicCastGet(EM::Horizon*,bothor,EM::EMM().getObject(objid))
    bothor_ = bothor;

    if ( tophor_ && bothor_ )
    {
	tophor_->ref();
	bothor_->ref();
    }

    return tophor_ && bothor_;
}


void HorizonModifier::setMode( ModifyMode mode )
{
    modifymode_ = mode;
}


void HorizonModifier::setStaticHorizon( bool top )
{
    topisstatic_ = top;
}


bool HorizonModifier::getNextNode( BinID& bid )
{
    return is2d_ ? getNextNode2D(bid) : getNextNode3D(bid);
}


bool HorizonModifier::getNextNode3D( BinID& bid )
{   
    if ( !iter_ )
    {
	HorSampling hrg;
	StepInterval<int> rrg = tophor_->geometry().rowRange();
	StepInterval<int> crg = tophor_->geometry().colRange();
	hrg.set( rrg, crg );

	rrg = bothor_->geometry().rowRange();
	crg = bothor_->geometry().colRange();
	hrg.include( BinID(rrg.start,crg.start) );
	hrg.include( BinID(rrg.stop,crg.stop) );

	iter_ = new HorSamplingIterator( hrg );
    }

    return iter_ ? iter_->next(bid) : false;
}


bool HorizonModifier::getNextNode2D( BinID& bid )
{
    if ( !linenames_ )
    {
	linenames_ = new BufferStringSet;
	getLines( tophor_ );
	getLines( bothor_ );
    }

    if ( !linenames_ || !linenames_->size() ) return false;

    if ( bid.crl < trcrgs_[bid.inl].start )
	bid.crl = trcrgs_[bid.inl].start;
    else
	bid.crl += trcrgs_[bid.inl].step;

    if ( bid.crl > trcrgs_[bid.inl].stop )
    {
	bid.inl++;
	if ( bid.inl >= linenames_->size() )
	    return false;

	bid.crl = trcrgs_[bid.inl].start;
    }

    return true;
}


void HorizonModifier::getLines( const EM::Horizon* hor )
{
    if ( !linenames_ ) return;

    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return;

    const int sid = hor2d->sectionID( 0 );
    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
    {
	const int lid = hor2d->geometry().lineID( ldx );
	const char* linenm = hor2d->geometry().lineName( lid );
	const Geometry::Horizon2DLine* geom =
	    				hor2d->geometry().sectionGeometry(sid);
	if ( !geom ) return;

	const int lidx = linenames_->indexOf(linenm);
	if ( lidx < 0 )
	{
	    linenames_->add( linenm );
	    trcrgs_ += geom->colRange( lid );
	}
	else
	    trcrgs_[lidx].include( geom->colRange(lid) );
    }
}


void HorizonModifier::doWork()
{
    BinID binid;
    while ( getNextNode(binid) )
    {
	float topz = mUdf(float), botz = mUdf(float);
	if ( is2d_ )
	{
	    topz = getDepth2D( tophor_, binid );
	    botz = getDepth2D( bothor_, binid );
	}
	else
	{
	    const EM::SubID subid = binid.toInt64();
	    topz = tophor_->getPos( tophor_->sectionID(0), subid ).z;
	    botz = bothor_->getPos( bothor_->sectionID(0), subid ).z;
	}

	if ( botz >= topz || mIsUdf(topz) || mIsUdf(botz) ) continue;

	if ( modifymode_ == Shift )
	    shiftNode( binid );
	else if ( modifymode_ == Remove )
	    removeNode( binid );
    }
}


float HorizonModifier::getDepth2D( const EM::Horizon* hor, const BinID& bid )
{
    if (      !hor ) return mUdf(float);

    const EM::SectionID sid = hor->sectionID(0);
    mDynamicCastGet(const EM::Horizon2D*,hor2d,hor)
    if ( !hor2d ) return mUdf(float);

    const int lidx = hor2d->geometry().lineIndex( linenames_->get(bid.inl) );
    const int lid = hor2d->geometry().lineID( lidx );
    const EM::SubID subid = BinID( lid, bid.crl ).toInt64();
    return hor2d->getPos( sid, subid ).z;
}


void HorizonModifier::shiftNode( const BinID& bid )
{
    const EM::Horizon* statichor = topisstatic_ ? tophor_ : bothor_;
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;

    const float extrashift = topisstatic_ ? 0.001 : -0.001;
    EM::SubID staticsubid, dynamicsubid;
    if ( !is2d_ )
	staticsubid = dynamicsubid = bid.toInt64();
    else
    { 
	mDynamicCastGet(const EM::Horizon2D*,statichor2d,statichor)
	if ( !statichor2d || !linenames_ ) return;

	int lidx = statichor2d->geometry().lineIndex( linenames_->get(bid.inl));
	int lid = statichor2d->geometry().lineID( lidx );
	staticsubid = BinID( lid, bid.crl ).toInt64();
     
	mDynamicCastGet(EM::Horizon2D*,dynamichor2d,dynamichor)
	if ( !dynamichor2d ) return;

	lidx = dynamichor2d->geometry().lineIndex( linenames_->get(bid.inl) );
	lid = dynamichor2d->geometry().lineID( lidx );
	dynamicsubid = BinID( lid, bid.crl ).toInt64();
    }

    float newz = statichor->getPos( statichor->sectionID(0), staticsubid ).z;
    if ( !mIsUdf(newz) )
	newz += extrashift;

    Coord3 pos = dynamichor->getPos( dynamichor->sectionID(0), dynamicsubid );
    pos.z = newz;
    dynamichor->setPos( dynamichor->sectionID(0), dynamicsubid, pos, false );
}


void HorizonModifier::removeNode( const BinID& bid )
{
    EM::Horizon* dynamichor = topisstatic_ ? bothor_ : tophor_;
    EM::SubID subid;
    if ( !is2d_ )
	subid = bid.toInt64();
    else
    { 
	mDynamicCastGet(EM::Horizon2D*,dynamichor2d,dynamichor)
	if ( !dynamichor2d || !linenames_ ) return;

	const int lidx = dynamichor2d->geometry().lineIndex(
						linenames_->get(bid.inl) );
	const int lid = dynamichor2d->geometry().lineID( lidx );
	subid = BinID( lid, bid.crl ).toInt64();
    }

    dynamichor->unSetPos( dynamichor->sectionID(0), subid, false );
}
