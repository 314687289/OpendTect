/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : April 2013
 * FUNCTION : Test various functions of TrcKeySampling and TrcKeyZSampling.
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "testprog.h"
#include "trckeyzsampling.h"
#include "survinfo.h"

#define mDeclTrcKeyZSampling( cs, istart, istop, istep, \
			       cstart, cstop, cstep, \
			       zstart, zstop, zstep) \
    TrcKeyZSampling cs( false ); \
    cs.hsamp_.set( StepInterval<int>(istart,istop,istep), \
	        StepInterval<int>(cstart,cstop,cstep) ); \
    cs.zsamp_.set( zstart, zstop, zstep );


#define mRetResult( funcname ) \
    { \
	od_cout() << funcname << " failed" << od_endl; \
	return false; \
    } \
    else if ( !quiet ) \
	od_cout() << funcname << " succeeded" << od_endl; \
    return true;


static bool testEmpty()
{
    TrcKeyZSampling cs0( false );
    cs0.setEmpty();
    if ( cs0.nrInl() || cs0.nrCrl() || !cs0.isEmpty() )
	mRetResult( "testEmpty" );
}


static bool testInclude()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			    4, 100, 3,
			    -1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( expcs, 1, 101, 1,
			      4, 100, 3,
			      -1, 4, 0.004 );
    cs1.include ( cs2 );
    if ( cs1 != expcs )
	mRetResult( "testInclude" );
}


static bool testIncludes()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			    4, 100, 3,
			    -1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 1, 101, 1,
			      4, 100, 3,
			      -1, 4, 0.004 );
    if ( cs2.includes(cs1) || !cs3.includes(cs1) )
	mRetResult( "testIncludes" );
}


static bool testLimitTo()
{
    mDeclTrcKeyZSampling( cs1, 3, 63, 6,
			    10, 100, 1,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 13, 69, 4,
			    4, 100, 1,
			    -1, 2.0, 0.005 );
    mDeclTrcKeyZSampling( csexp, 21, 57, 12,
			    10, 100, 1,
			    1, 2.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 2, 56, 2,
			    10, 100, 1,
			    1, 2.0, 0.004 );
    cs1.limitTo( cs2 );
    cs2.limitTo( cs3 );
    if ( cs1 != csexp || !cs2.isEmpty() )
	mRetResult( "testLimitTo" );
}


bool testIterator()
{
    TrcKeySampling hrg;
    hrg.survid_ = TrcKey::std3DSurvID();
    hrg.set( StepInterval<int>( 0, 2, 2 ), StepInterval<int>( 0, 2, 2 ) );

    TrcKeySamplingIterator iter( hrg );
    BinID curbid;

    mRunStandardTest( iter.next(curbid) && curbid==BinID(0,0), "Initial call");
    mRunStandardTest( iter.next(curbid) && curbid==BinID(0,2), "Second call");
    mRunStandardTest( iter.next(curbid) && curbid==BinID(2,0), "Third call");
    mRunStandardTest( iter.next(curbid) && curbid==BinID(2,2), "Forth call");
    mRunStandardTest( !iter.next(curbid) , "Final call");

    iter.reset();
    mRunStandardTest( iter.next(curbid) && curbid==BinID(0,0), "Reset");

    iter.setNextPos( BinID(2,0) );
    mRunStandardTest( iter.next(curbid) && curbid==BinID(2,0), "setNextPos");

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    mDeclTrcKeyZSampling( survcs, 1, 501, 2,
			    10, 100, 2,
			    1.0, 10.0, 0.004 );
    eSI().setRange( survcs, false );
    eSI().setRange( survcs, true ); //For the sanity of SI().

    if ( !testInclude()
      || !testIncludes()
      || !testEmpty()
      || !testLimitTo()
      || !testIterator() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}

