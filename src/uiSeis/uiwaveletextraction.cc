/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "od_helpids.h"


uiWaveletExtraction::uiWaveletExtraction( uiParent* p, bool is2d )
    : uiDialog( p,Setup(tr("Extract Wavelet"),mNoDlgTitle,
                        mODHelpKey(mWaveletExtractionHelpID) )
	             .modal(false) )
    , wvltsize_(0)
    , zrangefld_(0)
    , extractionDone(this)
    , seldata_(0)
    , seisselfld_(0)
    , linesel2dfld_(0)
    , subselfld3d_(0)
    , datastep_(SI().zStep())
{
    setCtrlStyle( RunAndClose );
    const Seis::GeomType gt = Seis::geomTypeOf( is2d, false );
    seisselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,true),
				 uiSeisSel::Setup(gt) );
    seisselfld_->selectionDone.notify(
			mCB(this,uiWaveletExtraction,inputSelCB) );
    if ( !is2d )
    {
	subselfld3d_ = new uiSeis3DSubSel( this, Seis::SelSetup(false,false)
					         .onlyrange(true)
					         .withstep(true)
					         .withoutz(true) );
	subselfld3d_->attach( alignedBelow, seisselfld_ );
    }
    else
    {
	linesel2dfld_ = new uiSeis2DMultiLineSel( this, 0, false, true );
	linesel2dfld_->attach( alignedBelow, seisselfld_ );
	linesel2dfld_->selectionChanged.notify(
			mCB(this,uiWaveletExtraction,lineSelCB) );
    }

    createCommonUIFlds();
    postFinalise().notify( mCB(this,uiWaveletExtraction,choiceSelCB) );
}


void uiWaveletExtraction::createCommonUIFlds()
{
    zextraction_ = new uiGenInput( this, tr("Vertical Extraction"),
			BoolInpSpec(linesel2dfld_,tr("Z range"),
                                    uiStrings::sHorizons(true)) );
    zextraction_->valuechanged.notify(
				mCB(this,uiWaveletExtraction,choiceSelCB) );

    linesel2dfld_ ? zextraction_->attach( alignedBelow, linesel2dfld_ )
		  : zextraction_->attach( alignedBelow, subselfld3d_ );

    zrangefld_ = new uiSelZRange( this, false, false, "Z Range " );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface(), this,
			 uiPosProvGroup::Setup(linesel2dfld_,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    BufferString lbl( "Wavelet length ", SI().getZUnitString() );
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, surfacesel_ );

    BufferString taperlbl( "Taper length ", SI().getZUnitString() );
    taperfld_ = new uiGenInput( this, taperlbl, IntInpSpec(20) );
    taperfld_->attach( alignedBelow, wtlengthfld_ );

    wvltphasefld_ = new uiGenInput( this, tr("Phase (Degrees)"),
                                    IntInpSpec(0) );
    wvltphasefld_->attach( alignedBelow, taperfld_ );

    IOObjContext wvltctxt( mIOObjContext(Wavelet) );
    wvltctxt.forread = false;
    outputwvltfld_ = new uiIOObjSel( this, wvltctxt );
    outputwvltfld_->attach( alignedBelow, wvltphasefld_ );

    postFinalise().notify( mCB(this,uiWaveletExtraction,inputSelCB) );
}


uiWaveletExtraction::~uiWaveletExtraction()
{
    delete seldata_;
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
    if ( linesel2dfld_ )
    {
	linesel2dfld_->setInput( seisselfld_->key() );
	lineSelCB(0);
	return;
    }

    CubeSampling cs;
    if ( subselfld3d_ )
    {
	const SeisIOObjInfo si( seisselfld_->ioobj() );
	si.getRanges( cs );
	cs.hrg.step.inl() = cs.hrg.step.crl() = 10;
	subselfld3d_->uiSeisSubSel::setInput( cs );
	datastep_ = cs.zrg.step;
    }

    if ( zextraction_->getBoolValue() )
    {
	zrangefld_->setRangeLimits( cs.zrg );
	zrangefld_->setRange( cs.zrg );
    }
}


void uiWaveletExtraction::lineSelCB( CallBacker* )
{
    if ( !zextraction_->getBoolValue() )
	return;

    const SeisIOObjInfo si( seisselfld_->ioobj() );
    StepInterval<int> trcrg;
    StepInterval<float> commonzrg;
    TypeSet<Pos::GeomID> geomids;
    linesel2dfld_->getSelGeomIDs( geomids );
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	StepInterval<float> zrg( 0, 0, 1 );
	if ( !si.getRanges(geomids[idx],trcrg,zrg) )
	    return;

	if ( idx==0 )
	{
	    commonzrg = zrg;
	    datastep_ = zrg.step;
	}
	else
	{
	    if ( datastep_ != zrg.step )
	    {
		uiMSG().message( tr("Selected lines having different sample"
				    " intervals\n Please change selection") );
		return;
	    }

	    commonzrg.limitTo( zrg );
	}
    }

    if ( !geomids.isEmpty() && commonzrg.nrSteps() == 0 )
    {
	uiMSG().message( tr("No common Z Range in selected lines.\n"
			    "Please change selection") );
	return;
    }

    zrangefld_->setRangeLimits( commonzrg );
    zrangefld_->setRange( commonzrg );
}


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    const IOObj* seisioobj = seisselfld_->ioobj();
    if ( !seisioobj ) return false;

    const IOObj* wvltioobj = outputwvltfld_->ioobj();
    if ( !wvltioobj ) return false;

    if ( linesel2dfld_ && !check2DFlds() )
	return false;

    IOPar inputpars, surfacepars;
    seisselfld_->fillPar( inputpars );

    outputwvltfld_->fillPar( inputpars );

    if ( subselfld3d_ )
	subselfld3d_->fillPar( inputpars );

    if ( !checkWaveletSize() )
	return false;

    if ( !zextraction_->getBoolValue() && !surfacesel_->fillPar(surfacepars) )
	    return false;

    const int taperlen = taperfld_->getIntValue();
    const int wvltlen = wtlengthfld_->getIntValue();
    if ( (2*taperlen > wvltlen) || taperlen < 0 )
    {
	uiMSG().error( tr("TaperLength should be in between\n",
			  "0 and half the Wavelet Length") );
	taperfld_->setValue( 5 );
	return false;
    }

    if ( wvltphasefld_->getIntValue()<-180 || wvltphasefld_->getIntValue()>180)
    {
	uiMSG().error( tr("Please enter Phase between -180 and 180") );
	wvltphasefld_->setValue( 0 );
	return false;
    }

    if ( outputwvltfld_->existingTyped() )
	outputwvltfld_->setConfirmOverwrite( true );

    doProcess( *seisioobj, *wvltioobj, inputpars, surfacepars );
    return false;
}


bool uiWaveletExtraction::checkWaveletSize()
{
    wvltsize_ = mNINT32( wtlengthfld_->getIntValue() /
		      (datastep_ * ((float) SI().zDomain().userFactor()))) + 1;
    if ( wvltsize_ < 3 )
    {
	uiMSG().error( tr("Minimum 3 samples are required to create Wavelet") );
	wtlengthfld_->setValue( 120 );
	return false;
    }

    if ( zextraction_->getBoolValue() )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	const int range = 1 + mNINT32( (zrg.stop - zrg.start) / datastep_ );
	if ( range < wvltsize_ )
	{
	    uiMSG().message( tr("Selection window size should be more"
				" than Wavelet Size") );
	    return false;
	}
    }

    return true;
}


bool uiWaveletExtraction::check2DFlds()
{
    if ( !linesel2dfld_->nrSelected() )
    {
	uiMSG().error( tr("Select at least one line") );
	return false;
    }

    if ( linesel2dfld_ && !zextraction_->getBoolValue() )
    {
	uiMSG().message( tr("Extraction of wavelet on/between 2D-horizon(s)"
			    " is not implemented") );
	return false;
    }

    return true;
}


bool uiWaveletExtraction::doProcess( const IOObj& seisioobj,
	const IOObj& wvltioobj, const IOPar& rangepar, const IOPar& surfacepar )
{
    const int phase = wvltphasefld_->getIntValue();
    PtrMan<WaveletExtractor> extractor= new WaveletExtractor( seisioobj,
							      wvltsize_ );
    if ( !linesel2dfld_ )
    {
	if ( !getSelData(rangepar,surfacepar) || !seldata_ )
	    return false;

	extractor->setSelData( *seldata_ );
    }
    else
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	Seis::RangeSelData range;
	range.setZRange( zrg );
	Interval<int> inlrg( 0, 0 );
	range.cubeSampling().hrg.setInlRange( inlrg );

	ObjectSet<Seis::SelData> sdset;
	StepInterval<int> trcrg;

	TypeSet<Pos::GeomID> geomids;
	linesel2dfld_->getSelGeomIDs( geomids );
	const TypeSet<StepInterval<int> >&  trcrgs =
					linesel2dfld_->getTrcRanges();
	for ( int lidx=0; lidx<geomids.size(); lidx++ )

	{
	    range.cubeSampling().hrg.setCrlRange( trcrgs[lidx] );
	    range.setGeomID( geomids[lidx] );
	    seldata_ = range.clone();
	    sdset += seldata_;
	}

	extractor->setSelData( sdset );
    }

    const int taperlength = taperfld_->getIntValue();
    const float val =
	  1-(2*taperlength/( (wvltsize_-1)*datastep_*
		      ((float)SI().zDomain().userFactor())) );
    const float paramval = (float) ( val == 1 ? 1.0 - 1e-6 : val );
    extractor->setTaperParamVal( paramval );
    extractor->setPhase( phase );

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *extractor ) )
	return false;

    Wavelet storewvlt = extractor->getWavelet();
    storewvlt.put( &wvltioobj );
    extractionDone.trigger();

    return true;
}


bool uiWaveletExtraction::fillHorizonSelData( const IOPar& rangepar,
					      const IOPar& surfacepar,
					      Seis::TableSelData& tsd )
{
    const char* extrazkey = IOPar::compKey( sKey::Surface(),
					  Pos::EMSurfaceProvider::extraZKey());
    Interval<float> extz( 0, 0 );
    if ( surfacepar.get(extrazkey,extz) )
	tsd.extendZ( extz );

    Pos::Provider3D* prov = Pos::Provider3D::make( rangepar );
    BufferString surfkey = IOPar::compKey( sKey::Surface(),
					   Pos::EMSurfaceProvider::id1Key() );
    MultiID surf1mid, surf2mid;
    if ( !surfacepar.get( surfkey.buf(), surf1mid ) )
	return false;

    surfkey = IOPar::compKey( sKey::Surface(),
			      Pos::EMSurfaceProvider::id2Key() );
    const bool betweenhors = surfacepar.get( surfkey.buf(), surf2mid );

    if ( !betweenhors )
    {
	const int size = int ( 1+(extz.stop-extz.start)/datastep_ );
	if ( size < wvltsize_ )
	{
	    uiMSG().error( tr("Selection window size should be"
		              " more than Wavelet size") );
	    return false;
	}
    }

    uiTaskRunner dlg( this );
    EM::EMObject* emobjsinglehor =
	EM::EMM().loadIfNotFullyLoaded( surf1mid, &dlg );

    if ( !emobjsinglehor )
	return false;

    emobjsinglehor->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon1,emobjsinglehor)
    if ( !horizon1 )
    {
	uiMSG().error( tr("Error loading horizon") );
	return false;
    }

    if ( betweenhors )
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	EM::EMObject* emobjdoublehor =
	    EM::EMM().loadIfNotFullyLoaded( surf2mid, &dlg );

	if ( !emobjdoublehor )
	    return false;

	emobjdoublehor->ref();
	mDynamicCastGet( EM::Horizon3D*, horizon2,emobjdoublehor )
	if ( !horizon2 )
	{
	    uiMSG().error( tr("Error loading second horizon") );
	    return false;
	}

	EM::SectionID sid2 = horizon2->sectionID( 0 );
	BinIDValueSet& bvs = tsd.binidValueSet();
	bvs.allowDuplicateBinIDs( true );
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
	    seldata_ = Seis::SelData::get( rangepar );
	    if ( !seldata_ ) return false;

	    StepInterval<float> zrg = zrangefld_->getRange();
	    seldata_->setZRange( zrg );
	}
    }
    else
    {
	Seis::TableSelData* tsd = new Seis::TableSelData;
	if ( !fillHorizonSelData( rangepar, surfacepar, *tsd ) )
	    return false;

	seldata_ = tsd;
    }

    return true;
}


MultiID uiWaveletExtraction::storeKey() const
{
    const IOObj* wvltioobj = outputwvltfld_->ioobj( true );
    return wvltioobj ? wvltioobj->key() : MultiID("");
}
