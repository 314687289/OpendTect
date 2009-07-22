/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : B.Bril & H.Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: prestackattrib.cc,v 1.17 2009-07-22 16:01:30 cvsbert Exp $";

#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "prestackgather.h"

#include "prestackprop.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "survinfo.h"

#include "ioman.h"
#include "ioobj.h"


namespace Attrib
{

mAttrDefCreateInstance(PSAttrib)
    
void PSAttrib::initClass()
{
    mAttrStartInitClass

    desc->addParam( new SeisStorageRefParam("id") );

#define mDefEnumPar(var,typ) \
    epar = new EnumParam( var##Str() ); \
    epar->addEnums( typ##Names() ); \
    desc->addParam( epar )

    EnumParam*
    mDefEnumPar(calctype,::PreStack::PropCalc::CalcType);
    mDefEnumPar(stattype,Stats::Type);
    mDefEnumPar(lsqtype,::PreStack::PropCalc::LSQType);
    mDefEnumPar(valaxis,::PreStack::PropCalc::AxisType);
    mDefEnumPar(offsaxis,::PreStack::PropCalc::AxisType);

    desc->addParam( new BoolParam( useazimStr(), false, false ) );
    IntParam* ipar = new IntParam( componentStr(), 0 , false );
    ipar->setLimits( Interval<int>(0,mUdf(int)) );
    desc->addParam( ipar );
    ipar = ipar->clone(); ipar->setKey( apertureStr() );
    desc->addParam( ipar );

    desc->addParam( new FloatParam( offStartStr(), 0, false ) );
    desc->addParam( new FloatParam( offStopStr(), mUdf(float), false ) );

    desc->addParam( new StringParam( preProcessStr(), "", false ) );

    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


PSAttrib::PSAttrib( Desc& ds )
    : Provider(ds)
    , psrdr_(0)
    , propcalc_(0)
    , preprocessor_( 0 )
    , psioobj_( 0 )
    , component_( 0 )
{
    if ( !isOK() ) return;

    const char* res; mGetString(res,"id") psid_ = res;
    float offstart, offstop;
    mGetFloat( offstart, offStartStr() );
    mGetFloat( offstop, offStopStr() );

    setup_.offsrg_ = Interval<float>( offstart, offstop );

#define mGetSetupEnumPar(var,typ) \
    int tmp_##var = (int)setup_.var##_; \
    mGetEnum(tmp_##var,var##Str()); \
    setup_.var##_ = (typ)tmp_##var

    mGetSetupEnumPar(calctype,::PreStack::PropCalc::CalcType);
    mGetSetupEnumPar(stattype,Stats::Type);
    mGetSetupEnumPar(lsqtype,::PreStack::PropCalc::LSQType);
    mGetSetupEnumPar(valaxis,::PreStack::PropCalc::AxisType);
    mGetSetupEnumPar(offsaxis,::PreStack::PropCalc::AxisType);

    bool useazim = setup_.useazim_;
    mGetBool( useazim, useazimStr() ); setup_.useazim_ = useazim;
    mGetInt( component_, componentStr() );
    mGetInt( setup_.aperture_, apertureStr() );

    BufferString preprocessstr;
    mGetString( preprocessstr, preProcessStr() );
    preprocid_ = preprocessstr;
    PtrMan<IOObj> preprociopar = IOM().get( preprocid_ );
    if ( preprociopar )
    {
	preprocessor_ = new ::PreStack::ProcessManager;
	if ( !PreStackProcTranslator::retrieve( *preprocessor_,preprociopar,
					       errmsg ) )
	{
	    delete preprocessor_;
	    preprocessor_ = 0;
	}
    }
}


PSAttrib::~PSAttrib()
{
    delete propcalc_;
    delete psrdr_;
    delete psioobj_;
    delete preprocessor_;
}


bool PSAttrib::getInputOutput( int input, TypeSet<int>& res ) const
{
    Interval<float>& rg = const_cast<Interval<float>&>(setup_.offsrg_);
    if ( rg.start > 1e28 ) rg.start = 0;
    if ( rg.stop > 1e28 ) rg.stop = mUdf(float);

    return Provider::getInputOutput( input, res );
}


bool PSAttrib::getInputData( const BinID& relpos, int zintv )
{
    if ( !psrdr_ )
	return false;

    if ( preprocessor_ )
    {
	if ( !preprocessor_->reset() || !preprocessor_->prepareWork() )
	    return false;

	const BinID stepout = preprocessor_->getInputStepout();
	const BinID stepoutstep( SI().inlRange(true).step,
				 SI().crlRange(true).step );
	::PreStack::Gather* gather = 0;
	for ( int inlidx=-stepout.inl; inlidx<=stepout.inl; inlidx++ )
	{
	    for ( int crlidx=-stepout.crl; crlidx<=stepout.crl; crlidx++ )
	    {
		const BinID relbid( inlidx, crlidx );
		if ( !preprocessor_->wantsInput(relbid) )
		    continue;

		const BinID bid = currentbid+relpos+relbid*stepoutstep;

		mTryAlloc( gather, ::PreStack::Gather );
		if ( !gather )
		    return false;

		if ( !gather->readFrom( *psioobj_, *psrdr_, bid, component_ ) )
		    continue;

		DPM(DataPackMgr::FlatID()).add( gather );
		preprocessor_->setInput( relbid, gather->id() );
		gather = 0;
	    }
	}

	if ( !preprocessor_->process() )
	    return false;

	propcalc_->setGather( preprocessor_->getOutput() );
	return true;
    }

    const BinID bid = currentbid+relpos;

    mDeclareAndTryAlloc( ::PreStack::Gather*, gather, ::PreStack::Gather );
    if ( !gather )
	return false;

    if ( !gather->readFrom( *psioobj_, *psrdr_, bid, component_ ) )
	return false;

    DPM(DataPackMgr::FlatID()).add( gather );
    propcalc_->setGather( gather->id() );
    return true;
}


#define mErrRet(s1,s2,s3) { errmsg = BufferString(s1,s2,s3); return; }

void PSAttrib::prepPriorToBoundsCalc()
{
    delete psioobj_;
    psioobj_ = IOM().get( psid_ );
    if ( !psioobj_ )
	mErrRet("Cannot find pre-stack data store ",psid_," in object manager")

    if ( desc.is2D() )
	psrdr_ = SPSIOPF().get2DReader(*psioobj_,curlinekey_.lineName().buf());
    else
	psrdr_ = SPSIOPF().get3DReader( *psioobj_ );

    if ( !psrdr_ )
	mErrRet("Cannot create reader for ",psid_," pre-stack data store")
    const char* emsg = psrdr_->errMsg();
    if ( emsg && *emsg ) mErrRet("PS Reader: ",emsg,"");

    mTryAlloc( propcalc_, ::PreStack::PropCalc( setup_ ) );
}


bool PSAttrib::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !propcalc_ )
	return false;

    float extrazfromsamppos = 0;
    if ( needinterp )
	extrazfromsamppos = getExtraZFromSampInterval( z0, nrsamples );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float z = (z0 + idx) * refstep + extrazfromsamppos;
	setOutputValue( output, 0, idx, z0, propcalc_->getVal(z) );
    }

    return true;
}

} // namespace Attrib
