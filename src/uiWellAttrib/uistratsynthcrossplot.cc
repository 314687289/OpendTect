/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthcrossplot.h"
#include "uistratsynthdisp.h"
#include "uistratlayseqattrsetbuild.h"
#include "uistratseisevent.h"
#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "commondefs.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "prestackgather.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "valseriesevent.h"



uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
			const Strat::LayerModel& lm,
			const ObjectSet<SyntheticData>& synths )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,"110.3.0"))
    , lm_(lm)
    , synthdatas_(synths)
{
    if ( lm.isEmpty() )
	{ errmsg_ = "Input model is empty."
	    "\nYou need to generate layer models."; return; }

    TypeSet<DataPack::FullID> fids, psfids; 
    for ( int idx=0; idx<synths.size(); idx++ )
    {
	const SyntheticData& sd = *synths[idx];
	if ( sd.isPS() )
	    psfids += sd.datapackid_;
	else
	    fids += sd.datapackid_;
    }
    if ( fids.isEmpty() && psfids.isEmpty() )
	{ errmsg_ = "Missing or invalid 'datapacks'."
	    "\nMost likely, no synthetics are available."; return; }

    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(true).showps( psfids.size() );
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    seisattrfld_->setDataPackInp( fids, false );
    seisattrfld_->setDataPackInp( psfids, true );

    uiSeparator* sep = new uiSeparator( this, "sep1", true );
    sep->attach( stretchedBelow, seisattrfld_ );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );
    layseqattrfld_->attach( alignedWith, seisattrfld_ );
    layseqattrfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "sep2" );
    sep->attach( stretchedBelow, layseqattrfld_ );

    evfld_ = new uiStratSeisEvent( this, uiStratSeisEvent::Setup(true) );
    evfld_->attach( alignedWith, layseqattrfld_ );
    evfld_->attach( ensureBelow, sep );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
    deepErase( extrgates_ );
}


DataPointSet* uiStratSynthCrossplot::getData( const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin,
					const Strat::Level* stoplvl )
{
    //If default Desc(s) present remove them
    for ( int idx=seisattrs.size()-1; idx>=0; idx-- )
    {
	const Attrib::Desc* tmpdesc = seisattrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<Attrib::DescSet*>(&seisattrs)->removeDesc(tmpdesc->id());
    }

    DataPointSet* dps = seisattrs.createDataPointSet(Attrib::DescSetup(),false);
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return 0; }
    if ( dps->nrCols() )
    {
	dps->dataSet().insert(dps->nrFixedCols(),new DataColDef(sKey::Depth()));
	dps->dataSet().insert( dps->nrFixedCols()+1,
		    new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }
    else
    {
	dps->dataSet().add( new DataColDef(sKey::Depth()) );
	dps->dataSet().add( 
		    new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }

    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	dps->dataSet().add(
		new DataColDef(seqattrs.attr(iattr).name(),toString(iattr)) );

    const int depthidx = dps->indexOf( sKey::Depth() );

    for ( int isynth=0; isynth<synthdatas_.size(); isynth++ )
    {
	const SyntheticData& sd = *synthdatas_[isynth];
	const ObjectSet<const TimeDepthModel>& d2tmodels = sd.d2tmodels_;
	const int nrmdls = d2tmodels.size();

	mDynamicCastGet(const SeisTrcBufDataPack*,tbpack,&sd.getPack());
	if ( !tbpack ) continue;

	const SeisTrcBuf& tbuf = tbpack->trcBuf();
	if ( tbuf.size() != nrmdls )
	    { pErrMsg("DataPack nr of traces != nr of d2t models"); return 0; }

	TypeSet<float> lvltms;
	const Strat::SeisEvent& ssev = evfld_->event();
	const float owtstep = ssev.extrwin_.step / 2.f;
	for ( int imod=0; imod<nrmdls; imod++ )
	{
	    SeisTrc& trc = const_cast<SeisTrc&>( *tbuf.get( imod ) );
	    const float dpth = lm_.sequence(imod).depthPositionOf( lvl );
	    trc.info().pick = d2tmodels[imod]->getTime( dpth );
	    lvltms += ssev.snappedTime( trc );
	}

	if ( isynth == 0 )
	{
	    const int nrextr = extrwin.nrSteps() + 1;
	    for ( int itrc=0; itrc<nrmdls; itrc++ )
	    {
		const SeisTrc& trc = *tbuf.get( itrc );
		const float starttwt = lvltms[itrc];
		TypeSet<Interval<float> >* extrgate
		    		= new TypeSet<Interval<float> >;
		for ( int iextr=0; iextr<nrextr; iextr++ )
		{
		    const float twt = starttwt + extrwin.atIndex( iextr );
		    float dah = d2tmodels[itrc]->getDepth( twt );
		    if ( mIsUdf(dah) )
			continue;

		    if ( SI().depthsInFeet() )
			dah *= mToFeetFactorF;

		    DataPointSet::DataRow dr;
		    dr.data_.setSize( dps->nrCols(), mUdf(float) );
		    dr.pos_.nr_ = trc.info().nr;
		    dr.pos_.set( trc.info().coord );
		    dr.pos_.z_ = twt;
		    dr.data_[depthidx] = dah;
		    dps->addRow( dr );

		    Interval<float> timerg( dr.pos_.z_ - owtstep,
			    		    dr.pos_.z_ + owtstep );
		    Interval<float> depthrg;
		    depthrg.start = d2tmodels[itrc]->getDepth( timerg.start );
		    depthrg.stop = d2tmodels[itrc]->getDepth( timerg.stop );
		    *extrgate += depthrg;
		}
		extrgates_ += extrgate;
	    }
	}
    }

    dps->dataChanged();

    if ( dps->isEmpty() )
    {
	uiMSG().error( "No positions for data extraction" );
	delete dps; dps = 0;
    }
    else if ( !extractSeisAttribs(*dps,seisattrs)
	    || !extractLayerAttribs(*dps,seqattrs,stoplvl) )
	{ delete dps; dps = 0; }

    extractModelNr( *dps );
    return dps;
}


bool uiStratSynthCrossplot::extractModelNr( DataPointSet& dps ) const
{
    const int modnridx =
	dps.indexOf( Strat::LayModAttribCalc::sKeyModelIdx() );
    if ( modnridx<0 ) return false;

    for ( int dpsrid=0; dpsrid<dps.size(); dpsrid++ )
    {
	float* dpsvals = dps.getValues( dpsrid );
	dpsvals[modnridx] = mCast(float,dps.trcNr(dpsrid));
    }

    return true;
}


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const Attrib::DescSet& attrs )
{
    if ( attrs.isEmpty() )
	return true;

    BufferString errmsg;
    PtrMan<Attrib::EngineMan> aem = createEngineMan( attrs );

    PtrMan<Executor> exec = aem->getTableExtractor(dps,attrs,errmsg,2,false);
    if ( !exec )                                                                
    {                                                                           
	uiMSG().error( errmsg );                                                
	return false;
    }

    exec->setName( "Attributes from Traces" );                       
    uiTaskRunner dlg( this );
    TaskRunner::execute( &dlg, *exec );
    return true;
}


bool uiStratSynthCrossplot::extractLayerAttribs( DataPointSet& dps,
				     const Strat::LaySeqAttribSet& seqattrs,
       				     const Strat::Level* stoplvl )
{
    if ( seqattrs.isEmpty() )
	return true;

    Strat::LayModAttribCalc lmac( lm_, seqattrs, dps );
    lmac.setExtrGates( extrgates_, stoplvl );
    uiTaskRunner tr( this );
    return TaskRunner::execute( &tr, lmac );
}


bool uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    BufferString wintitl( "Attributes at ", lvl.name() );
    wintitl.add( " [" ).add( extrwin.start )
	   .add( "-" ).add( extrwin.stop ).add( "]" );
    uiDataPointSet::Setup su( wintitl, false );
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    uidps->showXY( false );
    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->setDeleteOnClose( true );
    uidps->show();
    return false;
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const Attrib::DescSet& attrs ) const
{                                                                               
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    TypeSet<Attrib::SelSpec> attribspecs;
    attrs.fillInSelSpecs( Attrib::DescSetup().hidden(false), attribspecs );

    aem->setAttribSet( &attrs ); 
    aem->setAttribSpecs( attribspecs );                                         
    return aem;                                                                 
}


void uiStratSynthCrossplot::setRefLevel( const char* lvlnm )
{
    evfld_->setLevel( lvlnm );
}


bool uiStratSynthCrossplot::handleUnsaved()
{
    return seisattrfld_->handleUnsaved() && layseqattrfld_->handleUnsaved();
}


bool uiStratSynthCrossplot::rejectOK( CallBacker* )
{
    return handleUnsaved();
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( !errmsg_.isEmpty() )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( !evfld_->getFromScreen() )
	mErrRet(0)

    const StepInterval<float>& extrwin = evfld_->event().extrwin_;
    const Strat::Level& lvl = *evfld_->event().level_;
    const Strat::Level* stoplvl = evfld_->event().downtolevel_;
    DataPointSet* dps = getData( seisattrs, seqattrs, lvl, extrwin, stoplvl );
    const bool res = dps ? launchCrossPlot( *dps, lvl, extrwin ) : false;

    if ( res && !handleUnsaved() )
	return false;
    return res;
}
