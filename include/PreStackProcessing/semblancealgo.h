#ifndef semblancecalculator_h
#define semblancecalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: semblancealgo.h,v 1.1 2010-11-08 22:06:03 cvskris Exp $
________________________________________________________________________


-*/

#include "factory.h"
#include "objectset.h"


namespace PreStack
{

class Gather;

/*! Base class for algorithms that computes semblance along a moveout */
mClass SemblanceAlgorithm
{
public:
    			mDefineFactoryInClass( SemblanceAlgorithm, factory );
    virtual		~SemblanceAlgorithm();
    virtual float	computeSemblance( const Gather&,
					  const float* moveout,
	   				  const bool* usetrace = 0 ) const = 0;
    			/*!<Computes the semblance along the moveout.
			    \param moveout - One depth (absolute) per trace in
					     the gather, in the same order as
					     the gather.
			    \param usetrace  Optional array with one entry per
			    		     trace in the gather, int the same
					     order as the gather. If provided,
					     only traces with 'true' will be
					     included in computation. */
};

}; //namespace

#endif
