#ifndef uienergyattrib_h
#define uienergyattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uienergyattrib.h,v 1.9 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;

/*! \brief Energy Attribute ui */

mClass uiEnergyAttrib : public uiAttrDescEd
{
public:

			uiEnergyAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		gradientfld_;
    uiGenInput*         outpfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool                setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool                getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
