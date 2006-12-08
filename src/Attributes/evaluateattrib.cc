/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Payraudeau
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: evaluateattrib.cc,v 1.9 2006-12-08 15:43:10 cvshelene Exp $";


#include "evaluateattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"


namespace Attrib
{
mAttrDefCreateInstance(Evaluate)
    
void Evaluate::initClass()
{
    mAttrStartInitClass
    mAttrEndInitClass
}


Evaluate::Evaluate( Desc& ds )
        : Provider( ds )
{
    if ( !isOK() ) return;

    inputdata_.allowNull( true );
}


bool Evaluate::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Evaluate::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < inputs.size() )
    {
	inputdata_ += 0;
	dataidx_ += -1;
    }

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, zintv );
	if ( !data ) return false;

	inputdata_.replace( idx, data );
	dataidx_ [idx] = getDataIndex( idx );
    }
    
    return true;
}


bool Evaluate::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples ) const
{
    if ( inputdata_.isEmpty() || output.isEmpty() ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	for ( int sidx=0; sidx<output.nrSeries(); sidx++ )
	{
	    if ( !isOutputEnabled(sidx) ) continue;

	    const ValueSeries<float>* valseries = 
			inputdata_[sidx]->series( dataidx_[sidx] );

	    if ( !valseries )
		setOutputValue( output, sidx, idx, z0, mUdf(float) );
	    else
		setOutputValue( output, sidx, idx, z0,
		        valseries->value(cursample-inputdata_[sidx]->z0_) );
	}
    }

    return true;
}
			
} // namespace Attrib
