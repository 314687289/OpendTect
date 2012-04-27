#ifndef uifreqfilter_h
#define uifreqfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2012
 RCS:           $Id: uifreqfilter.h,v 1.3 2012-04-27 09:44:03 cvsbruno Exp $
________________________________________________________________________

-*/

#include "fftfilter.h"
#include "uigroup.h"


class uiGenInput;

mClass uiFreqFilterSelFreq : public uiGroup
{
public:
    			uiFreqFilterSelFreq(uiParent*);

    const Interval<float>& freqRange() const 		{ return freqrg_; }
    FFTFilter::Type	filterType() const 		{ return filtertype_; }

    void 		setFreqRange(Interval<float> rg) 
    			{ freqrg_ = rg; putToScreen(); }
    void 		setFilterType(FFTFilter::Type tp) 
    			{ filtertype_ = tp; putToScreen(); }
    void		setMinFreq(float f) { freqrg_.start = f; putToScreen();}
    void		setMaxFreq(float f) { freqrg_.stop = f; putToScreen(); }

    Notifier<uiFreqFilterSelFreq> parchanged;

protected:
    uiGenInput*		typefld_;
    uiGenInput*		freqfld_;

    FFTFilter::Type 	filtertype_;
    Interval<float>	freqrg_;

    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    void		parChgCB(CallBacker*);
    virtual void	typeSel(CallBacker*);
};


#endif
