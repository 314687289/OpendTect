#ifndef uisegyresortdlg_h
#define uisegyresortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
 RCS:           $Id: uisegyresortdlg.h,v 1.4 2011-04-13 10:44:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiIOObjSel;
class uiGenInput;
class uiPosSubSel;
class uiFileInput;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mClass uiResortSEGYDlg : public uiDialog
{
public :

			uiResortSEGYDlg(uiParent*);

    Seis::GeomType	geomType() const;

protected:

    uiGenInput*		geomfld_;
    uiIOObjSel*		volfld_;
    uiIOObjSel*		ps3dfld_;
    uiIOObjSel*		ps2dfld_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		newinleachfld_;
    uiGenInput*		inlnmsfld_;
    uiFileInput*	outfld_;

    uiIOObjSel*		objSel();

    void		inpSel(CallBacker*);
    void		geomSel(CallBacker*);
    void		nrinlSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
