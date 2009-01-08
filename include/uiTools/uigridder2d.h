#ifndef uigridder2d_h
#define uigridder2d_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2008
 RCS:           $Id: uigridder2d.h,v 1.2 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "factory.h"

class Gridder2D;
class InverseDistanceGridder2D;
class uiGenInput;


mClass uiGridder2DSel : public uiDlgGroup
{
public:
    				uiGridder2DSel(uiParent*,const Gridder2D*);
    				~uiGridder2DSel();

    const Gridder2D*		getSel();

protected:
    void			selChangeCB(CallBacker*);
    const Gridder2D*		original_;
    uiGenInput*			griddingsel_;

    ObjectSet<uiDlgGroup>	griddingparams_;
    ObjectSet<Gridder2D>	gridders_;
};

mClass uiInverseDistanceGridder2D : public uiDlgGroup
{
public:
    static void		initClass();
    static uiDlgGroup*	create(uiParent*,Gridder2D*);

    			uiInverseDistanceGridder2D(uiParent*,
					InverseDistanceGridder2D&);

    bool		acceptOK();
    bool		rejectOK();
    bool		revertChanges();

    const char*		errMsg() const;

protected:

    uiGenInput*			searchradiusfld_;
    const float			initialsearchradius_;

    InverseDistanceGridder2D&	idg_;
};


mDefineFactory2Param( uiDlgGroup, uiParent*, Gridder2D*, uiGridder2DFact );


#endif
