/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: jobdescprov.cc,v 1.6 2005-10-20 07:15:23 cvsarend Exp $
________________________________________________________________________

-*/

#include "jobdescprov.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "undefval.h"
#include <iostream>

const char* InlineSplitJobDescProv::sKeyMaxInlRg = "Maximum Inline Range";
const char* InlineSplitJobDescProv::sKeyMaxCrlRg = "Maximum Crossline Range";


JobDescProv::JobDescProv( const IOPar& iop )
	: inpiopar_(*new IOPar(iop))
{
}


JobDescProv::~JobDescProv()
{
    delete &inpiopar_;
}


KeyReplaceJobDescProv::KeyReplaceJobDescProv( const IOPar& iop, const char* key,
					      const BufferStringSet& nms )
	: JobDescProv(iop)
	, key_(key)
    	, names_(nms)
{
}


void KeyReplaceJobDescProv::getJob( int jid, IOPar& iop ) const
{
    iop = inpiopar_;
    iop.set( key_, objName(jid) );
}


const char* KeyReplaceJobDescProv::objName( int jid ) const
{
    objnm_ = names_.get( jid );
    return objnm_.buf();
}


void KeyReplaceJobDescProv::dump( std::ostream& strm ) const
{
    strm << "\nKey-replace JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<names_.size(); idx++ )
	strm << names_.get(idx) << ' ';
    strm << std::endl;
}


#define mSetInlRgDef() \
    Interval<int> dum; SI().sampling(false).hrg.get( inlrg_, dum )


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
					     	const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(0)
	, ninlperjob_( 1 )
{
    mSetInlRgDef();
    getRange( inlrg_ );
    iop.get( "Nr of Inlines per Job", ninlperjob_ );
}


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
			const TypeSet<int>& in, const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(new TypeSet<int>(in))
	, ninlperjob_( 1 )
{
    mSetInlRgDef();
}


InlineSplitJobDescProv::~InlineSplitJobDescProv()
{
    delete inls_;
}


void InlineSplitJobDescProv::getRange( StepInterval<int>& rg ) const
{
    rg.step = 0;

    if ( *(const char*)singlekey_ )
	inpiopar_.get( singlekey_, rg.start, rg.stop, rg.step );
    else
    {
	inpiopar_.get( sKey::FirstInl, rg.start );
	inpiopar_.get( sKey::LastInl, rg.stop );
	inpiopar_.get( sKey::StepInl, rg.step );
    }

    if ( rg.step < 0 ) rg.step = -rg.step;
    if ( !rg.step ) rg.step = SI().inlStep();
    rg.sort();

    Interval<int> maxrg; assign( maxrg, rg );
    inpiopar_.get( sKeyMaxInlRg, maxrg.start, maxrg.stop );
    if ( !Values::isUdf(maxrg.start) && rg.start < maxrg.start )
	rg.start = maxrg.start;
    if ( !Values::isUdf(maxrg.stop) && rg.stop > maxrg.stop )
	rg.stop = maxrg.stop;
}


int InlineSplitJobDescProv::nrJobs() const
{
    if ( inls_ ) return inls_->size();

    int nrinl = inlrg_.nrSteps() + 1;

    int ret = nrinl / ninlperjob_; 
    if ( nrinl % ninlperjob_ ) ret += 1;
    
    return ret;
}


int InlineSplitJobDescProv::firstInlNr( int jid ) const
{
    if ( inls_ ) return (*inls_)[jid];

    int inlnr = jid * ninlperjob_;

    return inlrg_.atIndex(inlnr);
}


int InlineSplitJobDescProv::lastInlNr( int jid ) const
{
    if ( inls_ ) return (*inls_)[jid];

    int inlnr = (( jid + 1 ) * ninlperjob_) - 1;
    inlnr = mMIN( inlnr, inlrg_.nrSteps() );

    return inlrg_.atIndex(inlnr);
}


void InlineSplitJobDescProv::getJob( int jid, IOPar& iop ) const
{
    iop = inpiopar_;
    const int frstinl = firstInlNr( jid );
    const int lastinl = lastInlNr( jid );

    if ( *(const char*)singlekey_ )
	iop.set( singlekey_, frstinl, lastinl, inlrg_.step );
    else
    {
	iop.set( sKey::FirstInl, frstinl );
	iop.set( sKey::LastInl, lastinl );
    }
}


const char* InlineSplitJobDescProv::objName( int jid ) const
{
    objnm_ = ""; objnm_ += firstInlNr( jid );
    if ( lastInlNr(jid) >  firstInlNr( jid ) )
	{ objnm_ += "-"; objnm_ += lastInlNr(jid); }

    return objnm_.buf();
}


void InlineSplitJobDescProv::dump( std::ostream& strm ) const
{
    strm << "\nInline-split JobDescProv dump.\n";
    if ( !inls_ )
	strm << "Inline range: " << inlrg_.start << '-' << inlrg_.stop
		<< " / " << inlrg_.step;
    else
    {
	strm << "The following inlines are requested:\n";
	for ( int idx=0; idx<inls_->size(); idx++ )
	    strm << (*inls_)[idx] << ' ';
    }
    strm << std::endl;
}
