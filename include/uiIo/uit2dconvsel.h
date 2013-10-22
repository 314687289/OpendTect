#ifndef uit2dconvsel_h
#define uit2dconvsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"

class uiIOObjSel;
class uiGenInput;
class uiComboBox;
class uiT2DConvSelGroup;


/*! \brief single-line object for selecting T to depth conversion. */

mExpClass(uiIo) uiT2DConvSel : public uiGroup
{
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup(uiIOObjSel* tiedto,bool opt=true);

	mDefSetupMemb(BufferString,fldtext)
	mDefSetupMemb(bool,optional)
	mDefSetupMemb(uiIOObjSel*,tiedto)
	mDefSetupMemb(bool,ist2d)
    };

			uiT2DConvSel(uiParent*,const Setup&);

    bool		usePar(const IOPar&);
    bool		fillPar(IOPar&i,bool typeonly=false) const;

protected:

    Setup				setup_;

    uiComboBox*				choicefld_;
    ObjectSet<uiT2DConvSelGroup>	grps_;

    void				inpSel(CallBacker*);
    void				choiceSel(CallBacker*);
};


mExpClass(uiIo) uiT2DConvSelGroup : public uiGroup
{
public:
   			uiT2DConvSelGroup( uiParent* p, const char* gnm )
			    : uiGroup(p,gnm)	{}

      virtual bool	fillPar(IOPar&) const	= 0;
      virtual bool	usePar(const IOPar&)	= 0;

    mDefineFactory1ParamInClass(uiT2DConvSelGroup,uiParent*,factory);
};


mExpClass(uiIo) uiT2DLinConvSelGroup : public uiT2DConvSelGroup
{
public:
    				uiT2DLinConvSelGroup(uiParent*);

    static void			initClass();
    static uiT2DConvSelGroup*	create( uiParent* p )
    				{ return new uiT2DLinConvSelGroup(p); }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

    uiGenInput*		fld_;
};

#endif

