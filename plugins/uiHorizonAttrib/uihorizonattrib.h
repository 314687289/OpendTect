#ifndef uihorizonattrib_h
#define uihorizonattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uihorizonattrib.h,v 1.6 2008-06-05 08:53:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class CtxtIOObj;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;


/*! \brief Horizon attribute description editor */

class uiHorizonAttrib : public uiAttrDescEd
{
public:

			uiHorizonAttrib(uiParent*,bool);
			~uiHorizonAttrib();

protected:

    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		typefld_;
    uiGenInput*		surfdatafld_;

    CtxtIOObj&		horctio_;
    BufferStringSet	surfdatanms_;
    int			nrouttypes_;

    virtual bool	setParameters(const Attrib::Desc&);
    virtual bool	setInput(const Attrib::Desc&);
    virtual bool	getParameters(Attrib::Desc&);
    virtual bool	getInput(Attrib::Desc&);

    void		horSel(CallBacker*);
    void		typeSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
