#ifndef array2dbitmap_h
#define array2dbitmap_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2006
 RCS:           $Id: array2dbitmap.h,v 1.11 2007-02-15 13:24:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "sets.h"
#include "arraynd.h"


typedef Array2D<char>	A2DBitMap;
#define A2DBitMapImpl	Array2DImpl<char>


/*! \brief Array2D Bitmap generation parameters */

struct A2DBitMapGenPars
{
		A2DBitMapGenPars()
		  : nointerpol_(false)
		  , autoscale_(true)
		  , fliplr_(false)
		  , fliptb_(false)
		  , clipratio_(0.025)
		  , scale_(0,1)			{}

    bool	nointerpol_;	//!< Do not interpolate between actual samples
    bool	autoscale_;	//!< If not, use the scale_
    bool	fliplr_;	//!< Flip left/right
    bool	fliptb_;	//!< Flip top/bottom
    float	clipratio_;	//!< ratio of numbers to be clipped before
    				//!< determining min/max, only when autoscale_
    Interval<float> scale_;	//!< Used when autoscale_ is false

    static const char cNoFill;	//!< -127, = background/transparent

};


/*! \brief Array2D<float>& + statistics */

class A2DBitMapInpData
{
public:

    			A2DBitMapInpData( const Array2D<float>& d )
			    : data_(d)	{ collectData(); }

    const Array2D<float>& data() const	{ return data_; }
    int			nrPts() const	{ return statpts_.size(); }
    Interval<float>	scale(float clipratio) const;
    virtual float	midVal() const
					{ return statpts_[nrPts()/2]; }

    void		collectData(); //!< again.

protected:

    const Array2D<float>&	data_;
    TypeSet<float>		statpts_;

    void		selectData();

};


/*! \brief Array2D Bitmap generation setup

This class allows 'zooming' into the data, and an irregularly positioned
first dimension.

If this class wouldn't exist, both dimensions of the Array2D have to have
100% regular positioning. But if for example the data contains samples
of a 2-D seismic line with gaps, this is not the case. That is why you can add
positioning in one dimension. The second dimension is assumed to be regular,
step 1, starting at 0.

Thus, the first dimension may be irregularly sampled. For the first dimension,
you can set up the axis by providing the positions in a float array. If you
don't provide that array, one will be generated, the postions are assumed to
be: 0 1 2 ..., which is the same as for the second dimension (which can never
be irregular).

Then, you can zoom in by setting the different ranges. The default ranges will
be -0.5 to N-0.5, i.e. half a distance between the cols/rows is added on all
sides as border.

The positions in dim 0 *must* be sorted, *ascending*. Only the distances may
vary. The average distance between the positions is used to calculate the
default border.

Dim 0 <-> X ...  left-to-right
Dim 1 <-> Y ...  Top to bottom

This classs is _not_ intended to support direct world coordinates and that
kind of things. It just enables most simple bitmap generations. You may still
need a (usually linear) transformation in both directions for display.

*/


class A2DBitMapPosSetup
{
public:

			A2DBitMapPosSetup(const Array2DInfo&,float* dim0pos=0);
				// If passed, dim0pos becomes mine
    virtual		~A2DBitMapPosSetup();

    void		setPositions(float* dim0positions);
				//!< dim0posistions will become mine
    inline const float*	dim0Positions() const
    			{ return dim0pos_; }
    int			dimSize( int dim ) const
    			{ return dim ? szdim1_ : szdim0_; }

    void		setDimRange( int dim, const Interval<float>& r )
    			{ (dim ? dim1rg_ : dim0rg_) = r; }	//!< 'zooming'
    inline const Interval<float>& dimRange( int dim ) const
    			{ return dim ? dim1rg_ : dim0rg_; }
    inline float	avgDist( int dim ) const
    			{ return dim ? 1 : dim0avgdist_; }
    inline float	dimEps( int dim ) const
			{ return 1e-6 * avgDist(dim); }

    void		setBitMapSizes(int,int) const;
    inline int		nrXPix() const		{ return nrxpix_; }
    inline int		nrYPix() const		{ return nrypix_; }

    inline float	getPixPerDim( int dim ) const
			{ return dim ? pixperdim1_ : pixperdim0_; }
    inline float	getPixOffs( int dim, float pos, bool rev ) const
			{ return rev ?
			        ((dim ? dim1rg_ : dim0rg_).stop - pos)
				 * getPixPerDim( dim )
			       : (pos - (dim ? dim1rg_ : dim0rg_).start)
				 * getPixPerDim( dim ); }

    int			getPix(int dim,float,bool rev) const;
    			// Nr of pixels from (0,0), always inside bitmap
    bool		isInside(int dim,float) const;
    			// Is position in dim inside bitmap?

protected:


    float*		dim0pos_;
    int			szdim0_;
    int			szdim1_;

    Interval<float>	dim0rg_;
    Interval<float>	dim1rg_;

    float		dim0avgdist_;

    // Vars for current bitmap
    int			nrxpix_;
    int			nrypix_;
    float		pixperdim0_;
    float		pixperdim1_;

};


/*!\brief Generates Array2D bitmap from Array2D<float> */

class A2DBitMapGenerator
{
public:

    virtual		~A2DBitMapGenerator()	{ delete &pars_; }

    void		setBitMap(A2DBitMap&);
    A2DBitMap&		bitmap()		{ return *bitmap_; }
    const A2DBitMap&	bitmap() const		{ return *bitmap_; }
    int			bitmapSize(int dim) const;

    static void		initBitMap(A2DBitMap&);	//!< with cNoFill
    void		initBitMap()
    			{ if ( bitmap_ ) initBitMap( *bitmap_ ); }

    void		fill();
    bool		dump(std::ostream&) const;

    A2DBitMapGenPars&		pars()		{ return pars_; }
    const A2DBitMapGenPars&	pars() const	{ return pars_; }
    const A2DBitMapInpData&	data() const	{ return data_; }
    const A2DBitMapPosSetup&	setup() const	{ return setup_; }

protected:

				A2DBitMapGenerator(const A2DBitMapInpData&,
						   const A2DBitMapPosSetup&,
						   A2DBitMapGenPars&);
				   //!< pass a new instance of (subclass of)
				   //!< A2DBitMapGenPars

    const A2DBitMapInpData&	data_;
    const A2DBitMapPosSetup&	setup_;
    A2DBitMapGenPars&		pars_;
    A2DBitMap*			bitmap_;

				// handy, duplicated from respective objects
    int				szdim0_;
    int				szdim1_;
    const float*		dim0pos_;
    Interval<float>		dim0rg_;
    Interval<float>		dim1rg_;
    float			dim0perpix_;
    float			dim1perpix_;
    Interval<float>		scalerg_;
    float			scalewidth_;

    virtual void		doFill()		= 0;
    virtual bool		dumpXPM(std::ostream&) const { return false; }

};


#endif
