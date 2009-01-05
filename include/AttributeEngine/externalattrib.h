#ifndef externalattrib_h
#define externalattrib_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: externalattrib.h,v 1.11 2009-01-05 09:49:43 cvsranojay Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "datapack.h"
#include "sets.h"

class BinIDValueSet;
class CubeSampling;
class LineKey;
class SeisTrcBuf;
class TaskRunner;


namespace Attrib
{

class SelSpec;
class DataCubes;

/*! Generic class for attribs that does not come from the attribute engine. */


mClass ExtAttribCalc
{
public:
    virtual			~ExtAttribCalc()			{}
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
    				/*!<\returns if this object can 
				     compute it or not. */
    //Old Interface, use functions with progressmeter instead
    virtual DataPack::ID	createAttrib(const CubeSampling&, DataPack::ID);
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&);
    virtual bool		createAttrib(const BinIDValueSet&, SeisTrcBuf&);
    virtual DataPack::ID	createAttrib(const CubeSampling&,
	    				     const LineKey&);

    //New interface
    virtual DataPack::ID	createAttrib(const CubeSampling&,
					     DataPack::ID, TaskRunner*);
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&,
	    				     TaskRunner*);
    virtual bool		createAttrib(const BinIDValueSet&, SeisTrcBuf&,
					     TaskRunner*);
    virtual DataPack::ID	createAttrib(const CubeSampling&,
	    				     const LineKey&, TaskRunner*);

    virtual bool		isIndexes() const;

    BufferString		errmsg_;
};


mDefineFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


} // namespace Attrib

#endif
