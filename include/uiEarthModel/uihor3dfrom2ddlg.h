#ifndef uihor3dfrom2ddlg_h
#define uihor3dfrom2ddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.h,v 1.8 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "multiid.h"

#include "uidialog.h"

class uiArray2DInterpolSel;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class uiEMPartServer;
namespace EM { class Horizon2D; class Horizon3D; };


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

mClass uiHor3DFrom2DDlg : public uiDialog
{
public:    
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&,
						 uiEMPartServer*);
				~uiHor3DFrom2DDlg();

    bool			doDisplay() const;
    MultiID			getSelID() const;
    EM::Horizon3D* 		getHor3D() { return hor3d_; }

protected:

    bool			acceptOK(CallBacker*);


    const EM::Horizon2D& 	hor2d_;
    EM::Horizon3D* 		hor3d_;
    uiEMPartServer*		emserv_;

    uiArray2DInterpolSel*	interpolsel_;

    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_;

    MultiID			selid_;

    void			typChg(CallBacker*);
};

#endif
