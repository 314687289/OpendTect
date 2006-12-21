/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:		$Id: uiattrvolout.cc,v 1.26 2006-12-21 10:48:24 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrvolout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribengman.h"
#include "uiattrsel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisfmtscale.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "attribsel.h"
#include "errh.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "survinfo.h"
#include "ptrman.h"
#include "binidselimpl.h"
#include "scaler.h"
#include "cubesampling.h"
#include "keystrs.h"

using namespace Attrib;

const char* uiAttrVolOut::sKeyMaxCrlRg = "Maximum Crossline Range";
const char* uiAttrVolOut::sKeyMaxInlRg = "Maximum Inline Range";

static void setTypeAttr( CtxtIOObj& ctio, bool yn )
{
    if ( yn )
	ctio.ctxt.parconstraints.set( sKey::Type, sKey::Attribute );
    else
	ctio.ctxt.parconstraints.removeWithKey( sKey::Type );
}


uiAttrVolOut::uiAttrVolOut( uiParent* p, const DescSet& ad,
			    const NLAModel* n, MultiID id )
	: uiFullBatchDialog(p,Setup("Process"))
	, ctio(mkCtxtIOObj())
    	, subselpar(*new IOPar)
    	, sel(*new CurrentSel)
	, ads(const_cast<DescSet&>(ad))
	, nlamodel(n)
	, nlaid(id)
{
    setHelpID( "101.2.0" );
    setTitleText( "Create seismic output" );

    uiAttrSelData attrdata( &ad );
    attrdata.nlamodel = nlamodel;
    todofld = new uiAttrSel( uppgrp, "Quantity to output", attrdata, ad.is2D());
    todofld->selectiondone.notify( mCB(this,uiAttrVolOut,attrSel) );

    transffld = new uiSeisTransfer( uppgrp, uiSeisTransfer::Setup()
	    	.geom(ad.is2D() ? Seis::Line : Seis::Vol)
	    	.fornewentry(false).withstep(false).multi2dlines(true) );
    transffld->attach( alignedBelow, todofld );
    if ( transffld->selFld2D() )
	transffld->selFld2D()->singLineSel.notify(
				mCB(this,uiAttrVolOut,singLineSel) );

    ctio.ctxt.forread = false;
    setTypeAttr( ctio, true );
    ctio.ctxt.includeconstraints = true;
    ctio.ctxt.allowcnstrsabsent = false;
    objfld = new uiSeisSel( uppgrp, ctio, SeisSelSetup().selattr(false) );
    objfld->attach( alignedBelow, transffld );

    uppgrp->setHAlignObj( transffld );

    addStdFields();
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete ctio.ioobj;
    delete &ctio;
    delete &sel;
    delete &subselpar;
}


CtxtIOObj& uiAttrVolOut::mkCtxtIOObj()
{
    return *mMkCtxtIOObj(SeisTrc);
}


void uiAttrVolOut::singLineSel( CallBacker* )
{
    if ( !transffld->selFld2D() ) return;

    singmachfld->setValue( transffld->selFld2D()->isSingLine() );
    singTogg( 0 );
    singmachfld->display( false );
}


void uiAttrVolOut::attrSel( CallBacker* )
{
    CubeSampling cs;
    const bool is2d = todofld->is2D();
    if ( todofld->getRanges(cs) )
	transffld->selfld->setInput( cs );
    setTypeAttr( ctio, !is2d );
    if ( is2d )
    {
	MultiID key;
	const Desc* desc = ads.getFirstStored();
	if ( desc && desc->getMultiID(key) )
	{
	    PtrMan<IOObj> ioobj = IOM().get( key );
	    if ( ioobj )
	    {
		objfld->setInput( ioobj->key() );
		transffld->setInput( *ioobj );
	    }
	}
    }

    setParFileNmDef( todofld->getInput() );
    singLineSel(0);
}


bool uiAttrVolOut::prepareProcessing()
{
    if ( !objfld->commitInput(true) )
    {
	uiMSG().error( "Please enter an output Seismic data set name" );
	return false;
    }
    else if ( !todofld->checkOutput(*ctio.ioobj) )
	return false;

    sel.ioobjkey = ctio.ioobj->key();
    sel.attrid = todofld->attribID();
    sel.outputnr = todofld->outputNr();
    if ( sel.outputnr < 0 && sel.attrid < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    uiSeisIOObjInfo ioobjinfo( *ctio.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld->spaceInfo() );
    subselpar.clear();
    transffld->selfld->fillPar( subselpar );
    subselpar.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


bool uiAttrVolOut::fillPar( IOPar& iop )
{
    DescID nlamodelid(-1, true);
    if ( nlamodel && todofld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	addNLA( nlamodelid );
    }
    const DescID targetid = nlamodelid < 0 ? todofld->attribID() : nlamodelid;

    IOPar attrpar( "Attribute Descriptions" );
    DescSet* clonedset = ads.optimizeClone( targetid );
    if ( !clonedset )
	return false; 
    clonedset->fillPar( attrpar );

    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString key(SeisTrcStorOutput::attribkey);
        key += "."; key += nm;
        iop.add( key, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iop.set( key, "Cube" );

    key = keybase; key += SeisTrcStorOutput::attribkey;
    key += "."; key += DescSet::highestIDStr();
    iop.set( key, 1 );

    key = keybase; key += SeisTrcStorOutput::attribkey; key += ".0";
    iop.set( key, targetid.asInt() );

    key = keybase; key += SeisTrcStorOutput::seisidkey;
    iop.set( key, ctio.ioobj->key() );
    transffld->scfmtfld->updateIOObj( ctio.ioobj );

    transffld->selfld->fillPar( subselpar );
    CubeSampling cs; cs.usePar( subselpar );
    if ( !cs.hrg.isEmpty() )
    {
	key = keybase; key += SeisTrcStorOutput::inlrangekey;
	iop.set( key, cs.hrg.start.inl, cs.hrg.stop.inl );
	key = keybase; key += SeisTrcStorOutput::crlrangekey;
	iop.set( key, cs.hrg.start.crl, cs.hrg.stop.crl );
    }
    else
    {
	CubeSampling curcs;
	todofld->getRanges( curcs );
	key = keybase; key += SeisTrcStorOutput::inlrangekey;
	iop.set( key, curcs.hrg.start.inl, curcs.hrg.stop.inl );
	key = keybase; key += SeisTrcStorOutput::crlrangekey;
	iop.set( key, curcs.hrg.start.crl, curcs.hrg.stop.crl );
    }

    key = keybase; key += SeisTrcStorOutput::depthrangekey;
    iop.set( key, cs.zrg.start*SI().zFactor(), cs.zrg.stop*SI().zFactor() );
    CubeSampling::removeInfo( subselpar );
    iop.mergeComp( subselpar, keybase );

    Scaler* sc = transffld->scfmtfld->getScaler();
    if ( sc )
    {
	key = keybase; key += Output::scalekey;
	iop.set( key, sc->toString() );
	delete sc;
    }

    iop.set( "Target value", todofld->getAttrName() );
    BufferString linename;
    if ( todofld->is2D() )
    {
	MultiID ky;
	DescSet descset(true);
	if ( nlamodel )
	    descset.usePar( nlamodel->pars() );

	const Desc* desc = nlamodel ? descset.getFirstStored()
	    			    : clonedset->getFirstStored();
	if ( desc && desc->getMultiID(ky) )
	{
	    iop.set( "Input Line Set", ky );
	    linename = ky;
	}
    }

    EngineMan::getPossibleVolume( *clonedset, cs, linename, targetid );
    iop.set( sKeyMaxInlRg, cs.hrg.start.inl, cs.hrg.stop.inl, cs.hrg.step.inl );
    iop.set( sKeyMaxCrlRg, cs.hrg.start.crl, cs.hrg.stop.crl, cs.hrg.step.crl );
    delete clonedset;

    return true;
}


void uiAttrVolOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;

    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads, todofld->outputNr(), 
			   nlamodel, errmsg );

    if ( errmsg.size() )
        uiMSG().error( errmsg );
}
