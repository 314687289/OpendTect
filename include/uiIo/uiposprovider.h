#ifndef uiposprovider_h
#define uiposprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovider.h,v 1.12 2008-12-02 13:58:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uiposprovgroup.h"
#include "iopar.h"
class uiGenInput;
namespace Pos { class Provider; }
class uiPosProvGroup;
class CubeSampling;

/*! \brief lets user choose a way to provide positions */

class uiPosProvider : public uiGroup
{
public:

    struct Setup : public uiPosProvGroup::Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges };

			Setup( bool is_2d, bool with_step, bool with_z )
			    : uiPosProvGroup::Setup(is_2d,with_step,with_z)
			    , seltxt_("Positions")
			    , allownone_(false)
			    , choicetype_(OnlyRanges)	{}
	virtual	~Setup()				{}
	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(ChoiceType,choicetype)
	mDefSetupMemb(bool,allownone)
    };

    			uiPosProvider(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;
    void		setExtractionDefaults();

    Pos::Provider*	createProvider() const;

    bool		is2D() const		{ return setup_.is2d_; }
    bool		isAll() const		{ return !curGrp(); }

protected:

    uiGenInput*			selfld_;
    ObjectSet<uiPosProvGroup>	grps_;
    Setup			setup_;

    void			selChg(CallBacker*);
    uiPosProvGroup*		curGrp() const;
};


/*!\brief CompoundParSel to capture a user's Pos::Provider wishes */


class uiPosProvSel : public uiCompoundParSel
{
public:

    typedef uiPosProvider::Setup Setup;

    			uiPosProvSel(uiParent*,const Setup&);
    			~uiPosProvSel();

    void		usePar(const IOPar&);
    void		fillPar( IOPar& iop ) const	{ iop.merge(iop_); }

    Pos::Provider*	curProvider()			{ return prov_; }
    const Pos::Provider* curProvider() const		{ return prov_; }

    const CubeSampling&	envelope() const;
    void		setInput(const CubeSampling&,bool chgtype=true);
    void		setInputLimit(const CubeSampling&);

    bool		isAll() const;
    void		setToAll();

protected:

    Setup		setup_;
    IOPar		iop_;
    Pos::Provider*	prov_;
    mutable CubeSampling& cs_;

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;
    void		setCSToAll() const;
    void		setProvFromCS();
    void		mkNewProv(bool updsumm=true);

};


#endif
