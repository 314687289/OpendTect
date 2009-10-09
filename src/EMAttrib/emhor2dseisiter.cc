/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: emhor2dseisiter.cc,v 1.2 2009-10-09 12:03:00 cvsbert Exp $";

#include "emhor2dseisiter.h"

#include "emmanager.h"
#include "emhorizon2d.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "ioman.h"
#include "ioobj.h"


#define mGetEMObjPtr(mid) EM::EMM().getObject( EM::EMM().getObjectID(mid) )


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const EM::Horizon2D& h2d )
    : nrlines_(0)
{
    init( &h2d );
}


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const MultiID& mid )
    : nrlines_(0)
{
    EM::EMObject* emobj = mGetEMObjPtr( mid );
    mDynamicCastGet(EM::Horizon2D*,h2d,emobj)
    init( h2d );
}


void EM::Hor2DSeisLineIterator::init( const Horizon2D* h2d )
{
    lset_ = 0; curlsid_ = "0"; lineidx_ = -1;

    h2d_ = h2d;
    if ( h2d_ )
    {
	h2d_->ref();
	geom_ = &h2d_->geometry();
	const_cast<int&>(nrlines_) = geom_->nrLines();
    }
}


EM::Hor2DSeisLineIterator::~Hor2DSeisLineIterator()
{
    if ( h2d_ )
	h2d_->unRef();
    delete lset_;
}


bool EM::Hor2DSeisLineIterator::next()
{
    lineidx_++;
    if ( lineidx_ < nrlines_ )
	getLineSet();
    return isValid();
}

bool EM::Hor2DSeisLineIterator::isValid() const
{
    return h2d_ && lineidx_ >= 0 && lineidx_ < nrlines_;
}

void EM::Hor2DSeisLineIterator::reset()
{
    lineidx_ = -1;
}

void EM::Hor2DSeisLineIterator::getLineSet()
{
    if ( !isValid() )
	{ delete lset_; lset_ = 0; return; }

    const int lineid = geom_->lineID( lineidx_ );
    const MultiID& lsid = geom_->lineSet( geom_->lineID(lineid) );
    if ( !lset_ || lsid != curlsid_ )
    {
	delete lset_; lset_ = 0;
	IOObj* ioobj = IOM().get( lsid );
	if ( ioobj )
	{
	    lset_ = new Seis2DLineSet( *ioobj );
	    curlsid_ = lsid;
	    delete ioobj;
	}
    }
}


int EM::Hor2DSeisLineIterator::lineID() const
{
    return lineidx_ >= 0 ? geom_->lineID(lineidx_) : -1;
}

const char* EM::Hor2DSeisLineIterator::lineName() const
{
    return isValid() ? geom_->lineName( lineID() ) : 0;
}


int EM::Hor2DSeisLineIterator::lineSetIndex( const char* attrnm ) const
{
    if ( !isValid() || !lset_ ) return -1;

    const LineKey lk( lineName(), attrnm );
    for ( int iln=0; iln<lset_->nrLines(); iln++ )
    {
	if ( lset_->lineKey(iln) == lk )
	    return iln;
    }

    return -1;
}
