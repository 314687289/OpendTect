/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackagc.cc,v 1.11 2008-04-01 18:20:14 cvskris Exp $";

#include "prestackagc.h"

#include "agc.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "iopar.h"
#include "prestackgather.h"
#include "varlenarray.h"
#include "survinfo.h"


void PreStack::AGC::initClass()
{
    PF().addCreator( PreStack::AGC::createFunc, PreStack::AGC::sName() );
}


PreStack::Processor* PreStack::AGC::createFunc()
{ return new PreStack::AGC; }


PreStack::AGC::AGC()
    : Processor( sName() )
    , window_( -100, 100 )
    , mutefraction_( 0 )
{}


bool PreStack::AGC::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] ) continue;

	const float zstep = inputs_[idx]->posData().range(false).step;

	samplewindow_.start = mNINT( window_.start/zstep );
	samplewindow_.stop = mNINT( window_.stop/zstep );
	return true;
    }

    return false;
}


void PreStack::AGC::setWindow( const Interval<float>& iv )
{ window_ = iv; }


const Interval<float>& PreStack::AGC::getWindow() const
{ return window_; }


void PreStack::AGC::getWindowUnit( BufferString& buf, bool parens ) const
{
    buf = SI().getZUnit( parens );
}


void PreStack::AGC::setLowEnergyMute( float iv )
{ mutefraction_ = iv; }


float PreStack::AGC::getLowEnergyMute() const
{ return mutefraction_; }


void PreStack::AGC::fillPar( IOPar& par ) const
{
    par.set( sKeyWindow(), window_ );
    par.set( sKeyMuteFraction(), mutefraction_ );
}


bool PreStack::AGC::usePar( const IOPar& par )
{
    par.get( sKeyWindow(), window_ );
    par.get( sKeyMuteFraction(), mutefraction_ );
    return true;
}


bool PreStack::AGC::doWork( int start, int stop, int )
{
    ::AGC<float> agc;
    agc.setMuteFraction( mutefraction_ );

    for ( int offsetidx=start; offsetidx<=stop; offsetidx++, reportNrDone() )
    {
	for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	{
	    Gather* output = outputs_[idx];
	    const Gather* input = inputs_[idx];
	    if ( !output || !input )
		continue;

	    Array1DSlice<float> inputtrace( input->data() );
	    inputtrace.setDimMap( 0, Gather::zDim() );
	    inputtrace.setPos( Gather::offsetDim(), offsetidx );

	    Array1DSlice<float> outputtrace( output->data() );
	    outputtrace.setDimMap( 0, Gather::zDim() );
	    outputtrace.setPos( Gather::offsetDim(), offsetidx );
	    if ( !inputtrace.init() || !outputtrace.init() )
		continue;

	    agc.setInput( inputtrace, inputtrace.info().getSize(0) );
	    agc.setOutput( outputtrace );
	    agc.execute( false );
	}
    }

    return true;
}
