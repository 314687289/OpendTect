#ifndef uiclusterjobprov_h
#define uiclusterjobprov_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "multiid.h"

class InlineSplitJobDescProv;
class uiGenInput;
class uiFileInput;
class uiLabel;


mExpClass(uiIo) uiClusterJobProv : public uiDialog
{
public:
    			uiClusterJobProv(uiParent* p,const IOPar& iop,
					 const char* prog,const char* parfnm);
			~uiClusterJobProv();

    static const char*	sKeySeisOutIDKey();
    static const char*	sKeyOutputID();

protected:

    InlineSplitJobDescProv*	jobprov_;
    IOPar&		iopar_;
    const char*		prognm_;
    BufferString	tempstordir_;

    uiGenInput*		nrinlfld_;
    uiLabel*		nrjobsfld_;
    uiFileInput*	parfilefld_;
    uiFileInput*	tmpstordirfld_;
    uiFileInput*	scriptdirfld_;
    uiGenInput*		cmdfld_;

    void		nrJobsCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		createJobScripts(const char*);
    const char*		getOutPutIDKey() const;
    MultiID		getTmpID(const char*) const;
};

#endif

