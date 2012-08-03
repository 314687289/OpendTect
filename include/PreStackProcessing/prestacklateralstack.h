#ifndef prestacklateralstack_h
#define prestacklateralstack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestacklateralstack.h,v 1.5 2012-08-03 13:00:33 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "multiid.h"
#include "offsetazimuth.h"
#include "prestackprocessor.h"

namespace PreStack
{

mClass(PreStackProcessing) LateralStack : public Processor
{
public:
    			mDefaultFactoryInstanciationBase(
				"LateralStack", "Super Gather" );
    static Processor*	createInstance();

 			LateralStack();
    			~LateralStack();

    bool		reset();

    bool		wantsInput(const BinID&) const;
    bool		setPattern(const BinID& stepout,bool cross);
    			//If cross if false, it will be a rectangle
    bool		isCross() const 	{ return iscross_; }
    const BinID&	getPatternStepout() const { return patternstepout_; }
    const BinID&	getInputStepout() const { return inputstepout_; }
    bool		setOutputInterest(const BinID& relbid,bool);

    bool		prepareWork();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    const char*		errMsg() const		{ return errmsg_.str(); }


protected:
    bool		isInPattern(const BinID&) const;
    bool		processOutput( const OffsetAzimuth&,const BinID&);
    static const char*	sKeyStepout()		{ return "Stepout"; }
    static const char*	sKeyCross()		{ return "Is cross"; }

    BufferString	errmsg_;
    bool		doWork(od_int64,od_int64,int);
    od_int64		nrIterations() const	{ return offsetazi_.size(); }

    BinID		inputstepout_;
    BinID		patternstepout_;
    bool		iscross_;

    TypeSet<OffsetAzimuth>	offsetazi_;
};


}; //namespace

#endif

