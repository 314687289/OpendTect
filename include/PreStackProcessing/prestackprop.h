#ifndef prestackprop_h
#define prestackprop_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id: prestackprop.h,v 1.1 2009-02-04 14:36:15 cvskris Exp $
________________________________________________________________________

-*/

#include "stattype.h"
#include "enums.h"
#include "ranges.h"
#include "datapack.h"

class BinID;
class SeisTrcBuf;
class SeisPSReader;

namespace PreStack
{

class Gather;

/*!\brief calculates 'post-stack' properties of a Pre-Stack data store */

mClass PropCalc
{
public:

    enum CalcType	{ Stats, LLSQ };
    			DeclareEnumUtils(CalcType)
    enum AxisType	{ Norm, Log, Exp, Sqr, Sqrt, Abs };
    			DeclareEnumUtils(AxisType)
    enum LSQType	{ A0, Coeff, StdDevA0, StdDevCoeff, CorrCoeff };
    			DeclareEnumUtils(LSQType)

    mClass Setup
    {
    public:
			Setup()
			    : calctype_(Stats)
			    , stattype_(Stats::Average)
			    , lsqtype_(A0)
			    , offsaxis_(Norm)
			    , valaxis_(Norm)
			    , offsrg_(0,mUdf(float))
			    , useazim_(false)
			    , aperture_(0)	{}

	mDefSetupMemb(CalcType,calctype)
	mDefSetupMemb(Stats::Type,stattype)
	mDefSetupMemb(LSQType,lsqtype)
	mDefSetupMemb(AxisType,offsaxis)
	mDefSetupMemb(AxisType,valaxis)
	mDefSetupMemb(Interval<float>,offsrg)
	mDefSetupMemb(bool,useazim)
	mDefSetupMemb(int,aperture)
    };
			PropCalc(const Setup&);
    virtual		~PropCalc();

    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }

    void		setGather(DataPack::ID);
    float		getVal(int sampnr) const;
    float		getVal(float z) const;

    static float	getVal( const PropCalc::Setup& su,
			        TypeSet<float>& vals, TypeSet<float>& offs );


protected:
    void		removeGather();

    Gather*		gather_;
    Setup		setup_;
};

}; //Namespace


#endif
