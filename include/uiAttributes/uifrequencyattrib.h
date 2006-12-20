#ifndef uifrequencyattrib_h
#define uifrequencyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifrequencyattrib.h,v 1.5 2006-12-20 11:23:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiGenInput;
class uiImagAttrSel;

/*! \brief Frequency Attribute description editor */

class uiFrequencyAttrib : public uiAttrDescEd
{
public:

			uiFrequencyAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*         gatefld;
    uiGenInput*		normfld;
    uiGenInput*		winfld;
    uiGenInput*		outpfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
