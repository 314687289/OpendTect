#ifndef expnodc_h
#define expnodc_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: expnodc.h,v 1.2 2002-09-05 15:50:21 kristofer Exp $
________________________________________________________________________

NoDC

NoDC removes the DC attribute on the traces.

Input:
0	Input Data

Output:
0	Output Data

@$*/

#include <attribcalc.h>
#include <basictask.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <attribparamimpl.h>


class NoDCAttrib : public AttribCalc
{
public:
			mAttrib0Param(NoDCAttrib,"NoDC");

			NoDCAttrib( Parameters* );
			~NoDCAttrib();

    int                 nrAttribs() const { return 1; }

    Seis::DataType	dataType(int,const TypeSet<Seis::DataType>&) const
			{ return Seis::UnknowData; }

    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }

    const char* 	definitionStr() const { return desc; }
protected:
    
    BufferString	desc;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const NoDCAttrib& calculator_ )
				    : calculator ( calculator_ )
				    , trc( 0 )
				{}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
				{ return new NoDCAttrib::Task::Input(*this);}

	    SeisTrc*		trc;
	    int 		attribute;
	    const NoDCAttrib&	calculator;
	};

			    Task( const NoDCAttrib& calculator_ )
				: outp( 0 )
				, calculator( calculator_ ) {}
	
			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
	void		    set( float t1_, int nrtimes_, float step_, 
					    const AttribCalc::Task::Input* inp,
                                            const TypeSet<float*>& outp_)
				{ t1 = t1_; nrtimes = nrtimes_; 
				  step = step_; input = inp; outp = outp_[0]; }

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return INT_MAX; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
			    { return new NoDCAttrib::Task::Input( calculator ); }

    protected:
	float*		outp;
	const NoDCAttrib& calculator;

    };

    friend NoDCAttrib::Task;
    friend NoDCAttrib::Task::Input;
};

#endif
