/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "ctxtioobj.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "rangeposprovider.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"
#include "uipossubsel.h"
#include "od_helpids.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf,
				      float shift )
    : uiDialog(p,uiDialog::Setup("Output selection",mNoDlgTitle,
                                 mODHelpKey(mWriteSurfaceDlgHelpID) ))
    , surface_(surf)
{
    mDynamicCastGet(const EM::Horizon3D*,hor,&surface_);
    const bool hasshift = hor && !mIsZero(shift,SI().zRange(true).step*1e-3);

    iogrp_ = new uiSurfaceWrite( this, surface_,
			     uiSurfaceWrite::Setup(surface_.getTypeStr(),
						   surface_.getUserTypeStr())
			     .withdisplayfld(!hasshift).withsubsel(true) );
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
    sels.selvalues.erase();
}


IOObj* uiWriteSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


bool uiWriteSurfaceDlg::replaceInTree() const
{
    return iogrp_->replaceInTree();
}


//Not used anymore, keep it?
uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup("Input selection",mNoDlgTitle,mNoHelpKey))
{
    iogrp_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(typ).withattribfld(false) );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
}



uiStoreAuxData::uiStoreAuxData( uiParent* p, const EM::Horizon3D& surf )
    : uiDialog(p,uiDialog::Setup("Output selection","Specify attribute name",
				 mODHelpKey(mStoreAuxDataHelpID) ))
    , surface_(surf)
{
    attrnmfld_ = new uiGenInput( this, "Attribute" );
    attrnmfld_->setText( surface_.auxdata.auxDataName(0) );
}


const char* uiStoreAuxData::auxdataName() const
{ return attrnmfld_->text(); }


bool uiStoreAuxData::acceptOK( CallBacker* )
{
    dooverwrite_ = false;
    BufferString attrnm = attrnmfld_->text();
    const bool ispres = checkIfAlreadyPresent( attrnm.buf() );
    if ( ispres )
    {
	BufferString msg( "This surface already has an attribute called:\n" );
	msg += attrnm; msg += "\nDo you wish to overwrite this data?";
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
	dooverwrite_ = true;
    }

    if ( attrnm != surface_.auxdata.auxDataName(0) )
	const_cast<EM::Horizon3D&>(surface_).
	    auxdata.setAuxDataName( 0, attrnm.buf() );

    return true;
}


bool uiStoreAuxData::checkIfAlreadyPresent( const char* attrnm )
{
    EM::IOObjInfo eminfo( surface_.multiID() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    bool present = false;
    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	if ( attrnms.get(idx) == attrnm )
	{
	    present = true;
	    break;
	}
    }

    return present;
}


#define mGet( ioobj, hor2d, hor3d, emfss, flt3d ) \
    (ioobj.group() == EMHorizon2DTranslatorGroup::keyword() ? hor2d : \
    (ioobj.group() == EMHorizon3DTranslatorGroup::keyword() ? hor3d : \
    (ioobj.group() == EMFaultStickSetTranslatorGroup::keyword()? emfss\
							    : flt3d )))

#define mGetHelpID(ioobj) \
    mGet( ioobj, mODHelpKey(mCopySurface2DHelpID), \
          mODHelpKey(mCopySurface3DHelpID), \
          mODHelpKey(mCopySurfaceStickSetsHelpID), \
          mODHelpKey(mCopySurfaceFaultsHelpID) )

#define mGetWinNm(ioobj) \
    mGet( ioobj, "Copy 2D horizon", "Copy 3D horizon", "Copy faultStickSet",\
	  "Copy fault")


uiCopySurface::uiCopySurface( uiParent* p, const IOObj& ioobj,
			      const uiSurfaceRead::Setup& su )
    : uiDialog(p,Setup(mGetWinNm(ioobj),mNoDlgTitle,mGetHelpID(ioobj)))
    , ctio_(*mkCtxtIOObj(ioobj))
{
    inpfld = new uiSurfaceRead( this, su );
    inpfld->setIOObj( ioobj.key() );

    ctio_.ctxt.forread = false;
    ctio_.setObj( 0 );

    if ( ioobj.group() == EMFault3DTranslatorGroup::keyword() )
	outfld = new uiIOObjSel( this, ctio_, "Output Fault" );
    else if ( ioobj.group() != EM::FaultStickSet::typeStr() )
	outfld = new uiIOObjSel( this, ctio_, "Output Surface" );
    else
	outfld = new uiIOObjSel( this, ctio_, "Output Stickset" );

    outfld->attach( alignedBelow, inpfld );
}


uiCopySurface::~uiCopySurface()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


CtxtIOObj* uiCopySurface::mkCtxtIOObj( const IOObj& ioobj )
{
    if ( ioobj.group() == EM::Horizon2D::typeStr() )
	return mMkCtxtIOObj( EMHorizon2D );
    if ( ioobj.group() == EM::Horizon3D::typeStr() )
	return mMkCtxtIOObj( EMHorizon3D );
    if ( ioobj.group() == EM::FaultStickSet::typeStr() )
	return mMkCtxtIOObj( EMFaultStickSet );
    if ( ioobj.group() == EM::Fault3D::typeStr() )
	return mMkCtxtIOObj( EMFault3D );
    return 0;
}


#define mErrRet(msg) { if ( msg ) uiMSG().error(msg); return false; }

bool uiCopySurface::acceptOK( CallBacker* )
{
    if ( !inpfld->processInput() ) return false;
    if ( !outfld->commitInput() )
	mErrRet(outfld->isEmpty() ? "Please select output surface" : 0)

    const IOObj* ioobj = inpfld->selIOObj();
    if ( !ioobj ) mErrRet("Cannot find surface")

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    inpfld->getSelection( sdsel );

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot create object")
    emobj->setMultiID( ioobj->key() );

    mDynamicCastGet(EM::Surface*,surface,emobj.ptr())
    PtrMan<Executor> loader = surface->geometry().loader( &sdsel );
    if ( !loader ) mErrRet("Cannot read surface")

    uiTaskRunner tr( this );
    if ( !TaskRunner::execute( &tr, *loader ) ) return false;

    uiPosSubSel* pss = inpfld->getPosSubSel();
    Pos::Filter* pf = pss ? pss->curProvider() : 0;
    mDynamicCastGet(Pos::Provider3D*,p3d,pf);
    if ( p3d )
	surface->apply( *pf );

    EM::SurfaceIOData outsd;
    outsd.use( *surface );
    EM::SurfaceIODataSelection outsdsel( outsd );
    outsdsel.setDefault();

    mDynamicCastGet(Pos::RangeProvider3D*,rp3d,pf);
    if ( rp3d )
	outsdsel.rg = rp3d->sampling().hrg;

    IOObj* newioobj = outfld->ctxtIOObj().ioobj;
    const MultiID& mid = newioobj->key();
    emobj->setMultiID( mid );
    PtrMan<Executor> saver = surface->geometry().saver( &outsdsel, &mid );
    if ( !saver ) mErrRet("Cannot save surface")

    if ( !TaskRunner::execute( &tr, *saver ) ) return false;

    const BufferString oldsetupname = EM::Surface::getSetupFileName( *ioobj );
    const BufferString newsetupname = EM::Surface::getSetupFileName( *newioobj);
    if ( File::exists(oldsetupname) )
	File::copy( oldsetupname, newsetupname );

    return true;
}
