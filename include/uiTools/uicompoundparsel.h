#ifndef uicompoundparsel_h
#define uicompoundparsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2006
 RCS:           $Id: uicompoundparsel.h,v 1.3 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiGenInput;
class uiPushButton;


/*!\brief Single-line element allowing multi-parameter to be set via a dialog.
  
  Most useful for options that are not often actually changed by user.
  After the button push trigger, the summary is displayed in the text field.
 
 */

mClass uiCompoundParSel : public uiGroup
{
public:

    			uiCompoundParSel(uiParent*,const char* seltxt,
					 const char* buttxt=0);


    void		setSelText(const char*);
    void		updateSummary()			{ updSummary(0); }

    Notifier<uiCompoundParSel>	butPush;

protected:

    virtual BufferString	getSummary() const	= 0;
    void			doSel(CallBacker*);
    void			updSummary(CallBacker*);

    uiGenInput*			txtfld_;
    uiPushButton*		selbut_;

};


#endif
