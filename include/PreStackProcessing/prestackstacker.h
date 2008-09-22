#ifndef prestackstacker_h
#define prestackstacker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackstacker.h,v 1.4 2008-09-22 13:10:42 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "multiid.h"


template <class T> class Interval;


namespace PreStack
{

class Stack : public Processor
{
public:

    static void			initClass();
    static Processor*		createFunc();
    static const char*		sName()			{ return "Stack"; }

 				Stack();
    				~Stack();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    const char*			errMsg() const;

    void			setOffsetRange(const Interval<float>*);
				//!<Null pointer means all offsets
    const Interval<float>*	getOffsetRange() const;
				//!<Null pointer means all offsets

protected:
    static const char*		sKeyOffsetRange() { return "Offset Range"; }
    Gather*			createOutputArray(const Gather&) const;
    od_int64			totalNr() const;
    bool			doWork(od_int64,od_int64,int);

    BufferString		errmsg_;
    Interval<float>*		offsetrg_;
};


}; //namespace

#endif
