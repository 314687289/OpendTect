/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
#include "uistrings.h"


Seis::Provider::Provider()
    : forcefpdata_(false)
    , selcomp_(-1)
    , readmode_(Prod)
    , zstep_(mUdf(float))
    , seldata_(0)
{
}


Seis::Provider* Seis::Provider::create( Seis::GeomType gt )
{
    switch ( gt )
    {
    case Vol:
	return new VolProvider;
    case VolPS:
	{ pFreeFnErrMsg("Implement VolPS"); return 0; }
    case Line:
	{ pFreeFnErrMsg("Implement Line"); return 0; }
    case LinePS:
	{ pFreeFnErrMsg("Implement LinePS"); return 0; }
    }

    pFreeFnErrMsg("Add switch case");
    return 0;
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    SeisIOObjInfo objinf( dbky );
    Provider* ret = 0;
    if ( !objinf.isOK() )
    {
	if ( uirv )
	    uirv->set( uiStrings::phrCannotFindDBEntry(dbky.toUiString()) );
    }
    else
    {
	ret = create( objinf.geomType() );
	uiRetVal dum; if ( !uirv ) uirv = &dum;
	*uirv = ret->setInput( dbky );
	if ( !uirv->isOK() )
	    { delete ret; ret = 0; }
    }

    return ret;
}


uiRetVal Seis::Provider::usePar( const IOPar& iop )
{
    forcefpdata_ = iop.isTrue( sKeyForceFPData() );
    uiRetVal ret;
    doUsePar( iop, ret );
    return ret;
}


void Seis::Provider::setSubsel( const SelData& sd )
{
    delete seldata_;
    seldata_ = sd.clone();
}


void Seis::Provider::handleTrace( SeisTrc& trc ) const
{
    ensureRightZSampling( trc );
    ensureRightDataRep( trc );
    nrtrcs_++;
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( getlock_ );
    doGetNext( trc, uirv );
    locker.unlockNow();
    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


uiRetVal Seis::Provider::get( const TrcKey& trcky, SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( getlock_ );
    doGet( trcky, trc, uirv );
    locker.unlockNow();
    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


void Seis::Provider::ensureRightDataRep( SeisTrc& trc ) const
{
    if ( !forcefpdata_ )
	return;

    const int nrcomps = trc.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
	trc.data().convertToFPs();
}


void Seis::Provider::ensureRightZSampling( SeisTrc& trc ) const
{
    if ( mIsUdf(zstep_) )
	return;

    const ZSampling trczrg( trc.zRange() );
    ZSampling targetzs( trczrg );
    targetzs.step = zstep_;
    int nrsamps = (int)( (targetzs.stop-targetzs.start)/targetzs.step + 1.5 );
    targetzs.stop = targetzs.atIndex( nrsamps-1 );
    if ( targetzs.stop - targetzs.step*0.001f > trczrg.stop )
    {
	nrsamps--;
	if ( nrsamps < 1 )
	    nrsamps = 1;
    }
    targetzs.stop = targetzs.atIndex( nrsamps-1 );

    TraceData newtd;
    const TraceData& orgtd = trc.data();
    const int newsz = targetzs.nrSteps() + 1;
    const int nrcomps = trc.nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const DataInterpreter<float>& di = *orgtd.getInterpreter(icomp);
	const DataCharacteristics targetdc( forcefpdata_
		? (di.nrBytes()>4 ? OD::F64 : OD::F32) : di.dataChar() );
	newtd.addComponent( newsz, targetdc );
	for ( int isamp=0; isamp<newsz; isamp++ )
	    newtd.setValue( isamp, trc.getValue(targetzs.atIndex(isamp),icomp));
    }

    trc.data() = newtd;
}