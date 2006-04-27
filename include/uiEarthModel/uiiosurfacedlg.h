#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.12 2006-04-27 15:29:13 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class uiGenInput;
class uiIOObjSel;
class uiSurfaceRead;
class uiSurfaceWrite;

namespace EM { class Surface; class SurfaceIODataSelection; class Horizon; };


/*! \brief Dialog for horizon export */

class uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Surface&);

    void		getSelection(EM::SurfaceIODataSelection&);
    IOObj*		ioObj() const;

protected:
    uiSurfaceWrite*	iogrp_;
    const EM::Surface&	surface_;

    bool		acceptOK(CallBacker*);
};


class uiReadSurfaceDlg : public uiDialog
{
public:
			uiReadSurfaceDlg(uiParent*,bool ishor);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:
    uiSurfaceRead*	iogrp_;
    bool		acceptOK(CallBacker*);
};


class uiStoreAuxData : public uiDialog
{
public:
    			uiStoreAuxData(uiParent*,const EM::Horizon&);

    bool		doOverWrite() const	{ return dooverwrite_; }

protected:
    uiGenInput*		attrnmfld_;
    const EM::Horizon&	surface_;

    bool		dooverwrite_;
    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};


class uiCopySurface : public uiDialog
{
public:
    			uiCopySurface(uiParent*,const IOObj&);
			~uiCopySurface();

protected:
    uiSurfaceRead*	inpfld;
    uiIOObjSel*		outfld;

    CtxtIOObj&		ctio_;
    
    CtxtIOObj&		mkCtxtIOObj(const IOObj&);
    bool		acceptOK(CallBacker*);
};

#endif
