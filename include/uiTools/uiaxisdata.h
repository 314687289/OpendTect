#ifndef uiaxisdata_h
#define uiaxisdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          Jul 2008
 RCS:           $Id: uiaxisdata.h,v 1.4 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "statruncalc.h"

class DataClipper;
class uiGraphicsScene;

/*!\brief convenient base class to carry axis data:
  	# the AxisHandler which handles the behaviour and positioning of
	  an axis in a 2D plot;
	# axis scaling parameters
	# axis ranges
 */

mClass uiAxisData
{
public:

			uiAxisData(uiRect::Side);
			~uiAxisData();

    virtual void	stop();
    void		setRange( const Interval<float>& rg ) { rg_ = rg; }

    struct AutoScalePars
    {
			AutoScalePars();

	bool            doautoscale_;
	float           clipratio_;

	static float    defclipratio_;
	//!< 1) settings "AxisData.Clip Ratio"
	//!< 2) env "OD_DEFAULT_AXIS_CLIPRATIO"
	//!< 3) zero
    };

    uiAxisHandler*		axis_;
    AutoScalePars		autoscalepars_;
    Interval<float>		rg_;

    bool			needautoscale_;
    uiAxisHandler::Setup	defaxsu_;
    bool			isreset_;

    void			handleAutoScale(const Stats::RunCalc<float>&);
    void			handleAutoScale(const DataClipper&);
    void			newDevSize();
    void			renewAxis(const char*,uiGraphicsScene*,int w,
	    				  int h,const Interval<float>*);
};

#endif
