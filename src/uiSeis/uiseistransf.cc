/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.42 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseisfmtscale.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "seis2dline.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "conn.h"
#include "errh.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, const uiSeisTransfer::Setup& s )
	: uiGroup(p,"Seis transfer pars")
	, setup_(s)
{
    const bool is2d = Seis::is2D(setup_.geom_);
    const bool isps = Seis::isPS(setup_.geom_);
    if ( is2d )
	selfld = new uiSeis2DSubSel( this, setup_.fornewentry_,
				     setup_.multi2dlines_ );
    else
	selfld = new uiSeis3DSubSel( this, setup_.withstep_ );

    static const char* choices[] = { "Discard", "Pass", "Add", 0 };
    if ( !is2d && !isps && setup_.withnullfill_ )
	remnullfld = new uiGenInput( this, "Null traces",
				     StringListInpSpec(choices) );
    else
	remnullfld = new uiGenInput( this, "Null traces",
				     BoolInpSpec(true,choices[0],choices[1]) );
    remnullfld->attach( alignedBelow, selfld->attachObj() );

    scfmtfld = new uiSeisFmtScale( this, setup_.geom_, !setup_.fornewentry_ );
    scfmtfld->attach( alignedBelow, remnullfld );

    setHAlignObj( remnullfld );
}


uiSeis2DSubSel* uiSeisTransfer::selFld2D()
{
    mDynamicCastGet(uiSeis2DSubSel*,ret,selfld)
    return ret;
}


uiSeis3DSubSel* uiSeisTransfer::selFld3D()
{
    mDynamicCastGet(uiSeis3DSubSel*,ret,selfld)
    return ret;
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    setInput( ioobj );
}


void uiSeisTransfer::setInput( const IOObj& ioobj )
{
    scfmtfld->updateFrom( ioobj );
    selfld->setInput( ioobj );
}


int uiSeisTransfer::maxBytesPerSample() const
{
    DataCharacteristics dc(
	    (DataCharacteristics::UserType)scfmtfld->getFormat() );
    return (int)dc.nrBytes();
}


SeisIOObjInfo::SpaceInfo uiSeisTransfer::spaceInfo() const
{
    int ntr = selfld->expectedNrTraces();
    SeisIOObjInfo::SpaceInfo si( selfld->expectedNrSamples(), ntr,
				   maxBytesPerSample() );
    if ( Seis::is2D(setup_.geom_) )
	si.expectednrtrcs = ntr;

    return si;
}


bool uiSeisTransfer::removeNull() const
{
    return setup_.withnullfill_ ? remnullfld->getIntValue() == 0
				: remnullfld->getBoolValue();
}


bool uiSeisTransfer::fillNull() const
{
    return remnullfld->getIntValue() == 2;
}


void uiSeisTransfer::setSteering( bool yn )
{
    scfmtfld->setSteering( yn );
}


Seis::SelData* uiSeisTransfer::getSelData() const
{
    IOPar iop;
    selfld->fillPar( iop );
    return Seis::SelData::get( iop );
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    if ( selfld->isAll() ) return 0;

    CubeSampling cs;
    selfld->getSampling( cs.hrg );
    selfld->getZRange( cs.zrg );
    return new SeisResampler( cs, Seis::is2D(setup_.geom_) );
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj,
				      const IOObj& outobj,
				      const char* extxt,
				      const char* worktxt,
				      const char* attrnm2d,
				      const char* linenm2d ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(&outobj) );

    PtrMan<Seis::SelData> seldata = getSelData();
    IOPar iop;
    iop.set( "ID", inobj.key() );
    if ( seldata )
    {
	if ( linenm2d && *linenm2d )
	    seldata->lineKey().setLineName( linenm2d );
	seldata->lineKey().setAttrName( attrnm2d );
	seldata->fillPar( iop );
    }

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( &inobj, &outobj,
	    				extxt, &iop, worktxt );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( removeNull() );
    stp->fillNullTraces( fillNull() );
    stp->setResampler( getResampler() );

    return stp;
}
