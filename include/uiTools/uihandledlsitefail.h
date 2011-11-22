#ifndef uihandledlsitefail_h
#define uihandledlsitefail_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: uihandledlsitefail.h,v 1.1 2011-11-22 12:58:17 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstring.h"
class ODDLSite;
class BufferStringSet;
class uiComboBox;
class uiSlider;


/*!Presents Handles a download site failure. Presents options to user.

  If the user cancels (i.e. if ( !dlg.go() )), then the user wants to give up.
  If you have set isfatal, that means that the program will be exited!

  The list_of_sites should be alternatives (which may contain the ODDLSite).

 */


mClass uiHandleDLSiteFail : public uiDialog
{
public:

    			uiHandleDLSiteFail(uiParent*,const ODDLSite&,
				bool isfatal,
				const BufferStringSet* list_of_sites=0);

    const char*		site() const		{ return site_; }
    float		timeout() const;

protected:

    BufferString	site_;
    const bool		isfatal_;

    uiComboBox*		dlsitefld_;
    uiSlider*		timeoutfld_;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
