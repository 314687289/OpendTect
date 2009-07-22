/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackstacker.cc,v 1.8 2009-07-22 16:01:34 cvsbert Exp $";

#include "prestackstacker.h"

#include "ranges.h"
#include "iopar.h"
#include "prestackgather.h"


using namespace PreStack;

void Stack::initClass()
{
    PF().addCreator( Stack::createFunc, Stack::sName() );
}


Processor* Stack::createFunc()
{ return new Stack; }


Stack::Stack()
    : Processor( sName() )
    , offsetrg_( 0 )
{ }


Stack::~Stack()
{
    delete offsetrg_;
}


const char* Stack::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }


#define mErrRet(s) \
{ delete muter_; muter_ = 0; errmsg_ = s; return false; }


void Stack::setOffsetRange( const Interval<float>* offrg )
{
    if ( offrg )
    {
	if ( !offsetrg_ ) offsetrg_ = new Interval<float>( *offrg );
	else *offsetrg_ = *offrg;
    }
    else
    {
	delete offsetrg_;
	offsetrg_ = 0;
    }
}


void Stack::fillPar( IOPar& par ) const
{
    if ( offsetrg_ )
	par.set( sKeyOffsetRange(), offsetrg_->start, offsetrg_->stop );
}


bool Stack::usePar( const IOPar& par )
{
    Interval<float> rg;
    setOffsetRange( par.get( sKeyOffsetRange(), rg ) ? &rg : 0 );

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Stack::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idz=start; idz<=stop; idz++, addToNrDone(1) )
    {
	 for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	 {
	     Gather* output = outputs_[idx];
	     const Gather* input = inputs_[idx];
	     if ( !output || !input )
		 continue;

	     if ( idz>=input->data().info().getSize( Gather::zDim() ) ||
		  idz>=output->data().info().getSize( Gather::zDim() ) )
		 continue;

	    const int nroffsets =
		input->data().info().getSize(Gather::offsetDim());
	    int nrvals = 0;
	    float stack;
	    for ( int ioff=0; ioff<nroffsets; ioff++ )
	    {
		const float offset = input->getOffset(ioff);
		if ( offsetrg_ && !offsetrg_->includes( offset ) )
		    continue;

		const float val = input->data().get( ioff, idz );
		if ( mIsUdf(val) )
		    continue;

		if ( !nrvals ) stack = val;
		else stack += val;

		nrvals += 0;
	    }

	    output->data().set( 0, idz, nrvals ? stack/nrvals : mUdf(float) );
	}
    }

    return true;
}


Gather* Stack::createOutputArray( const Gather& input ) const
{
    Gather* res = new Gather( input );
    if ( !res->setSize( 1, input.size(false) ) )
    {
	delete res;
	return 0;
    }

    return res;
}


od_int64 Stack::nrIterations() const
{
    int max = 0;
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	const int nrz = inputs_[idx]->data().info().getSize( Gather::zDim() );

	max = mMAX(max,nrz);
    }

    return max;
}
