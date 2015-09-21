/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipicksetmgr.h"
#include "uiimppickset.h"
#include "uiioobj.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "keystrs.h"
#include "uimain.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "keyboardevent.h"
#include "ptrman.h"
#include "od_helpids.h"


uiPickSetMgr::uiPickSetMgr( uiParent* p, Pick::SetMgr& m )
    : setmgr_(m)
    , parent_(p)
{
    mAttachCB(uiMain::keyboardEventHandler().keyPressed,
	uiPickSetMgr::keyPressedCB);
}


uiPickSetMgr::~uiPickSetMgr()
{
    detachAllNotifiers();
}


bool uiPickSetMgr::storeNewSet( Pick::Set*& ps ) const
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->setName( ps->name() );
    if ( uiIOObj::fillCtio(*ctio,true) )
    {
	PtrMan<IOObj> ioobj = ctio->ioobj_;
	if ( ps->disp_.connect_ == Pick::Set::Disp::None )
	    ioobj->pars().set( sKey::Type(),
			       PickSetTranslatorGroup::sKeyPickSet() );
	else
	    ioobj->pars().set( sKey::Type(), sKey::Polygon() );

	if ( !doStore( *ps, *ioobj ) )
	    { delete ps; ps = 0; return false; }

	setmgr_.set( ioobj->key(), ps );
	return true;
    }

    delete ps; ps = 0;
    return false;
}


IOObj* uiPickSetMgr::getSetIOObj( const Pick::Set& ps ) const
{
    int setidx = setmgr_.indexOf( ps );
    if ( setidx < 0 ) return 0;

    IOObj* ioobj = IOM().get( setmgr_.id(setidx) );
    if ( !ioobj )
    {
	uiString msg = tr("The PickSet '%1' no longer has "
			  "an entry in the data store.\n"
			  "Please use 'Save As' to store this set.")
		     .arg(ps.name());
	uiMSG().warning( msg );
    }
    return ioobj;
}


bool uiPickSetMgr::storeSet( const Pick::Set& ps )
{
    PtrMan<IOObj> ioobj = getSetIOObj( ps );
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    setmgr_.setUnChanged( setmgr_.indexOf(ps) );
    return true;
}


bool uiPickSetMgr::storeSets()
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( !setmgr_.isChanged(idx) )
	    continue;

	storeSet( setmgr_.get(idx) );
    }
    return true;
}


bool uiPickSetMgr::pickSetsStored() const
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( setmgr_.isChanged(idx) )
	    return false;
    }
    return true;
}


bool uiPickSetMgr::storeSetAs( const Pick::Set& ps )
{
    const bool ispoly = ps.disp_.connect_ != Pick::Set::Disp::None;
    const BufferString oldname = ps.name();
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( PickSet );
    ctio->ctxt_.forread_ = false;

    if ( ispoly )
	ctio->ctxt_.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    ctio->setName( oldname );
    uiIOObjSelDlg dlg( parent_, *ctio );
    if ( !dlg.go() )
	return false;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    const_cast<Pick::Set&>(ps).setName( ioobj->name() );
    const int psidx = setmgr_.indexOf( ps );
    if ( psidx >= 0 )
	setmgr_.setID( psidx, ioobj->key() );
    setmgr_.reportChange( this, ps );
    return true;
}


bool uiPickSetMgr::doStore( const Pick::Set& ps, const IOObj& ioobj ) const
{
    IOM().commitChanges( ioobj );
    BufferString bs;
    if ( !PickSetTranslator::store( ps, &ioobj, bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


class uiMergePickSets : public uiDialog
{ mODTextTranslationClass(uiMergePickSets);
public:

uiMergePickSets( uiParent* p, MultiID& mid )
    : uiDialog(p,uiDialog::Setup("Merge Pick Sets","Specify sets to merge",
				 mODHelpKey(mMergePickSetsHelpID) ))
    , ctioin_( PickSetTranslatorGroup::ioContext() )
    , ctioout_( PickSetTranslatorGroup::ioContext() )
    , mid_(mid)
{
    ctioin_.setObj( IOM().get(mid_) );
    selfld = new uiIOObjSelGrp( this, ctioin_, "Select Pick Sets to merge",
			uiIOObjSelGrp::Setup(OD::ChooseZeroOrMore) );
    ctioout_.ctxt_.forread_ = false;
    outfld = new uiIOObjSel( this, ctioout_, "Output merged set" );
    outfld->attach( alignedBelow, selfld );
}


void setInputSets( const BufferStringSet& nms )
{
    selfld->getListField()->setChosen( nms );
}


bool acceptOK( CallBacker* )
{
    nrsel = selfld->nrChosen();
    if ( nrsel < 2 )
    { uiMSG().error(tr("Please select at least two sets")); return false; }
    else if (!outfld->commitInput())
    {
	uiMSG().error(uiStrings::phrCannotCreate( outfld->getInput() ));
	return false;
    }

    if ( ctioout_.ioobj_ )
	mid_ = ctioout_.ioobj_->key();
    return true;
}

    uiIOObjSelGrp*	selfld;
    uiIOObjSel*		outfld;
    CtxtIOObj		ctioin_;
    CtxtIOObj		ctioout_;
    MultiID&		mid_;

    int			nrsel;

};


void uiPickSetMgr::mergeSets( MultiID& mid, const BufferStringSet* nms )
{
    uiMergePickSets dlg( parent_, mid );
    if ( nms )
	dlg.setInputSets( *nms );

    if ( !dlg.go() ) return;

    ObjectSet<const Pick::Set> pss;
    ObjectSet<Pick::Set> pssread;
    for ( int idx=0; idx<dlg.nrsel; idx++ )
    {
	const MultiID& ky = dlg.selfld->chosenID( idx );
	int setidx = setmgr_.indexOf( ky );
	if ( setidx >= 0 )
	    pss += &setmgr_.get( setidx );
	else
	{
	    Pick::Set* newset = new Pick::Set;
	    IOObj* ioobj = IOM().get( ky );
	    BufferString msg;
	    if ( PickSetTranslator::retrieve(*newset,ioobj,true, msg) )
		{ pss += newset; pssread += newset; }
	    else
		uiMSG().warning( msg );
	    delete ioobj;
	}
    }

    if ( pss.size() < 2 )
    {
	uiMSG().error( tr("Not enough valid sets selected for merge") );
	deepErase( pssread ); return;
    }

    Pick::Set resset( *pss[0] );
    resset.setName( dlg.ctioout_.ioobj_->name() );
    for ( int idx=1; idx<pss.size(); idx ++ )
	resset.append( *pss[idx] );

    BufferString msg;
    if ( !PickSetTranslator::store(resset,dlg.ctioout_.ioobj_,msg) )
	uiMSG().error( msg );

    dlg.ctioout_.ioobj_->pars().set( sKey::Type(),
	    PickSetTranslatorGroup::sKeyPickSet() );
    IOM().commitChanges( *dlg.ctioout_.ioobj_);
    deepErase( pssread );
}

void uiPickSetMgr::keyPressedCB(CallBacker*)
{
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();

    if ( KeyboardEvent::isUnDo(kbe) )
	setmgr_.undo().unDo( 1,true );

    if ( KeyboardEvent::isReDo(kbe) )
	setmgr_.undo().reDo( 1, true );
    
}

