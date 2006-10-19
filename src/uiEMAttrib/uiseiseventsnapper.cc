/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.cc,v 1.4 2006-10-19 11:53:45 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseiseventsnapper.h"

#include "uiexecutor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"

#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seiseventsnapper.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "valseriesevent.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p, const IOObj* inp )
    : uiDialog(p,Setup("Snap horizon to seismic event","",""))
    , horinctio_(*mMkCtxtIOObj(EMHorizon))
    , horoutctio_(*mMkCtxtIOObj(EMHorizon))
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , horizon_(0)
{
    if ( inp )
	horinctio_.setObj( inp->clone() );
    horinfld_ = new uiIOObjSel( this, horinctio_, "Horizon to snap" );

    seisfld_ = new uiSeisSel( this, seisctio_, SeisSelSetup() );
    seisfld_->attach( alignedBelow, horinfld_ );

    BufferStringSet eventnms( VSEvent::TypeNames );
    eventnms.remove(0);
    eventfld_ = new uiGenInput( this, "Event", StringListInpSpec(eventnms) );
    eventfld_->attach( alignedBelow, seisfld_ );

    BufferString gatelbl( "Search gate " ); gatelbl += SI().getZUnit();
    gatefld_ = new uiGenInput( this, gatelbl, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, eventfld_ );

    savefld_ = new uiGenInput( this, "Save snapped horizon",
			       BoolInpSpec("As new","Overwrite",false) );
    savefld_->valuechanged.notify( mCB(this,uiSeisEventSnapper,saveSel) );
    savefld_->attach( alignedBelow, gatefld_ );

    horoutctio_.ctxt.forread = false;
    horoutfld_ = new uiIOObjSel( this, horoutctio_, "Output horizon" );
    horoutfld_->attach( alignedBelow, savefld_ );
    saveSel(0);
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{
    delete horoutctio_.ioobj; delete &horoutctio_;
    delete horinctio_.ioobj; delete &horinctio_;
    delete seisctio_.ioobj; delete &seisctio_;
}


void uiSeisEventSnapper::saveSel( CallBacker* )
{
    horoutfld_->display( savefld_->getBoolValue() );
}


bool uiSeisEventSnapper::readHorizon()
{
    const MultiID& mid = horinfld_->ctxtIOObj().ioobj->key();
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader ) return false;

	uiExecutor dlg( this, *reader );
	if ( !dlg.go() )
	{
	    delete reader;
	    return false;
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisEventSnapper::saveHorizon()
{
    PtrMan<Executor> exec = 0;
    const bool saveas = savefld_ && savefld_->getBoolValue();
    if ( !saveas )
	exec = horizon_->saver();
    else if ( !horoutfld_->ctxtIOObj().ioobj )
	mErrRet( "Cannot continue: write permission problem" )
    else
    {
	const MultiID& mid = horoutfld_->ctxtIOObj().ioobj->key();
	horizon_->setMultiID( mid );
	exec = horizon_->geometry().saver( 0, &mid );
    }

    if ( !exec ) return false;

    uiExecutor dlg( this, *exec );
    const bool res = dlg.go();
    return res;
}


bool uiSeisEventSnapper::acceptOK( CallBacker* )
{
    if ( !seisctio_.ioobj )
	mErrRet( "Please select Seismics" )
    if ( !readHorizon() )
	mErrRet( "Cannot read horizon" );

// TODO: loop over all sections
    EM::SectionID sid = horizon_->sectionID( 0 );
    BinIDValueSet bivs( 1, false );
    horizon_->geometry().fillBinIDValueSet( sid, bivs );

    SeisEventSnapper snapper( *seisctio_.ioobj, bivs );
    snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );
    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1 / SI().zFactor() );
    snapper.setSearchGate( rg );
    uiExecutor dlg( this, snapper );
    if ( !dlg.go() )
	return false;

    BinIDValueSet::Pos pos;
    while ( bivs.next(pos) )
    {
	BinID bid; float z;
	bivs.get( pos, bid, z );
	horizon_->setPos( sid, bid.getSerialized(), Coord3(0,0,z), false );
    }

    if ( !saveHorizon() )
	mErrRet( "Cannot save horizon" )

    return true;
}



