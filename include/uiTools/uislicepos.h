#ifndef uislicepos_h
#define uislicepos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
 RCS:           $Id: uislicepos.h,v 1.9 2011-04-04 15:13:09 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "bufstringset.h"
#include "cubesampling.h"

class uiLabel;
class uiSpinBox;
class uiToolBar;
class uiToolButton;

/*! \brief Toolbar for setting slice position _ base class */

mClass uiSlicePos : public CallBacker
{
public:		
			uiSlicePos(uiParent*);
			~uiSlicePos();

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    CubeSampling	getCubeSampling() const		{ return curcs_; }

    void		setLabels(const char* inl,const char* crl,const char*z);

    enum Orientation            { Inline=0, Crossline=1, Zslice=2 };
				DeclareEnumUtils(Orientation);

    Notifier<uiSlicePos> positionChg;

protected:

    uiToolBar*		toolbar_;
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;
    int			laststeps_[3];
    float		zfactor_;
    CubeSampling	curcs_;
    BufferStringSet	boxlabels_;

    void		setBoxLabel(Orientation);
    virtual void	setBoxRanges()			=0;
    virtual void	setPosBoxValue()		=0;
    virtual void	setStepBoxValue()		=0;
    virtual void	slicePosChg(CallBacker*)	=0;
    virtual void	sliceStepChg(CallBacker*)	=0;
    void		updatePos(CallBacker*);
    void		initSteps(CallBacker* cb=0);
    void		slicePosChanged(Orientation,const CubeSampling&);
    void		sliceStepChanged(Orientation);
    void		setBoxRg(Orientation,const CubeSampling&);
    void		setPosBoxVal(Orientation,const CubeSampling&);

    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);
};

#endif
