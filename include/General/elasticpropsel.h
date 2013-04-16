#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "elasticprop.h"

class IOObj;
class MultiID;

/*!
\brief User parameters to compute values for an elastic layer (den,p/s-waves).
*/

mExpClass(General) ElasticPropSelection : public PropertyRefSelection
{
public:
				ElasticPropSelection();
				ElasticPropSelection(
					const ElasticPropSelection& elp)
				{ *this = elp; }

    ElasticPropertyRef&		get(int idx) 		{ return gt(idx); }
    const ElasticPropertyRef&	get(int idx) const	{ return gt(idx); }
    ElasticPropertyRef&		get(ElasticFormula::Type tp) 
    							{ return gt(tp); }
    const ElasticPropertyRef&	get(ElasticFormula::Type tp) const
    							{ return gt(tp); }

    static ElasticPropSelection* get(const MultiID&);
    static ElasticPropSelection* get(const IOObj*);
    bool                	put(const IOObj*) const;

    bool			isValidInput(BufferString* errmsg=0) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    ElasticPropertyRef&		gt(ElasticFormula::Type) const;
    ElasticPropertyRef&		gt(int idx) const;
};


/*!
\brief Computes elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mExpClass(General) ElasticPropGen
{
public:
    			ElasticPropGen(const ElasticPropSelection& eps,
					const PropertyRefSelection& rps)
			    : elasticprops_(eps), refprops_(rps) {}

    float		getVal(const ElasticPropertyRef& ef,
	    			const float* proprefvals,
				int proprefsz) const
    			{ return getVal(ef.formula(),proprefvals, proprefsz); }

    void 		getVals(float& den,float& pbel,float& svel,
	    			const float* proprefvals,int proprefsz) const;

protected:

    ElasticPropSelection elasticprops_;
    const PropertyRefSelection& refprops_;

    float		getVal(const ElasticFormula& ef,
	    			const float* proprefvals,
				int proprefsz) const;
};


/*!
\brief Guesses elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mExpClass(General) ElasticPropGuess
{
public:
    			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:

    void		guessQuantity(const PropertyRefSelection&,
					ElasticFormula::Type);
    bool		guessQuantity(const PropertyRef&,ElasticFormula::Type);

    ElasticPropSelection& elasticprops_; 
};


#endif


