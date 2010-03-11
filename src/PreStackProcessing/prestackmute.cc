/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackmute.cc,v 1.15 2010-03-11 09:38:09 cvsnanne Exp $";

#include "prestackmute.h"

#include "arrayndslice.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"


using namespace PreStack;

void Mute::initClass()
{
    PF().addCreator( Mute::createFunc, Mute::sName() );
}


Processor* Mute::createFunc()
{
    return new Mute;
}


Mute::Mute()
    : Processor( sName() )
    , def_(*new MuteDef)
    , muter_(0)
    , tail_(false)
    , taperlen_(10)
{}


Mute::~Mute()
{
    delete muter_;
    delete &def_;
}


#define mErrRet(s) \
{ delete muter_; muter_ = 0; errmsg_ = s; return false; }

bool Mute::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    if ( muter_ ) return true;

    muter_ = new Muter( taperlen_, tail_ );
    return true;
}


void Mute::setEmptyMute()
{
    id_ = MultiID();
    while ( def_.size() )
	def_.remove( 0 );
}


void Mute::setTailMute( bool yn )
{ tail_ = yn; delete muter_; muter_ = 0; }


void Mute::setTaperLength( float l )
{ taperlen_ = l; delete muter_; muter_ = 0; }


bool Mute::setMuteDefID( const MultiID& mid )
{
    if ( id_==mid )
	return true;

    if ( mid.isEmpty() )
	mErrRet("No MuteDef ID provided")
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	mErrRet("Cannot find MuteDef ID in Object Manager")

    if ( !MuteDefTranslator::retrieve(def_,ioobj,errmsg_) )
	mErrRet(errmsg_)
    
    id_ = mid;
    
    return true;
}


void Mute::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( id_ );
    if ( ioobj )
	par.set( sMuteDef(), id_ );

    par.set( sTaperLength(), taperlen_ );
    par.setYN( sTailMute(), tail_ );
}


bool Mute::usePar( const IOPar& par )
{
    float taperlen;
    if ( par.get( sTaperLength(), taperlen ) )
	setTaperLength( taperlen );

    bool tail;
    if ( par.getYN( sTailMute(), tail ) )
	setTailMute( tail );

    MultiID mid;
    if ( par.get(sMuteDef(),mid) && !setMuteDefID(mid) )
    {
	errmsg_ = "No Mute definition ID found";
	return false;
    }

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Mute::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !muter_ )
	return false;

    for ( int ioffs=start; ioffs<=stop; ioffs++, addToNrDone(1) )
    {
	for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	{
	    Gather* output = outputs_[idx];
	    const Gather* input = inputs_[idx];
	    if ( !output || !input )
		continue;

	    const int nrsamples = input->data().info().getSize(Gather::zDim());
	    const float offs = input->getOffset(ioffs);
	    const float mutez = def_.value( offs, input->getBinID() );
	    if ( mIsUdf(mutez) )
		continue;

	    const float mutepos =
		Muter::mutePos( mutez, input->posData().range(false) );

	    Array1DSlice<float> slice( output->data() );
	    slice.setPos( 0, ioffs );
	    if ( !slice.init() )
		continue;

	    muter_->mute( slice, nrsamples, mutepos );
	}
    }

    return true;
}
