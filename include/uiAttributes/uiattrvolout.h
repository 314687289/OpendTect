#ifndef uiattrvolout_h
#define uiattrvolout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uibatchlaunch.h"
#include "multiid.h"

namespace Attrib { class CurrentSel; class DescID; class DescSet; };

class CtxtIOObj;
class IOObj;
class IOPar;
class NLAModel;

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiMultiAttribSel;
class uiSeisSel;
class uiSeisTransfer;


/*! \brief Dialog for creating volume output */

mExpClass(uiAttributes) uiAttrVolOut : public uiFullBatchDialog
{
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     bool multioutput,
				     const NLAModel*,const MultiID& nlaid);
			~uiAttrVolOut();

    const IOPar&	subSelPar() const		{ return subselpar_; }
    const Attrib::CurrentSel& outputSelection() const	{ return sel_; }

    static const char*  sKeyMaxCrlRg();
    static const char*  sKeyMaxInlRg();

protected:

    CtxtIOObj&		ctio_;
    Attrib::CurrentSel&	sel_;
    IOPar&		subselpar_;
    Attrib::DescSet& 	ads_;
    MultiID		nlaid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		todofld_;
    uiMultiAttribSel*	attrselfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		objfld_;
    uiIOObjSel*		datastorefld_;
    uiGenInput*		offsetfld_;

    TypeSet<int>	seloutputs_;
    BufferStringSet	seloutnms_;

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    void		attrSel(CallBacker*);
    void		singLineSel(CallBacker*);
    void		psSelCB(CallBacker*);
    void		addNLA(Attrib::DescID&);

private:

    CtxtIOObj&		mkCtxtIOObj(const Attrib::DescSet&);

};

#endif

