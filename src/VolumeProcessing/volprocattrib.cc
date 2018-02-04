/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "dbman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "volprocchainexec.h"
#include "volproctrans.h"

using namespace Attrib;

mAttrDefCreateInstance(VolProcAttrib)

void VolProcAttrib::initClass()
{
    mAttrStartInitClass
    StringParam* setup = new StringParam( sKeySetup() );
    desc->addParam( setup );

    desc->addOutputDataType( Seis::UnknownData );
    mAttrEndInitClass
}


VolProcAttrib::VolProcAttrib( Desc& ds )
    : Provider(ds)
    , chain_(0)
    , executor_(0)
{
    const char* idstr = desc_.getValParam( sKeySetup() )->getStringValue();
    setupmid_ = DBKey::getFromString( idstr );
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
    PtrMan<IOObj>  ioobj = DBM().get( setupmid_ );
    if ( !ioobj )
	return;

    chain_ = new VolProc::Chain();
    chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = uiStrings::phrCannotRead(tr("processing setup."));
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
    const float zstep = geometry.sampling().zsamp_.step;
    StepInterval<int> zrg( mNINT32( cs.zsamp_.start/zstep ),
			   mNINT32( cs.zsamp_.stop/zstep ),
			   mNINT32( cs.zsamp_.step/zstep ) );
    if ( !executor_->setCalculationScope(cs.hsamp_,zrg) )
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
{ Attrib::ExtAttribCalc::factory().addCreator( create, "VolProc" ); }


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


BufferString ExternalAttribCalculator::createDefinition( const DBKey& setup )
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

    BufferString dbkystring;
    if ( !Attrib::Desc::getParamString( definition, sKeySetup(), dbkystring ) )
	return false;

    DBKey dbky = DBKey::getFromString( dbkystring );
    PtrMan<IOObj>  ioobj = DBM().get( dbky );
    if ( !ioobj )
    {
	errmsg_ = tr("Cannot find the processing setup.");
	return false;
    }

    chain_ = new Chain();
    chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = uiStrings::phrCannotRead(tr("processing setup.") );
	if ( !errmsg.isEmpty() )
	{
	    errmsg_.append( tr( " Reason given: %1").arg( errmsg ) );
	}

	return false;
    }

    chain_->setStorageID( dbky );

    return true;
}


RefMan<RegularSeisDataPack>
ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& cs,
						     DataPack::ID dpid,
						     TaskRunner* taskrunner )
{
    if ( !chain_ || !chain_->nrSteps() )
    {
	errmsg_ = tr("There are no steps in the processing chain.");
	return 0;
    }

    ChainExecutor executor( *chain_ );
    const Survey::Geometry& geometry = Survey::Geometry3D::default3D();
    const float zstep = geometry.sampling().zsamp_.step;
    StepInterval<int> zrg( mNINT32( cs.zsamp_.start/zstep ),
			   mNINT32( cs.zsamp_.stop/zstep ),
			   mNINT32( cs.zsamp_.step/zstep ) );
    if ( !executor.setCalculationScope(cs.hsamp_,zrg) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return 0;
    }

    if ( !TaskRunner::execute(taskrunner,executor) )
    {
	if ( executor.errMsg().isEmpty() )
	    errmsg_ = tr("Error while calculating.");

	return 0;
    }

    RefMan<RegularSeisDataPack> output = executor.getOutput();
    if ( !output || output->isEmpty() )
    {
	errmsg_ = tr("No output produced");
	return 0;
    }

    return output;
}


} // namespace VolProc
