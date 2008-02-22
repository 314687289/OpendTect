#ifndef uiposprovgroupstd_h
#define uiposprovgroupstd_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroupstd.h,v 1.8 2008-02-22 09:31:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposprovgroup.h"
class MultiID;
class CtxtIOObj;
class CubeSampling;
class uiGenInput;
class uiIOObjSel;
class uiSelSteps;
class uiSelHRange;
class uiSelZRange;
class uiSelNrRange;
class uiIOFileSelect;


/*! \brief UI for RangePosProvider */

class uiRangePosProvGroup : public uiPosProvGroup
{
public:

			uiRangePosProvGroup(uiParent*,
					    const uiPosProvGroup::Setup&,
					    bool wstep=true);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    void		setExtractionDefaults();

    void		getCubeSampling(CubeSampling&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSelHRange*	hrgfld_;
    uiSelNrRange*	nrrgfld_;
    uiSelZRange*	zrgfld_;

};


/*! \brief UI for PolyPosProvider */

class uiPolyPosProvGroup : public uiPosProvGroup
{
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiPolyPosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    void		setExtractionDefaults();

    bool		getID(MultiID&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio_;

    uiIOObjSel*		polyfld_;
    uiSelSteps*		stepfld_;
    uiSelZRange*	zrgfld_;

};


/*! \brief UI for TablePosProvider */

class uiTablePosProvGroup : public uiPosProvGroup
{
public:
			uiTablePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    bool		getID(MultiID&) const;
    bool		getFileName(BufferString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiTablePosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio_;

    uiGenInput*		selfld_;
    uiIOObjSel*		psfld_;
    uiIOFileSelect*	tffld_;

    void		selChg(CallBacker*);

};


#endif
