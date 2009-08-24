#ifndef uicalcpoly2horvol_h
#define uicalcpoly2horvol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
 RCS:           $Id: uicalcpoly2horvol.h,v 1.3 2009-08-24 09:41:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiIOObjSel;
class uiGenInput;
class uiCheckBox;
namespace Pick	{ class Set; }
namespace EM	{ class Horizon3D; }


/*! \brief UI for calculation of volume at horizons */

mClass uiCalcHorVol : public uiDialog
{
protected:

			uiCalcHorVol(uiParent*,const char*);

    uiCheckBox*		upwbox_;
    uiCheckBox*		ignnegbox_;
    uiGenInput*		velfld_;
    uiGenInput*		valfld_;

    uiGroup*		mkStdGrp();

    const bool		zinft_;
    
    void		haveChg(CallBacker*);
    void		calcReq(CallBacker*);

    virtual const Pick::Set*		getPickSet()		= 0;
    virtual const EM::Horizon3D*	getHorizon()		= 0;

};


/*! \brief using polygon to calculate to different horizons */

mClass uiCalcPolyHorVol : public uiCalcHorVol
{
public:

			uiCalcPolyHorVol(uiParent*,const Pick::Set&);
			~uiCalcPolyHorVol();

    const Pick::Set&	pickSet()		{ return ps_; }

protected:

    uiIOObjSel*		horsel_;

    const Pick::Set&	ps_;
    EM::Horizon3D*	hor_;
    
    const Pick::Set*		getPickSet()	{ return &ps_; }
    const EM::Horizon3D*	getHorizon();

    void		horSel(CallBacker*);

};


/*! \brief using horizon to calculate from different levels by polygon */

mClass uiCalcHorPolyVol : public uiCalcHorVol
{
public:

			uiCalcHorPolyVol(uiParent*,const EM::Horizon3D&);
			~uiCalcHorPolyVol();

    const EM::Horizon3D& horizon()		{ return hor_; }

protected:

    uiIOObjSel*		pssel_;

    Pick::Set*		ps_;
    const EM::Horizon3D& hor_;
    
    const Pick::Set*		getPickSet();
    const EM::Horizon3D*	getHorizon()	{ return &hor_; }

    void		psSel(CallBacker*);

};


#endif
