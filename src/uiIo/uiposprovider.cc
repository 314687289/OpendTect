/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/


#include "uiposprovider.h"
#include "uipossubsel.h"
#include "rangeposprovider.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uilabel.h"
#include "uidialog.h"
#include "uitoolbutton.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "survinfo.h"
#include "od_helpids.h"


uiPosProvider::uiPosProvider( uiParent* p, const uiPosProvider::Setup& su )
	: uiGroup(p,"uiPosProvider")
	, setup_(su)
	, selfld_(0)
	, fullsurvbut_(0)
{
    const BufferStringSet& factnms( setup_.is2d_
	    ? Pos::Provider2D::factory().getNames()
	    : Pos::Provider3D::factory().getNames() );

    const uiStringSet& factusrnms( setup_.is2d_
	   ? Pos::Provider2D::factory().getUserNames()
	   : Pos::Provider3D::factory().getUserNames() );

    uiStringSet nms;
    BufferStringSet reqnms;
    if ( setup_.choicetype_ != Setup::All )
    {
	reqnms.add( sKey::Range() );
	if ( setup_.choicetype_ == Setup::OnlySeisTypes )
	{
	    reqnms.add( sKey::Table() );
	    reqnms.add( sKey::Polygon() );
	}
	else if ( setup_.choicetype_ == Setup::VolumeTypes )
	{
	    reqnms.add( sKey::Well() );
	    reqnms.add( sKey::Polygon() );
	    reqnms.add( sKey::Body() );
	}
	else if ( setup_.choicetype_ == Setup::RangewithPolygon )
	    reqnms.add( sKey::Polygon() );
    }

    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const OD::String& nm( factnms.get(idx) );
	if ( !reqnms.isEmpty() && !reqnms.isPresent(nm) )
	    continue;

	uiPosProvGroup* grp = uiPosProvGroup::factory()
				.create(nm,this,setup_,true);
	if ( !grp ) continue;

	nms.add( factusrnms[idx] );
	grp->setName( nm );
	grps_ += grp;
    }
    if ( setup_.allownone_ )
	nms.add( uiStrings::sAll() );

    const CallBack selcb( mCB(this,uiPosProvider,selChg) );
    if ( grps_.size() == 0 )
    {
	new uiLabel( this, tr("No position providers available") );
	return;
    }

    if ( nms.size() > 1 )
    {
	selfld_ = new uiGenInput( this, setup_.seltxt_, StringListInpSpec(nms));
	for ( int idx=0; idx<grps_.size(); idx++ )
	    grps_[idx]->attach( alignedBelow, selfld_ );
	selfld_->valuechanged.notify( selcb );
	if ( !setup_.is2d_ )
	{
	    fullsurvbut_ = new uiToolButton( this, "exttofullsurv",
				tr("Set ranges to work area"),
				 mCB(this,uiPosProvider,fullSurvPush) );
	    fullsurvbut_->attach( rightOf, selfld_ );
	}
    }

    setHAlignObj( grps_[0] );
    postFinalise().notify( selcb );
}


void uiPosProvider::selChg( CallBacker* )
{
    if ( !selfld_ ) return;
    const int selidx = selfld_->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx );

    if ( fullsurvbut_ )
	fullsurvbut_->display( BufferString(selfld_->text()) == sKey::Range() );
}


void uiPosProvider::fullSurvPush( CallBacker* )
{
    if ( !selfld_ ) return;
    const int selidx = selfld_->getIntValue();
    if ( selidx < 0 ) return;

    IOPar iop;
    SI().sampling( true ).fillPar( iop );
    grps_[selidx]->usePar( iop );
}


uiPosProvGroup* uiPosProvider::curGrp() const
{
    if ( grps_.size() < 1 ) return 0;
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    return const_cast<uiPosProvGroup*>(
	    selidx < grps_.size() ? grps_[selidx] : 0);
}


void uiPosProvider::usePar( const IOPar& iop )
{
    BufferString typ;
    iop.get( sKey::Type(), typ );
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( typ == grps_[idx]->name() )
	{
	    grps_[idx]->usePar( iop );
	    if ( selfld_ )
		selfld_->setValue( idx );
	    return;
	}
    }

    if ( selfld_ )
	selfld_->setValue( ((int)0) );
}


bool uiPosProvider::fillPar( IOPar& iop ) const
{
    if ( grps_.size() < 1 )
	return false;

    if ( !isAll() )
    {
	const uiPosProvGroup* curgrp = curGrp();
	return curgrp ? curgrp->fillPar(iop) : true;
    }

    iop.set( sKey::Type(), sKey::None() );
    return true;
}


void uiPosProvider::setExtractionDefaults()
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->setExtractionDefaults();
}


Pos::Provider* uiPosProvider::createProvider() const
{
    IOPar iop;
    if ( !fillPar(iop) )
	return 0;

    if ( setup_.is2d_ )
	return Pos::Provider2D::make( iop );
    else
	return Pos::Provider3D::make( iop );
}


uiPosProvSel::uiPosProvSel( uiParent* p, const uiPosProvSel::Setup& su )
    : uiCompoundParSel(p,su.seltxt_)
    , setup_(su)
    , prov_(0)
    , tkzs_(*new TrcKeyZSampling(false))
{
    txtfld_->setElemSzPol( uiObject::WideVar );
    iop_.set( sKey::Type(), sKey::None() );
    mkNewProv(false);
    butPush.notify( mCB(this,uiPosProvSel,doDlg) );
}


uiPosProvSel::~uiPosProvSel()
{
    delete prov_;
    delete &tkzs_;
}


BufferString uiPosProvSel::getSummary() const
{
    BufferString ret;
    if ( !prov_ )
	ret = "-";
    else
    {
	ret = prov_->type(); ret[1] = '\0'; ret += ": ";
	prov_->getSummary( ret );
    }
    return ret;
}


void uiPosProvSel::setCSToAll() const
{
    if ( setup_.is2d_ )
	tkzs_.set2DDef();
    else
	tkzs_ = SI().sampling( true );
}


void uiPosProvSel::setProvFromCS()
{
    delete prov_;
    if ( setup_.is2d_ )
    {
	Pos::RangeProvider2D* rp2d = new Pos::RangeProvider2D;
	rp2d->setTrcRange( tkzs_.hsamp_.crlRange(), 0 );
	rp2d->setZRange( tkzs_.zsamp_, 0 );
	prov_ = rp2d;
    }
    else
    {
	Pos::RangeProvider3D* rp3d = new Pos::RangeProvider3D;
	rp3d->setSampling( tkzs_ );
	prov_ = rp3d;
    }
    prov_->fillPar( iop_ );
    iop_.set( sKey::Type(), prov_->type() );
    updateSummary();
}


const TrcKeyZSampling& uiPosProvSel::envelope() const
{
    return tkzs_;
}


void uiPosProvSel::mkNewProv( bool updsumm )
{
    delete prov_;
    if ( setup_.is2d_ )
	prov_ = Pos::Provider2D::make( iop_ );
    else
	prov_ = Pos::Provider3D::make( iop_ );

    if ( prov_ )
    {
	prov_->getTrcKeyZSampling( tkzs_ );
	if ( !setup_.is2d_ ) //set step for 3D cs
	{
	    TrcKeyZSampling tmpcs;
	    tmpcs.usePar( iop_ );
	    tkzs_.hsamp_.step_ = tmpcs.hsamp_.step_;
	}
    }
    else
    {
	setCSToAll();
	if ( !setup_.allownone_ )
	    { setProvFromCS(); return; }
    }

    if ( updsumm )
	updateSummary();
}


void uiPosProvSel::setInput( const TrcKeyZSampling& cs, bool chgtyp )
{
    if ( chgtyp || (prov_ && sKey::Range()==prov_->type()) )
    {
	tkzs_ = cs;
	setProvFromCS();
    }
}


void uiPosProvSel::setInput( const TrcKeyZSampling& initcs,
			     const TrcKeyZSampling& ioparcs )
{
    setInput( initcs );
    ioparcs.fillPar( iop_ );
}


void uiPosProvSel::setInputLimit( const TrcKeyZSampling& cs )
{
    setup_.tkzs_ = cs;
}


bool uiPosProvSel::isAll() const
{
    if ( setup_.allownone_ )
	return !prov_;

    TrcKeyZSampling cskp = tkzs_;
    setCSToAll();
    const bool ret = tkzs_ == cskp;
    tkzs_ = cskp;
    return ret;
}


void uiPosProvSel::setToAll()
{
    if ( !setup_.allownone_ )
    {
	iop_.set( sKey::Type(), sKey::None() );
	mkNewProv( true );
    }
    else
    {
	setCSToAll();
	setProvFromCS();
    }
}


void uiPosProvSel::doDlg( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(uiStrings::sPosition(mPlural),
			   uiStrings::phrSpecify(uiStrings::sPosition(mPlural)),
			   mODHelpKey(mPosProvSelHelpID) ) );
    uiPosProvider* pp = new uiPosProvider( &dlg, setup_ );
    pp->usePar( iop_ );
    if ( dlg.go() )
    {
	iop_.setEmpty();
	pp->fillPar( iop_ );
	mkNewProv();
    }
}


void uiPosProvSel::usePar( const IOPar& iop )
{
    IOPar orgiop( iop_ );
    iop_.merge( iop );
    mkNewProv();
    iop_ = orgiop;
    if ( prov_ )
    {
	prov_->fillPar( iop_ );
	iop_.set(sKey::Type(),prov_->type());
    }
}


uiPosSubSel::uiPosSubSel( uiParent* p, const uiPosSubSel::Setup& su )
    : uiGroup(p,"uiPosSubSel")
    , selChange(this)
{
    uiPosProvider::Setup ppsu( su.is2d_, su.withstep_, su.withz_ );
    ppsu.seltxt( su.seltxt_ )
	.allownone( true )
	.choicetype( (uiPosProvider::Setup::ChoiceType)su.choicetype_ );
    ppsu.zdomkey( su.zdomkey_ );
    ps_ = new uiPosProvSel( this, ppsu );
    ps_->butPush.notify( mCB(this,uiPosSubSel,selChg) );
    setHAlignObj( ps_ );
}


void uiPosSubSel::selChg( CallBacker* )
{
    selChange.trigger();
}


#define mDefFn(ret,nm,typ,arg,cnst,retstmt) \
ret uiPosSubSel::nm( typ arg ) cnst \
{ \
    retstmt ps_->nm( arg ); \
}
#define mDefFn2(ret,nm,typ1,arg1,typ2,arg2,cnst) \
ret uiPosSubSel::nm( typ1 arg1, typ2 arg2 ) cnst \
{ \
    ps_->nm( arg1, arg2 ); \
}

mDefFn(void,usePar,const IOPar&,iop,,)
mDefFn(void,fillPar,IOPar&,iop,const,)
mDefFn(Pos::Provider*,curProvider,,,,return)
mDefFn(const Pos::Provider*,curProvider,,,const,return)
mDefFn(const TrcKeyZSampling&,envelope,,,const,return)
mDefFn(const TrcKeyZSampling&,inputLimit,,,const,return)
mDefFn(bool,isAll,,,const,return)
mDefFn(void,setToAll,,,,)
mDefFn(void,setInputLimit,const TrcKeyZSampling&,cs,,)
mDefFn2(void,setInput,const TrcKeyZSampling&,cs,bool,ct,)
mDefFn2(void,setInput,const TrcKeyZSampling&,initcs,const TrcKeyZSampling&,
	ioparcs,)
