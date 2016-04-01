/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocchainexec.h"

#include "seisdatapack.h"
#include "threadwork.h"
#include "simpnumer.h" // for getCommonStepInterval

uiString VolProc::ChainExecutor::sGetStepErrMsg()
{
    return uiStrings::phrCannotFind( tr("output step with id: %1") );
}


VolProc::ChainExecutor::ChainExecutor( Chain& vr )
    : ::Executor( "Volume processing" )
    , chain_( vr )
    , isok_( true )
    , outputzrg_( 0, 0, 0 )
    , outputdp_( 0 )
    , totalnrepochs_( 1 )
    , curepoch_( 0 )
{
    setName( vr.name().getFullString() );
    web_ = chain_.getWeb();
    //TODO Optimize connections, check for indentical steps using same inputs
}


VolProc::ChainExecutor::~ChainExecutor()
{
    deepErase( epochs_ );
    if ( curepoch_ )
	delete curepoch_;
}


uiString VolProc::ChainExecutor::errMsg() const
{ return errmsg_; }


#define mGetStep( var, id, errret ) \
Step* var = chain_.getStepFromID( id ); \
if ( !var ) \
{ \
    errmsg_ = sGetStepErrMsg().arg(toString(id)); \
    return errret; \
}


bool VolProc::ChainExecutor::scheduleWork()
{
    deepErase( epochs_ );
    scheduledsteps_.erase();

    ObjectSet<Step> unscheduledsteps;

    //This is the output step of the whole chain,
    //but the input step of the connection since in receives data
    mGetStep( chainoutputstep, chain_.outputstepid_, false );

    unscheduledsteps += chainoutputstep;

    int firstepoch = 0;
    while ( unscheduledsteps.size() )
    {
	Step* currentstep = unscheduledsteps.pop(); //an inputstep
	if ( !scheduledsteps_.addIfNew( currentstep ) )
	    continue;

	const int lateststepepoch = computeLatestEpoch( currentstep->getID() );
	if ( mIsUdf(lateststepepoch) )
	{
	    pErrMsg("Cannot compute latest epoch");
	    return false;
	}

	firstepoch = mMAX( firstepoch, lateststepepoch );

	TypeSet<Chain::Connection> inputconnections;
	web_.getConnections( currentstep->getID(), true, inputconnections );
	for ( int idx=0; idx<inputconnections.size(); idx++ )
	{
	    mGetStep( outputstep, inputconnections[idx].outputstepid_, false );
	    if ( scheduledsteps_.isPresent( outputstep ) ||
		 unscheduledsteps.isPresent( outputstep ) )
		continue;

	    unscheduledsteps += outputstep;
	}
    }

    const int nrepochs = firstepoch+1;
    for ( int idx=0; idx<nrepochs; idx++ )
    {
	Epoch* epoch = new Epoch( *this );
	for ( int idy=0; idy<scheduledsteps_.size(); idy++ )
	    if ( computeLatestEpoch( scheduledsteps_[idy]->getID() )==idx )
		epoch->addStep( scheduledsteps_[idy] );

	epochs_ += epoch;
    }

    totalnrepochs_ = epochs_.size();

    return true;
}


int VolProc::ChainExecutor::computeLatestEpoch( Step::ID stepid ) const
{
    if ( stepid==chain_.outputstepid_ )
	return 0;

    TypeSet<Chain::Connection> outputconnections;
    web_.getConnections( stepid, false, outputconnections );

    int latestepoch = mUdf(int);
    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const int curepoch =
	    computeLatestEpoch( outputconnections[idx].inputstepid_ );
	if ( mIsUdf(curepoch) )
	    continue;

	const int newepoch = curepoch+1;
	if ( mIsUdf(latestepoch) || newepoch>latestepoch )
	    latestepoch = newepoch;
    }

    return latestepoch;
}


void VolProc::ChainExecutor::computeComputationScope( Step::ID stepid,
				TrcKeySampling& stepoutputhrg,
				StepInterval<int>& stepoutputzrg ) const
{
    if ( stepid==chain_.outputstepid_ )
    {
	stepoutputhrg = outputhrg_;
	stepoutputzrg = outputzrg_;
	return;
    }

    TypeSet<Chain::Connection> outputconnections;
    web_.getConnections( stepid, false, outputconnections );

    stepoutputhrg.init(false);
    stepoutputzrg.setUdf();

    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const Step* nextstep = chain_.getStepFromID(
			    outputconnections[idx].inputstepid_ );

	TrcKeySampling nextstephrg;
	StepInterval<int> nextstepzrg;
	computeComputationScope( nextstep->getID(), nextstephrg, nextstepzrg );

	const TrcKeySampling requiredhrg = nextstep->getInputHRg( nextstephrg );
	if ( stepoutputhrg.isDefined() )
	    stepoutputhrg.include( requiredhrg );
	else
	    stepoutputhrg = requiredhrg;

	const StepInterval<int> requiredzrg =
		nextstep->getInputZRg( nextstepzrg );
	if ( stepoutputzrg.isUdf() )
	    stepoutputzrg = requiredzrg;
	else
	    stepoutputzrg = getCommonStepInterval( stepoutputzrg, requiredzrg );

    }
}

namespace VolProc
{

struct VolumeMemory
{
    Step::ID		creator_;
    Step::OutputSlotID	outputslot_;
    od_int64		nrbytes_;
    int			firstepoch_;
    int			lastepoch_;

			VolumeMemory( Step::ID creator,
				      Step::OutputSlotID outputslot,
				      od_int64 nrbytes,
				      int firstepoch, int lastepoch )
			: creator_(creator)
			, outputslot_(outputslot)
			, nrbytes_(nrbytes)
			, firstepoch_(firstepoch)
			, lastepoch_(lastepoch)			{};

    bool		operator==( VolumeMemory vm ) const
			{
			    return creator_ == vm.creator_ &&
				   outputslot_ == vm.outputslot_ &&
				   nrbytes_ == vm.nrbytes_ &&
				   firstepoch_ == vm.firstepoch_ &&
				   lastepoch_ == vm.lastepoch_;
			}
};

} // namespace VolProc


od_int64 VolProc::ChainExecutor::computeMaximumMemoryUsage(
					const TrcKeySampling& hrg,
					const StepInterval<int>& zrg )
{
    if ( !scheduleWork() )
	return -1;

    TypeSet<VolumeMemory> activevolumes;
    const od_int64 basesize = Step::getBaseMemoryUsage( hrg, zrg );

    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	{
	    const Step* step = steps[stepidx];
	    for ( int outputidx=0; outputidx<step->getNrOutputs(); outputidx++ )
	    {
		if ( !step->validOutputSlotID(step->getOutputSlotID(outputidx)))
		    continue;

		const od_int64 extrasize = step->extraMemoryUsage( outputidx,
								    hrg, zrg );

		od_int64 outputsize = basesize + extrasize;
		VolumeMemory volmem( step->getID(), outputidx, outputsize,
				     epochidx, epochidx );

		activevolumes += volmem;
	    }

	    //Handle if mem can be the same.

	    for ( int inputidx=0; inputidx<step->getNrInputs(); inputidx++ )
	    {
		//Look at connection. Is any of the activevolumes used ?
		const int activevolidx = 0; //Compute();
		activevolumes[activevolidx].lastepoch_ = epochidx;
	    }
	}
    }

    od_int64 res = 0;
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	od_int64 memneeded = 0;

	for ( int idx=0; idx<activevolumes.size(); idx++ )
	{
	    if ( epochidx<activevolumes[idx].firstepoch_ )
		continue;

	    if ( epochidx>activevolumes[idx].lastepoch_ )
		continue;

	    memneeded += activevolumes[idx].nrbytes_;
	}
	res += memneeded;
    }

    return res;
}


bool VolProc::ChainExecutor::setCalculationScope( const TrcKeySampling& hrg,
					 const StepInterval<int>& zrg )
{
    outputhrg_ = hrg;
    outputzrg_ = zrg;

    return scheduleWork();
}


bool VolProc::ChainExecutor::areSamplesIndependent() const
{
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	    if ( !steps[stepidx]->areSamplesIndependent() )
		return false;
    }
    return true;
}


bool VolProc::ChainExecutor::needsFullVolume() const
{
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	    if ( steps[stepidx]->needsFullVolume() )
		return true;
    }
    return false;
}


VolProc::ChainExecutor::Epoch::Epoch( const ChainExecutor& ce )
    : taskgroup_(*new TaskGroup)
    , chainexec_(ce)
{
    taskgroup_.setParallel( true );
    taskgroup_.setName( ce.name() );
}


bool VolProc::ChainExecutor::Epoch::needsStepOutput( Step::ID stepid ) const
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	const Step* currentstep = steps_[idx];
	if ( currentstep->getID() == stepid )
	    return true;

	TypeSet<Chain::Connection> inputconnections;
	chainexec_.web_.getConnections( currentstep->getID(), true,
					inputconnections );
	for ( int idy=0; idy<inputconnections.size(); idy++ )
	    if ( inputconnections[idy].outputstepid_==stepid )
		return true;
    }

    return false;
}


bool VolProc::ChainExecutor::Epoch::doPrepare( ProgressMeter* progmeter )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	Step* currentstep = steps_[idx];
	TypeSet<Chain::Connection> inputconnections;
	chainexec_.web_.getConnections( currentstep->getID(), true,
					inputconnections );
	PosInfo::CubeData trcssampling;
	for ( int idy=0; idy<inputconnections.size(); idy++ )
	{
	    const Step* outputstep = chainexec_.chain_.getStepFromID(
					inputconnections[idy].outputstepid_ );
	    if ( !outputstep )
	    {
		pErrMsg("This should not happen");
		return false;
	    }

	    const RegularSeisDataPack* input = outputstep->getOutput();
	    const Step::OutputSlotID outputslot =
					inputconnections[idy].outputslotid_;
	    const int outputidx = outputstep->getOutputIdx( outputslot );

	    if ( !input->validComp(outputidx) )
	    {
		pErrMsg("Output is not available");
		return false;
	    }

	    currentstep->setInput( inputconnections[idy].inputslotid_, input );
	    if ( input->getTrcsSampling() )
		trcssampling.merge( *input->getTrcsSampling(), true );
	}

	TrcKeySampling stepoutputhrg;
	StepInterval<int> stepoutputzrg;

	chainexec_.computeComputationScope( currentstep->getID(), stepoutputhrg,
					    stepoutputzrg );

	TrcKeyZSampling csamp;
	csamp.hsamp_ = stepoutputhrg;
	const StepInterval<float> fullzrg = csamp.zsamp_;
	csamp.zsamp_.start = stepoutputzrg.start * fullzrg.step; //index -> real
	csamp.zsamp_.stop = stepoutputzrg.stop * fullzrg.step; //index -> real
	csamp.zsamp_.step = stepoutputzrg.step * fullzrg.step; //index -> real

	RefMan<RegularSeisDataPack> outcube =
	       DPM( DataPackMgr::SeisID() ).add( new RegularSeisDataPack( 0 ) );
	outcube->setSampling( csamp );
	if ( trcssampling.totalSizeInside( csamp.hsamp_ ) > 0 )
	    outcube->setTrcsSampling(new PosInfo::SortedCubeData(trcssampling));

	if ( !outcube->addComponent( 0 ) )
	{
	    errmsg_ = "Cannot allocate enough memory.";
	    return false;
	}

	Step::OutputSlotID outputslotid = 0; // TODO: get correct slotid
	currentstep->setOutput( outputslotid, outcube,
				stepoutputhrg, stepoutputzrg );

	//The step should have reffed it, so we can forget it now.
	outcube = 0;

	TypeSet<Chain::Connection> outputconnections;
	chainexec_.web_.getConnections( currentstep->getID(),
					false, outputconnections );

	for ( int idy=0; idy<outputconnections.size(); idy++ )
	     currentstep->enableOutput( outputconnections[idy].outputslotid_ );

	if ( currentstep->getID()==chainexec_.chain_.outputstepid_ )
	    currentstep->enableOutput( chainexec_.chain_.outputslotid_ );

	Task* newtask = currentstep->needReportProgress()
			    ? currentstep->createTaskWithProgMeter(progmeter)
			    : currentstep->createTask();
	if ( !newtask )
	{
	    pErrMsg("Could not create task");
	    return false;
	}

	taskgroup_.addTask( newtask );
    }

    return true;
}


const RegularSeisDataPack* VolProc::ChainExecutor::Epoch::getOutput() const
{
    return steps_[steps_.size()-1] ? steps_[steps_.size()-1]->getOutput() : 0;
}


const RegularSeisDataPack* VolProc::ChainExecutor::getOutput() const
{
    return outputdp_;
}


#define mCleanUpAndRet( ret ) \
{ \
    delete curepoch_; \
    curepoch_ = 0; \
    return ret; \
}

int VolProc::ChainExecutor::nextStep()
{
    if ( curepoch_ )
	{ delete curepoch_; curepoch_ = 0; }

    if ( !isok_ )
	return ErrorOccurred();
    if ( epochs_.isEmpty() )
	return Finished();

    curepoch_ = epochs_.pop();

    if ( !curepoch_->doPrepare(progressmeter_) )
	mCleanUpAndRet( ErrorOccurred() )

    Task& curtask = curepoch_->getTask();
    if ( progressmeter_ )
	progressmeter_->skipProgress( false );
    curtask.setProgressMeter( progressmeter_ );

    curtask.enableWorkControl( true );

    if ( !curtask.execute() )
	mCleanUpAndRet( ErrorOccurred() )

    if ( epochs_.isEmpty() )		//we just executed the last one
	outputdp_ = curepoch_->getOutput();

    //To prevent the overall chain progress display in between sub-tasks
    if ( progressmeter_ )
	progressmeter_->skipProgress( true );

    return epochs_.isEmpty() ? Finished() : MoreToDo();
}


void VolProc::ChainExecutor::releaseMemory()
{
    ObjectSet<Step> stepstorelease = scheduledsteps_;
    for ( int idx=stepstorelease.size()-1; idx>=0; idx-- )
    {
	const Step::ID curid = stepstorelease[idx]->getID();
	for ( int idy=0; idy<epochs_.size(); idy++ )
	{
	    if ( epochs_[idy]->needsStepOutput( curid ) )
	    {
		stepstorelease.removeSingle( idx );
		break;
	    }
	}
    }

    for ( int idx=stepstorelease.size()-1; idx>=0; idx-- )
	stepstorelease[idx]->releaseData();
}


void VolProc::ChainExecutor::controlWork( Task::Control ctrl )
{
    Task::controlWork( ctrl );
    if ( curepoch_ )
	curepoch_->getTask().controlWork( ctrl );
}


od_int64 VolProc::ChainExecutor::nrDone() const
{
    if ( totalnrepochs_ < 1 )
	return 0;

    const float percentperepoch = 100.f / totalnrepochs_;
    const int epochsdone = totalnrepochs_ - epochs_.size() - 1;
    float percentagedone = percentperepoch * epochsdone;

    if ( curepoch_ )
    {
	Task& curtask = curepoch_->getTask();

	const od_int64 nrdone = curtask.nrDone();
	const od_int64 totalnr = curtask.totalNr();
	if ( nrdone>=0 && totalnr>0 )
	{
	    const float curtaskpercentage = percentperepoch * nrdone / totalnr;
	    percentagedone += curtaskpercentage;
	}
    }

    return mNINT64( percentagedone );
}


uiString VolProc::ChainExecutor::uiNrDoneText() const
{
    return uiStrings::sPercentageDone();
}


od_int64 VolProc::ChainExecutor::totalNr() const
{
    return 100;
}


uiString VolProc::ChainExecutor::uiMessage() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    if ( curepoch_ )
	return curepoch_->getTask().uiMessage();

    return tr("Preparing processing");
}
