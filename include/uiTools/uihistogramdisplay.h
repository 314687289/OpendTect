#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uihistogramdisplay.h,v 1.9 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "datapack.h"

class uiTextItem;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }

mClass uiHistogramDisplay : public uiFunctionDisplay
{
public:

    				uiHistogramDisplay(uiParent*,Setup&,
						   bool withheader=false);
				~uiHistogramDisplay();

    bool			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);

    void			setHistogram(const TypeSet<float>&,
	    				     Interval<float>,int N=-1);

    const Stats::RunCalc<float>&	getRunCalc()	{ return rc_; }
    int                         nrInpVals() const       { return nrinpvals_; }
    int				nrClasses() const       { return nrclasses_; }
    void			putN(); 

protected:

    Stats::RunCalc<float>&	rc_;	
    int                         nrinpvals_;
    int                         nrclasses_;
    bool			withheader_;
    uiTextItem*			header_;
    uiTextItem*			nitm_;
    
    void			updateAndDraw();
    void			updateHistogram();
};


#endif
