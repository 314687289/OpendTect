#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;
namespace Math { class Expression; }

/*! \brief Math Attribute description editor */

mExpClass(uiAttributes) uiMathAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiMathAttrib);
public:

			uiMathAttrib(uiParent*,bool);

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiGenInput*         inpfld_;
    uiPushButton*	parsebut_;
    uiTable*            xtable_;
    uiTable*            ctable_;
    ObjectSet<uiAttrSel>  attribflds_;
    uiGenInput*		recstartfld_;
    uiGenInput*		recstartposfld_;

    int			nrvars_;
    int			nrcsts_;
    int			nrspecs_;
    BufferStringSet	varnms;
    BufferStringSet	cstnms;

    void		parsePush(CallBacker*);
    void		updateDisplay(bool);
    void		getVarsNrAndNms(Math::Expression*);
    void		setupOneRow(const uiAttrSelData&,int,bool);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);


			mDeclReqAttribUIFns
};

#endif

