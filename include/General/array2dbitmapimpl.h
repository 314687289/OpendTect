#ifndef array2dbitmapimpl_h
#define array2dbitmapimpl_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2006
 RCS:           $Id: array2dbitmapimpl.h,v 1.7 2007-02-15 13:24:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "array2dbitmap.h"


/*! \brief Common pars for A2DBitMapGenerators */

struct WVAA2DBitMapGenPars : public A2DBitMapGenPars
{
		WVAA2DBitMapGenPars()
		  : drawwiggles_(true)
		  , drawmid_(false)
		  , fillleft_(false)
		  , fillright_(true)
		  , midvalue_(0)
		  , minpixperdim0_(2)
		  , overlap_(0.5)	{}

    bool	drawwiggles_;	//!< Draw the wiggles themselves
    bool	drawmid_;	//!< Draw mid line for each trace
    bool	fillleft_;	//!< Fill the left loops
    bool	fillright_;	//!< Fill the right loops

    float	overlap_;	//!< If > 0, part of the trace is drawn on
    				//!< both neighbours' display strip
    				//!< If < 0, uses less than entire strip
    float	midvalue_;	//!< if mUdf(float), use the median data value
    int		minpixperdim0_;	//!< Set to 0 or neg for dump everything

    static const char	cZeroLineFill;		// => -126
    static const char	cWiggFill;		// => -125
    static const char	cLeftFill;		// => -124
    static const char	cRightFill;		// => -123

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitMap's. */

class WVAA2DBitMapGenerator : public A2DBitMapGenerator
{
public:

			WVAA2DBitMapGenerator(const A2DBitMapInpData&,
					      const A2DBitMapPosSetup&);

    WVAA2DBitMapGenPars&	wvapars()		{ return gtPars(); }
    const WVAA2DBitMapGenPars&	wvapars() const		{ return gtPars(); }

    int				dim0SubSampling() const;

protected:

    inline WVAA2DBitMapGenPars& gtPars() const
				{ return (WVAA2DBitMapGenPars&)pars_; }

    float			stripwidth_;

				WVAA2DBitMapGenerator(
					const WVAA2DBitMapGenerator&);
				//!< Not implemented to prevent usage
				//!< Copy the pars instead
    void			doFill();

    void			drawTrace(int);
    void			drawVal(int,int,float,float,float,float);

    bool			dumpXPM(std::ostream&) const;
};


namespace Interpolate { template <class T> class Applier2D; }


struct VDA2DBitMapGenPars : public A2DBitMapGenPars
{
			VDA2DBitMapGenPars()
			: lininterp_(false)	{}

    bool		lininterp_;	//!< Use bi-linear interpol, not poly

    static const char	cMinFill;	// => -120
    static const char	cMaxFill;	// => 120

    static float	offset(char);	//!< cMinFill -> 0, 0 -> 0.5

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitMap's. */

class VDA2DBitMapGenerator : public A2DBitMapGenerator
{
public:

			VDA2DBitMapGenerator(const A2DBitMapInpData&,
					     const A2DBitMapPosSetup&);

    VDA2DBitMapGenPars&		vdpars()	{ return gtPars(); }
    const VDA2DBitMapGenPars&	vdpars() const	{ return gtPars(); }

protected:

    inline VDA2DBitMapGenPars& gtPars() const
				{ return (VDA2DBitMapGenPars&)pars_; }

    float			strippixs_;

				VDA2DBitMapGenerator(
					const VDA2DBitMapGenerator&);
				    //!< Not implemented to prevent usage
				    //!< Copy the pars instead

    void			doFill();

    void			drawStrip(int);
    void			drawPixLines(int,const Interval<int>&);
    void			fillInterpPars(Interpolate::Applier2D<float>&,
					       int,int);
    void			drawVal(int,int,float);

    bool			dumpXPM(std::ostream&) const;
};


#endif
