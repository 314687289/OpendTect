#ifndef uieventattrib_h
#define uieventattrib_h

/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2005
 RCS:           $Id: uieventattrib.h,v 1.5 2006-12-20 11:23:00 cvshelene Exp $
 ________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;

/*! \brief Event Attributes description editor */

class uiEventAttrib : public uiAttrDescEd
{
public:

                        uiEventAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld;
    uiGenInput*         issinglefld;
    uiGenInput*         tonextfld;
    uiGenInput*		outpfld;
    uiGenInput*		evtypefld;
    uiGenInput*		gatefld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		isSingleSel(CallBacker*);
    void                isGateSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
