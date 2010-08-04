#ifndef uiexport2dhorizon_h
#define uiexport2dhorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
 RCS:           $Id: uiexport2dhorizon.h,v 1.5 2010-08-04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class SurfaceInfo;
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFileInput;


/*! \brief Dialog for 2D horizon export */

mClass uiExport2DHorizon : public uiDialog
{
public:
			uiExport2DHorizon(uiParent*,
					  const ObjectSet<SurfaceInfo>&);
			~uiExport2DHorizon();


protected:

    uiComboBox*		horselfld_;
    uiListBox*		linenmfld_;
    uiGenInput*		udffld_;
    uiFileInput*	outfld_;
    uiCheckBox*		wrlnmsbox_;
    uiCheckBox*		zbox_;

    const ObjectSet<SurfaceInfo>&	hinfos_;

    virtual bool	acceptOK(CallBacker*);
    void		horChg(CallBacker*);
    bool		doExport();
};


#endif
