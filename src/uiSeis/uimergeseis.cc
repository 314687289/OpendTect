/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:		$Id: uimergeseis.cc,v 1.39 2008-03-07 12:39:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "bufstringset.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"

#include "uiseissel.h"
#include "uilistbox.h"
#include "uiexecutor.h"
#include "uigeninput.h"
#include "uimsg.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Seismic file merging",
				 "Specify input/output seismics",
				 "103.1.2"))
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    IOM().to( ctio_.ctxt.getSelKey() );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    BufferStringSet ioobjnms("Stored seismic data");
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
        const IOObj& ioobj = *ioobjs[idx];
        if ( strcmp(ioobj.translator(),"CBVS") ) continue;
        ioobjnms.add( ioobj.name() );
    }

    ioobjnms.sort();
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const char* nm = ioobjnms.get(idx).buf();
	const IOObj* ioobj = (*IOM().dirPtr())[nm];
        ioobjids_ += new MultiID( ioobj ? (const char*)ioobj->key() : "" );
    }
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Input Cubes", true );
    inpfld_ = llb->box();
    inpfld_->addItems( ioobjnms );
    inpfld_->setCurrentItem( 0 );

    stackfld_ = new uiGenInput( this, "Duplicate traces",
	    			BoolInpSpec(true,"Stack","Use first") );
    stackfld_->attach( alignedBelow, llb );

    ctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, stackfld_ );
}


uiMergeSeis::~uiMergeSeis()
{
    delete ctio_.ioobj; delete &ctio_;
    deepErase( ioobjids_ );
}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    ObjectSet<IOPar> inpars; IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    SeisMerger mrgr( inpars, outpar, false );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    uiExecutor dlg( this, mrgr );
    return dlg.go();
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    if ( !outfld_->commitInput(true) )
    {
        uiMSG().error( "Please enter an output Seismic data set name" );
        return false;
    }
    outpar.set( sKey::ID, ctio_.ioobj->key() );

    ObjectSet<IOObj> selobjs;
    for ( int idx=0; idx<inpfld_->size(); idx++ )
    {
        if ( inpfld_->isSelected(idx) )
            selobjs += IOM().get( *ioobjids_[idx] );
    }
    const int inpsz = selobjs.size();
    if ( inpsz < 2 )
    {
	uiMSG().error( "Please select at least 2 inputs" ); 
	return false; 
    }

    static const char* optdirkey = "Optimized direction";
    BufferString type = "";
    BufferString optdir = "";
    for ( int idx=0; idx<inpsz; idx++ )
    {
	const IOObj& ioobj = *selobjs[idx];
	if ( idx == 0 ) 
	{
	    if ( ioobj.pars().hasKey(sKey::Type) )
		type = ioobj.pars().find(sKey::Type);
	    if ( ioobj.pars().hasKey(optdirkey) )
		optdir = ioobj.pars().find( optdirkey );
	}
	IOPar* iop = new IOPar;
	iop->set( sKey::ID, ioobj.key() );
	inpars += iop;
    }
    deepErase( selobjs );

    if ( type.isEmpty() )
	ctio_.ioobj->pars().removeWithKey( sKey::Type );
    else
	ctio_.ioobj->pars().set( sKey::Type, type );
    if ( optdir.isEmpty() )
	ctio_.ioobj->pars().removeWithKey( optdirkey );
    else
	ctio_.ioobj->pars().set( optdirkey, optdir );
    IOM().commitChanges( *ctio_.ioobj );

    return true;
}
