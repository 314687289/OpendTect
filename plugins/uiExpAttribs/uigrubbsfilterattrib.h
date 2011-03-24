#ifndef uigrubbsfilterattrib_h
#define uigrubbsfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2011
 RCS:           $Id: uigrubbsfilterattrib.h,v 1.2 2011-03-24 03:00:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Semblance Attribute description editor */

mClass uiGrubbsFilterAttrib : public uiAttrDescEd
{
public:

			uiGrubbsFilterAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		grubbsvalfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		replacetype_;
    uiStepOutSel*	stepoutfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
