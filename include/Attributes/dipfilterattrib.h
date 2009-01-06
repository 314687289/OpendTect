#ifndef dipfilterattrib_h
#define dipfilterattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: dipfilterattrib.h,v 1.11 2009-01-06 10:29:52 cvsranojay Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "arrayndimpl.h"

/*!\brief Dip filtering Attribute

DipFilter size= minvel= maxvel= type=LowPass|HighPass|BandPass
    filterazi=Y/N minazi= maxazi= taperlen=

DipFilter convolves a signal with the on the command-line specified signal.

minvel/maxvel is the cutoff velocity (m/s) or dip (degrees), depending
on the z-range unit.

Azimuth is given in degrees from the inline, in the direction
of increasing crossline-numbers.

Taper is given as percentage. Tapering is done differently for the three
different filtertype.
Bandpass tapers at the upper and lower taperlen/2 parts of t the interval.
Lowpass tapers from the maxvel down to 100-taperlen of the maxvel.
Highpass tapers from minvel to 100+taperlen of minvel

The azimuthrange is tapered in the same way as bandpass.

type = HighPass
          *     * minvel > 0
	  **   **
	  *** ***
	  *******
	  *** ***
	  **   **
	  *     *

type = LowPass
          ******* maxvel > 0
           *****
            ***
             *
            ***
           *****
          *******
								
type = BandPass
             *       *  minvel
            ***     ***
            ****   **** maxvel > 0
              *** ***
                 *
              *** ***
            ****   ****
            ***     ***
             *       *

Inputs:
0       Signal to be filtered.

*/

namespace Attrib
{

mClass DipFilter : public Provider
{
public:
    static void		initClass();
			DipFilter(Desc&);

    static const char*	attribName()	{ return "DipFilter"; }
    static const char*	sizeStr()	{ return "size"; }
    static const char*	typeStr()	{ return "type"; }
    static const char*	minvelStr()	{ return "minvel"; }
    static const char*	maxvelStr()	{ return "maxvel"; }
    static const char*	filteraziStr()	{ return "filterazi"; }
    static const char*	minaziStr()	{ return "minazi"; }
    static const char*	maxaziStr()	{ return "maxazi"; }
    static const char*  taperlenStr()   { return "taperlen"; }
    static const char*	filterTypeNamesStr(int);

protected:
			~DipFilter() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);


    bool		allowParallelComputation() const	{ return true; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples,int threadid) const;
    bool		initKernel();
    float		taper(float) const;

    const BinID*		reqStepout(int input,int output) const;

    int				size;
    int				type;
    float			minvel;
    float			maxvel;
    bool			filterazi;

    float			minazi;
    float			maxazi;
    float			taperlen;
    bool			isinited;

    Array3DImpl<float>  	kernel;
    Interval<float>     	valrange;
    float            	   	azi;
    float               	aziaperture;

    BinID               	stepout;
    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata;
};

}; // namespace Attrib


#endif

