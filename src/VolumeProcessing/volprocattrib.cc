/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribdatapack.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "volprocchain.h"
#include "volproctrans.h"

using namespace Attrib;

mAttrDefCreateInstance(VolProcAttrib)

void VolProcAttrib::initClass()
{
    mAttrStartInitClass
    StringParam* setup = new StringParam( sKeySetup() );
    desc->addParam( setup );

    desc->addOutputDataType( Seis::UnknowData );
    mAttrEndInitClass
}


VolProcAttrib::VolProcAttrib( Desc& ds )
    : Provider(ds)
    , chain_(0)
    , executor_(0)
{
    const char* idstr = desc_.getValParam( sKeySetup() )->getStringValue();
    setupmid_ = MultiID( idstr );
}


VolProcAttrib::~VolProcAttrib()
{
    if ( chain_ ) chain_->unRef();
    delete executor_;
}


bool VolProcAttrib::allowParallelComputation() const
{ return false; }


void VolProcAttrib::prepareForComputeData()
{
    PtrMan<IOObj>  ioobj = IOM().get( setupmid_ );
    if ( !ioobj )
	return;

    chain_ = new VolProc::Chain();
    chain_->ref();
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_, ioobj, errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = tr("Cannot read processing setup.");
	if ( !errmsg.isEmpty() )
	{
	    errmsg_.append(tr(" Reason given: %1").arg( errmsg ) );
	}

	return;
    }

    chain_->setStorageID( setupmid_ );

    executor_ = new VolProc::ChainExecutor( *chain_ );
    const TrcKeyZSampling cs = *getDesiredVolume();
    const Survey::Geometry& geometry = Survey::Geometry3D::default3D();
    const StepInterval<float> geometryzrg = geometry.sampling().zsamp_;
    StepInterval<int> zrg( geometryzrg.nearestIndex( cs.zsamp_.start ),
			   geometryzrg.nearestIndex( cs.zsamp_.stop ),
			   mNINT32(cs.zsamp_.step/geometryzrg.step) );
    if ( !executor_->setCalculationScope(cs.hrg,zrg) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return;
    }

    if ( !executor_->execute() )
    {
	if ( !executor_->errMsg().isEmpty() )
	    errmsg_ = executor_->errMsg();
	else
	    errmsg_ = tr("Error while calculating.");
    }
}


bool VolProcAttrib::computeData( const Attrib::DataHolder& output,
				 const BinID& relpos, int z0, int nrsamples,
				 int threadid ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float outval = mUdf(float);
	setOutputValue( output, 0, idx, z0, outval );
    }

    return true;
}



namespace VolProc
{

// ExternalAttribCalculator
void ExternalAttribCalculator::initClass()
{ Attrib::ExtAttrFact().addCreator( create, 0 ); }


Attrib::ExtAttribCalc* ExternalAttribCalculator::create(
					const Attrib::SelSpec& as )
{
    ExternalAttribCalculator* res = new ExternalAttribCalculator;
    if ( res->setTargetSelSpec( as ) )
	return res;

    delete res;
    return 0;
}


ExternalAttribCalculator::ExternalAttribCalculator()
    : chain_( 0 )
{}


ExternalAttribCalculator::~ExternalAttribCalculator()
{
    if ( chain_ ) chain_->unRef();
}


BufferString ExternalAttribCalculator::createDefinition( const MultiID& setup )
{
    BufferString res = sAttribName();
    res += " ";
    res += sKeySetup();
    res += "=";
    res += setup;

    return res;
}


bool ExternalAttribCalculator::setTargetSelSpec( const Attrib::SelSpec& ss )
{
    const char* definition = ss.defString();

    BufferString attribname;
    if ( !Attrib::Desc::getAttribName( definition, attribname ) ||
	 attribname != sAttribName() )
	return false;

    BufferString midstring;
    if ( !Attrib::Desc::getParamString( definition, sKeySetup(), midstring ) )
	return false;

    MultiID mid = midstring.buf();
    PtrMan<IOObj>  ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	errmsg_ = tr("Cannot find the processing setup.");
	return false;
    }

    chain_ = new Chain();
    chain_->ref();
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_, ioobj, errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = tr("Cannot read processing setup.");
	if ( !errmsg.isEmpty() )
	{
	    errmsg_.append( tr( " Reason given: %1").arg( errmsg ) );
	}

	return false;
    }

    chain_->setStorageID( mid );

    return true;
}


DataPack::ID ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& cs,
						     DataPack::ID dpid,
						     TaskRunner* taskrunner )
{
    if ( !chain_ || !chain_->nrSteps() )
    {
	errmsg_ = tr("There are no steps in the processing chain.");
	return DataPack::cNoID();
    }

    ChainExecutor executor( *chain_ );
    const Survey::Geometry& geometry = Survey::Geometry3D::default3D();
    const StepInterval<float> geometryzrg = geometry.sampling().zsamp_;

    StepInterval<int> zrg( geometryzrg.nearestIndex( cs.zsamp_.start ),
			   geometryzrg.nearestIndex( cs.zsamp_.stop ),
			   mNINT32(cs.zsamp_.step/geometryzrg.step) );
    if ( !executor.setCalculationScope(cs.hrg,zrg) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return DataPack::cNoID();
    }

    if ( !TaskRunner::execute(taskrunner,executor) )
    {
	if ( !executor.errMsg().isEmpty() )
	    errmsg_ = executor.errMsg();
	else
	    errmsg_ = tr("Error while calculating.");
    }

    const RegularSeisDataPack* output = executor.getOutput();
    if ( !output || output->isEmpty() )
    {
	errmsg_ = tr("No output produced");
	return DataPack::cNoID();
    }

    DPM( DataPackMgr::SeisID() ).obtain( output->id() );
    return output->id();
}


} // namespace VolProc
