/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwaveletextraction.cc,v 1.19 2010-07-19 09:24:54 cvsnageswara Exp $";

#include "uiwaveletextraction.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiposprovgroupstd.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiselsurvranges.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "bufstring.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceposprov.h"
#include "ioman.h"
#include "iopar.h"
#include "posprovider.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletextractor.h"


uiWaveletExtraction::uiWaveletExtraction( uiParent* p, bool is2d )
    : uiDialog( this,Setup("Wavelet Extraction","Specify parameters","103.3.4")
	             .modal(true) )
    , seisctio_(*mMkCtxtIOObj(SeisTrc)) 
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , wvltsize_(0)
    , zrangefld_(0)
    , extractionDone(this)
    , sd_(0)
    , seissel3dfld_(0)
    , linesel2dfld_(0)
{
    setCtrlStyle( DoAndStay );

    if ( !is2d )
    {
	seisctio_.ctxt.forread = true;
	seisctio_.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
	seisctio_.ctxt.includeconstraints = false;
	seisctio_.ctxt.allowcnstrsabsent = true;

	seissel3dfld_ = new uiSeisSel( this, seisctio_,
	    			     uiSeisSel::Setup(false,false) );
	seissel3dfld_->selectionDone.notify(
				mCB(this,uiWaveletExtraction,inputSelCB) );

	subselfld3d_ = new uiSeis3DSubSel( this, Seis::SelSetup(false,false)
					         .onlyrange(true)
					         .withstep(true)
					         .withoutz(true) );
	subselfld3d_->attach( alignedBelow, seissel3dfld_ );
    }
    else
    {
	uiSeis2DMultiLineSel::Setup su( "Select lines" );
	linesel2dfld_ = new uiSeis2DMultiLineSel( this, su.withz(true) );
	linesel2dfld_->butPush.notify( 
				mCB(this,uiWaveletExtraction,inputSelCB) );
    }

    createCommonUIFlds();
    finaliseDone.notify( mCB(this,uiWaveletExtraction,choiceSelCB) );
}


void uiWaveletExtraction::createCommonUIFlds()
{
    zextraction_ = new uiGenInput( this, "Vertical Extraction",
	    		BoolInpSpec(linesel2dfld_,"Z range","Horizons") );
    zextraction_->valuechanged.notify(
				mCB(this,uiWaveletExtraction,choiceSelCB) );
    if ( linesel2dfld_ )
	zextraction_->attach( alignedBelow, linesel2dfld_ );
    else
    	zextraction_->attach( alignedBelow, subselfld3d_ );

    BufferString rangelbl( "Z Range ", SI().getZUnitString() );
    zrangefld_ = new uiSelZRange( this, false, false, rangelbl );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface, this,
	   		 uiPosProvGroup::Setup(linesel2dfld_,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    BufferString lbl = "Wavelet length ";
    lbl += SI().getZUnitString();
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, surfacesel_ );

    BufferString taperlbl = "Taper length ";
    taperlbl += SI().getZUnitString();
    taperfld_ = new uiGenInput( this, taperlbl, IntInpSpec(20) );
    taperfld_->attach( alignedBelow, wtlengthfld_ );

    wvltphasefld_ = new uiGenInput( this, "Phase (Degrees)", IntInpSpec(0) );
    wvltphasefld_->attach( alignedBelow, taperfld_ );

    wvltctio_.ctxt.forread = false;
    outputwvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
    outputwvltfld_->attach( alignedBelow, wvltphasefld_ );
}


uiWaveletExtraction::~uiWaveletExtraction()
{
    delete sd_;
    delete &seisctio_; delete &wvltctio_;
}


void uiWaveletExtraction::choiceSelCB( CallBacker* )
{
    zrangefld_->display( zextraction_->getBoolValue() );
    surfacesel_->display( !zextraction_->getBoolValue() );
    if ( !zextraction_->getBoolValue() )
    {
	Interval<float> defextraz( -.2, .2 );
	IOPar extrazpar;
	extrazpar.set( "Surface.Extra Z", defextraz );
	surfacesel_->usePar( extrazpar );
    }
}


void uiWaveletExtraction::inputSelCB( CallBacker* )
{
    CubeSampling cs;
    PtrMan<SeisIOObjInfo> si=0;
    if ( !linesel2dfld_ )
    	 si = new SeisIOObjInfo( seissel3dfld_->ioobj() );
    else
	 si = new SeisIOObjInfo( linesel2dfld_->getIOObj() );

    si->getRanges( cs );
    if ( !linesel2dfld_ && subselfld3d_ )
    {
	cs.hrg.step.inl = cs.hrg.step.crl = 10;
	subselfld3d_->uiSeisSubSel::setInput( cs );
    }

    if ( zextraction_->getBoolValue() )
    {
	zrangefld_->setRangeLimits( cs.zrg );
    	zrangefld_->setRange( cs.zrg );
    }
} 


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    if ( !linesel2dfld_ )
    {
    	if ( !seissel3dfld_->ioobj() || !outputwvltfld_->ioobj() ) 
	    return false;
    }
    else
    {
       if ( !check2DFlds() )
	   return false;
    }

    IOPar inputpars, surfacepars;
    if ( !linesel2dfld_ )
	seissel3dfld_->fillPar( inputpars );

    outputwvltfld_->fillPar( inputpars );

    if ( subselfld3d_ && !linesel2dfld_ )
	subselfld3d_->fillPar( inputpars );

    if ( !checkWaveletSize() )
	return false;

    if ( !zextraction_->getBoolValue() )
    {
	if ( !surfacesel_->fillPar(surfacepars) )
	    return false;
    }

    int taperlen = mNINT( taperfld_->getfValue() );
    int wvltlen = mNINT( wtlengthfld_->getfValue() );
    if ( (2*taperlen > wvltlen) || taperlen < 0 )
    {
	uiMSG().error( "TaperLength should be in between\n",
		       "0 and ( Wavelet Length/2)" );
	taperfld_->setValue( 5 );
	return false;
    }	

    if ( wvltphasefld_->getfValue()<-180 || wvltphasefld_->getfValue()>180 )
    {
	uiMSG().error( "Please enter Phase between -180 and 180" );
	wvltphasefld_->setValue( 0 );
	return false;
    }

    if ( outputwvltfld_->existingTyped() )
	outputwvltfld_->setConfirmOverwrite( true );

    doProcess( inputpars, surfacepars );
    return false;
}


bool uiWaveletExtraction::checkWaveletSize()
{
    wvltsize_ = mNINT( wtlengthfld_->getfValue() /
	    		      (SI().zStep() * SI().zFactor()) ) + 1;
    if ( zextraction_->getBoolValue() )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	int range = 1 + mNINT( (zrg.stop - zrg.start) / SI().zStep() );
	if ( range < wvltsize_ )
	{
	    uiMSG().message( "Selection window size should be more",
		   	     " than Wavelet Size" );
	    return false;
	}
    }

    if ( wvltsize_ < 3 )
    {
	uiMSG().error( "Minimum 3 samples are required to create Wavelet" );
	wtlengthfld_->setValue( 120 );
	return false;
    }

    return true;
}


bool uiWaveletExtraction::check2DFlds()
{
    if ( !linesel2dfld_->getIOObj() )
    {
	uiMSG().warning( "Please select LineSet name" );
	return false;
    }

    if ( linesel2dfld_->getSelLines().isEmpty() )
    {
	uiMSG().error( "Select at least one line from LineSet" );
	return false;
    }

    if ( !outputwvltfld_->ioobj() )
	return false;

    if ( linesel2dfld_ && !zextraction_->getBoolValue() )
    {
	uiMSG().message( "Extraction of wavelet from 2D-horizon(s)",
			 " is not implemented" );
	return false;
    }

    return true;
}


bool uiWaveletExtraction::doProcess( const IOPar& rangepar,
				     const IOPar& surfacepar )
{
    int phase = mNINT(wvltphasefld_->getfValue());
    int taperlength = mNINT(taperfld_->getfValue());

    PtrMan<WaveletExtractor> extractor=0;
    if ( !linesel2dfld_ )
    {
	if ( !getSelData(rangepar,surfacepar) || !sd_ )
	    return false;

	extractor = new WaveletExtractor( *seisctio_.ioobj, wvltsize_ );
	extractor->setSelData( *sd_ );
    }
    else
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	Seis::RangeSelData range;
	range.setZRange( zrg );
	Interval<int> inlrg(0,0);
	range.cubeSampling().hrg.setInlRange( inlrg );

	ObjectSet<Seis::SelData> sdset;
	StepInterval<int> trcrg;
	BufferStringSet sellines = linesel2dfld_->getSelLines();
	const TypeSet<StepInterval<int> >&  trcrgs = linesel2dfld_->getTrcRgs();
	for ( int lidx=0; lidx<sellines.size(); lidx++ )
	{
	    range.cubeSampling().hrg.setCrlRange( trcrgs[lidx] );
	    range.lineKey().setLineName( sellines.get(lidx).buf() );
	    sd_ = range.clone();
	    sdset += sd_;
	}

	extractor = new WaveletExtractor( *linesel2dfld_->getIOObj(),							  wvltsize_ );
	extractor->setSelData( sdset );
    }

    extractor->setCosTaperParamVal( taperlength );
    extractor->setPhase( phase );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*extractor) )
	return false;

    Wavelet storewvlt = extractor->getWavelet();
    storewvlt.put( wvltctio_.ioobj );
    extractionDone.trigger();

    return true;
}


bool uiWaveletExtraction::fillHorizonSelData( const IOPar& rangepar,
					      const IOPar& surfacepar,
					      Seis::TableSelData& tsd )
{
    const char* extrazkey = IOPar::compKey( sKey::Surface,
	    			  	  Pos::EMSurfaceProvider::extraZKey() );
    Interval<float> extz( 0, 0 );
    if ( surfacepar.get(extrazkey,extz) )
	tsd.extendZ( extz );

    Pos::Provider3D* prov = Pos::Provider3D::make( rangepar );
    BufferString surfkey = IOPar::compKey( sKey::Surface,
	    				   Pos::EMSurfaceProvider::id1Key() );
    MultiID surf1mid, surf2mid;
    if ( !surfacepar.get( surfkey.buf(), surf1mid ) )
	return false;

    surfkey = IOPar::compKey( sKey::Surface,
			      Pos::EMSurfaceProvider::id2Key() );
    const bool betweenhors = surfacepar.get( surfkey.buf(), surf2mid );

    if ( !betweenhors )
    {
	const int size = int ( 1+(extz.stop-extz.start)/SI().zStep() );
	if ( size < wvltsize_ )
	{
	    uiMSG().error( "Selection window size should be"
		           " more than Wavelet size" );
	    return false;
	}
    }

    uiTaskRunner dlg( this );
    EM::EMObject* emobjsinglehor = EM::EMM().loadIfNotFullyLoaded( surf1mid,
	    							   &dlg );

    if ( emobjsinglehor )
	emobjsinglehor->ref();
    else 
	return false;

    mDynamicCastGet(EM::Horizon3D*,horizon1,emobjsinglehor)
    if ( !horizon1 )
    {
	uiMSG().error( "Error loading horizon" );
	return false;
    }

    if ( betweenhors )
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	EM::EMObject* emobjdoublehor = EM::EMM().loadIfNotFullyLoaded( surf2mid,
								       &dlg );

	if ( emobjdoublehor )
	    emobjdoublehor->ref();
	else 
	    return false;

	mDynamicCastGet( EM::Horizon3D*, horizon2,emobjdoublehor )
	if ( !horizon2 )
	{
	    uiMSG().error( "Error loading second horizon" );
	    return false;
	}

	EM::SectionID sid2 = horizon2->sectionID( 0 );

	BinIDValueSet& bvs = tsd.binidValueSet();
	bvs.allowDuplicateBids( true );
	horizon1->geometry().fillBinIDValueSet( sid, bvs, prov );
	horizon2->geometry().fillBinIDValueSet( sid2, bvs, prov );

	emobjdoublehor->unRef();
    }
    else
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	horizon1->geometry().fillBinIDValueSet( sid,tsd.binidValueSet(),prov );
    }

    emobjsinglehor->unRef();
    
    return true;
}


bool uiWaveletExtraction::getSelData( const IOPar& rangepar,
				      const IOPar& surfacepar )
{
    if ( zextraction_->getBoolValue() )
    {
	if ( !linesel2dfld_ )
	{
	    sd_ = Seis::SelData::get( rangepar );
	    if ( !sd_ ) return false;

	    StepInterval<float> zrg = zrangefld_->getRange();
	    sd_->setZRange( zrg );
	}
    }
    else
    {
	Seis::TableSelData* tsd = new Seis::TableSelData;
	if ( !fillHorizonSelData( rangepar, surfacepar, *tsd ) )
	    return false;
	sd_ = tsd;
    }

    return true;
}


MultiID uiWaveletExtraction::storeKey() const
{
    return wvltctio_.ioobj ? wvltctio_.ioobj->key() : MultiID("");
}
