#ifndef uiinstantattrib_h
#define uiinstantattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.h,v 1.7 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;

/*! \brief Instantaneous Attribute description editor */

mClass uiInstantaneousAttrib : public uiAttrDescEd
{
public:

			uiInstantaneousAttrib(uiParent*,bool);

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		outpfld;

    static const char*	outstrs[];

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
