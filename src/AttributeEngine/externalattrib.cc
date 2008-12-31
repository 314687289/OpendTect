/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: externalattrib.cc,v 1.7 2008-12-31 05:40:45 cvsranojay Exp $";

#include "externalattrib.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplFactory1Param(  ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, DataPack::ID )
{ return DataPack::cNoID(); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>& )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf )
{ return false; }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, const LineKey& )
{ return DataPack::cNoID(); }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling& cs,
	DataPack::ID dp, TaskRunner*	)
{ return createAttrib( cs, dp ); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>& o,
       TaskRunner*	)
{ return createAttrib( o ); }


bool ExtAttribCalc::createAttrib( const BinIDValueSet& bv, SeisTrcBuf& buf,
       TaskRunner*	)
{ return createAttrib( bv, buf ); }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling& cs,
	const LineKey& lk, TaskRunner* )
{ return createAttrib( cs, lk ); }


bool ExtAttribCalc::isIndexes() const
{ return false; }


} // namespace Attrib
