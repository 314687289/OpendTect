/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratlayermodel.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "executor.h"
#include "hiddenparam.h"
#include "ioobj.h"
#include "ioman.h"
#include "pixmap.h"
#include "strmprov.h"
#include "settings.h"
#include "separstr.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "strattransl.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uielasticpropsel.h"
#include "uiobjdisposer.h"
#include "uiioobjsel.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitoolbar.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimultiflatviewcontrol.h"
#include "uisaveimagedlg.h"
#include "uispinbox.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratsimplelaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uistratsynthcrossplot.h"
#include "uistratlaymodtools.h"
#include "uistrattreewin.h"
#include "uistatusbar.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uichecklist.h"

mDefineInstanceCreatedNotifierAccess(uiStratLayerModel)


HiddenParam<uiStratLayerModel,const char*> needtoretrievefrpars( "false" );

const char* uiStratLayerModel::sKeyModeler2Use()
{
    return "dTect.Stratigraphic Modeler to use";
}


class uiStratLayerModelManager : public CallBacker
{
public:

uiStratLayerModelManager()
    : dlg_(0)
{
    IOM().surveyToBeChanged.notify(mCB(this,uiStratLayerModelManager,survChg));
    IOM().applicationClosing.notify(mCB(this,uiStratLayerModelManager,survChg));
}


void survChg( CallBacker* )
{
    if ( dlg_ )
	dlg_->saveGenDescIfNecessary( false );
    delete dlg_; dlg_ = 0;
}

void winClose( CallBacker* )
{
    dlg_ = 0;
}

void startCB( CallBacker* cb )
{
    if ( haveExistingDlg() )
	return;
    const BufferStringSet& nms =
			uiLayerSequenceGenDesc::factory().getNames( true );
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( Strat::RT().isEmpty() || nms.isEmpty() || !tb )
	{ pErrMsg("Pre-condition not met"); return; }

    uiParent* par = tb->parent();
    const char* settres = Settings::common().find(
	    			uiStratLayerModel::sKeyModeler2Use());
    BufferString modnm( settres );
    int defmodnr = -1;
    bool givechoice = nms.size() > 1;
    if ( modnm.isEmpty() )
	modnm = nms.get( nms.size() - 1 );
    else
    {
	FileMultiString fms( modnm );
	modnm = fms[0];
	defmodnr = nms.indexOf( modnm.buf() );
	if ( defmodnr < 0 )
	    modnm.setEmpty();
	else
	{
	    const bool alwayswant = fms.size() > 1 && *fms[1] == 'A';
	    givechoice = givechoice && !alwayswant;
	}
    }

    if ( givechoice )
    {
	uiSelectFromList::Setup sflsu( "Select modeling type", nms );
	sflsu.current( defmodnr < 0 ? nms.size()-1 : defmodnr );
	uiSelectFromList dlg( par, sflsu );
	uiCheckList* defpol = new uiCheckList( &dlg, "Set as default",
				"Always use this type", uiCheckList::Chain1st );
	defpol->setChecked( 0, defmodnr >= 0 );
	defpol->attach( centeredBelow, dlg.selFld() );
	if ( !dlg.go() ) return;

	const int sel = dlg.selection();
	if ( sel < 0 ) return;
	const BufferString newmodnm = nms.get( sel );
	int indic = defpol->isChecked(0) ? (defpol->isChecked(1) ? 2 : 1) : 0;
	bool needwrite = true;
	if ( indic == 0 )
	{
	    if ( defmodnr < 0 )
		needwrite = false;
	    else
		Settings::common().removeWithKey(
				    uiStratLayerModel::sKeyModeler2Use() );
	}
	else
	{
	    if ( indic == 2 || defmodnr < 0 || modnm != newmodnm )
	    {
		Settings::common().set( uiStratLayerModel::sKeyModeler2Use(),
			BufferString(newmodnm, indic == 2 ? "`Always" : "") );
	    }
	    else if ( defmodnr >= 0 )
		needwrite = false;
	}

	if ( needwrite )
	    Settings::common().write( false );
	modnm = newmodnm;
    }
    doLayerModel( par, modnm );
}

bool haveExistingDlg()
{
    if ( dlg_ )
    {
	uiMSG().error( "Please exit your other layer modeling window first" );
	dlg_->raise();
	return true;
    }
    return false;
}

void doLayerModel( uiParent* p, const char* modnm )
{
    if ( haveExistingDlg() || Strat::RT().isEmpty() )
	return;

    dlg_ = new uiStratLayerModel( p, modnm );
    dlg_->windowClosed.notify(mCB(this,uiStratLayerModelManager,winClose));
    dlg_->go();
}

void addToTreeWin()
{
    uiToolButtonSetup* su = new uiToolButtonSetup( "stratlayermodeling",
			    "Start layer/synthetics modeling",
			    mCB(this,uiStratLayerModelManager,startCB) );
    uiStratTreeWin::addTool( su );
}

    uiStratLayerModel*	dlg_;

};

static uiStratLayerModelManager& uislm_manager()
{
    static uiStratLayerModelManager theinst;
    return theinst;
}

void uiStratLayerModel::initClass()
{
    uislm_manager().addToTreeWin();
}


void uiStratLayerModel::doBasicLayerModel()
{
    doLayerModel( uiBasicLayerSequenceGenDesc::typeStr() );
}


void uiStratLayerModel::doLayerModel( const char* modnm )
{
    if ( Strat::RT().isEmpty() )
	StratTreeWin().popUp();
    else
	uislm_manager().doLayerModel( &StratTreeWin(), modnm );
}



class uiStratLayerModelLMProvider : public Strat::LayerModelProvider
{
public:

uiStratLayerModelLMProvider()
{
    modl_ = new Strat::LayerModel;
    setEmpty();
}

~uiStratLayerModelLMProvider()
{
    delete modl_;
}

Strat::LayerModel& getCurrent()
{
    return *curmodl_;
}

Strat::LayerModel& getEdited( bool yn )
{
    return yn ? modled_ : *modl_;
}

void setUseEdited( bool yn )
{
    curmodl_ = yn ? &modled_ : modl_;
}

void setEmpty()
{
    modl_->setEmpty();
    modled_.setEmpty();
    curmodl_ = modl_;
}

void setModel( Strat::LayerModel* newmdl )
{
    delete modl_;
    modl_ = newmdl;
    modled_ = *modl_;
    curmodl_ = modl_;
}

void initEditing()
{
    modled_ = *modl_;
    curmodl_ = &modled_;
}

    Strat::LayerModel*		curmodl_;
    Strat::LayerModel*		modl_;
    Strat::LayerModel		modled_;

};


uiStratLayerModel::uiStratLayerModel( uiParent* p, const char* edtyp )
    : uiMainWin(p,"",1,true)
    , desc_(*new Strat::LayerSequenceGenDesc(Strat::RT()))
    , elpropsel_(0)				   
    , descctio_(*mMkCtxtIOObj(StratLayerSequenceGenDesc))
    , analtb_(0)
    , zoomwr_(mUdf(double),0,0,0)
    , lmp_(*new uiStratLayerModelLMProvider)
    , automksynth_(true)
    , newModels(this)				   
    , levelChanged(this)				   
    , waveletChanged(this)
    , saveRequired(this) 
    , retrieveRequired(this) 
{
    setDeleteOnClose( true );
    needtoretrievefrpars.setParam( this, "false" );

    if ( !edtyp || !*edtyp )
	edtyp = uiBasicLayerSequenceGenDesc::typeStr();
    descctio_.ctxt.toselect.require_.set( sKey::Type(), edtyp );

    uiGroup* gengrp = new uiGroup( this, "Gen group" );
    seqdisp_ = uiLayerSequenceGenDesc::factory().create( edtyp, gengrp, desc_ );
    if ( !seqdisp_ )
	seqdisp_ = new uiBasicLayerSequenceGenDesc( gengrp, desc_ );

    uiGroup* topgrp; uiGroup* botgrp; uiGroup* rightgrp=0;
    if ( seqdisp_->separateDisplay() )
    {
	rightgrp = new uiGroup( this, "Right group" );
	topgrp = new uiGroup( rightgrp, "Top group" );
	botgrp = new uiGroup( rightgrp, "Bot group" );
    }
    else
    {
	topgrp = new uiGroup( this, "Top group" );
	botgrp = gengrp;
    }

    modtools_ = new uiStratLayModEditTools( botgrp );
    gentools_ = new uiStratGenDescTools( gengrp );
    moddisp_ = seqdisp_->getLayModDisp( *modtools_, lmp_ );
    if ( !moddisp_ )
	return;

    synthdisp_ = new uiStratSynthDisp( topgrp, lmp_ );
    analtb_ = new uiToolBar( this, "Analysis toolbar", uiToolBar::Right );
    uiToolButtonSetup tbsu( "xplot", "Attributes vs model properties",
	   		    mCB(this,uiStratLayerModel,xPlotReq) );
    synthdisp_->control()->getToolBar(0)->addButton(
	    "snapshot", "Get snapshot", mCB(this,uiStratLayerModel,snapshotCB));
    synthdisp_->synthsChanged.notify(
	    	mCB(this,uiStratLayerModel,syntheticsChangedCB) );
    analtb_->addButton( tbsu );
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr ) synthdisp_->addViewerToControl( *vwr );

    modtools_->attach( ensureBelow, moddisp_ );
    gentools_->attach( ensureBelow, seqdisp_->outerObj() );

    uiToolBar* helptb = new uiToolBar( this, "Help toolbar", uiToolBar::Right );
    uiToolButtonSetup htbsu( "contexthelp", "Help",
	    		     mCB(this,uiStratLayerModel,helpCB) );
    helptb->addButton( htbsu );

    uiSplitter* hspl;
    if ( !seqdisp_->separateDisplay() )
    {
	modtools_->attach( rightOf, gentools_ );
	hspl = new uiSplitter( this, "Hor splitter", false );
    }
    else
    {
	modtools_->attach( rightBorder );
	uiSplitter* vspl = new uiSplitter( this, "Vert splitter", true );
	vspl->addGroup( gengrp ); vspl->addGroup( rightgrp );
	hspl = new uiSplitter( rightgrp, "Hor splitter", false );
    }
    hspl->addGroup( topgrp ); hspl->addGroup( botgrp );

    modtools_->dispEachChg.notify( mCB(this,uiStratLayerModel,dispEachChg) );
    modtools_->selLevelChg.notify( mCB(this,uiStratLayerModel,levelChg) );
    modtools_->flattenChg.notify( mCB(this,uiStratLayerModel,levelChg) );
    modtools_->mkSynthChg.notify( mCB(this,uiStratLayerModel,mkSynthChg) );
    gentools_->openReq.notify( mCB(this,uiStratLayerModel,openGenDescCB) );
    gentools_->saveReq.notify( mCB(this,uiStratLayerModel,saveGenDescCB) );
    gentools_->propEdReq.notify( mCB(this,uiStratLayerModel,manPropsCB) );
    gentools_->genReq.notify( mCB(this,uiStratLayerModel,genModels) );
    synthdisp_->wvltChanged.notify( mCB(this,uiStratLayerModel,wvltChg) );
    synthdisp_->zoomChanged.notify( mCB(this,uiStratLayerModel,zoomChg) );
    synthdisp_->viewChanged.notify( mCB(this,uiStratLayerModel,zoomChg) );
    synthdisp_->modSelChanged.notify( mCB(this,uiStratLayerModel,modSelChg) );
    synthdisp_->layerPropSelNeeded.notify(
			    mCB(this,uiStratLayerModel,selElasticPropsCB) );
    synthdisp_->control()->infoChanged.notify(
	    mCB(this,uiStratLayerModel,infoChanged) );
    moddisp_->genNewModelNeeded.notify( mCB(this,uiStratLayerModel,genModels) );
    moddisp_->rangeChanged.notify( 
			    mCB(this,uiStratLayerModel,modDispRangeChanged));
    moddisp_->infoChanged.notify( 
			    mCB(this,uiStratLayerModel,infoChanged));
    moddisp_->sequenceSelected.notify( mCB(this,uiStratLayerModel,seqSel) );
    moddisp_->modelEdited.notify( mCB(this,uiStratLayerModel,modEd) );

    setWinTitle();
    StratTreeWin().changeLayerModelNumber( true );
    postFinalise().notify( mCB(this,uiStratLayerModel,initWin) );
}


uiStratLayerModel::~uiStratLayerModel()
{
    delete &desc_;
    delete &lmp_;
    delete descctio_.ioobj; delete &descctio_;
    StratTreeWin().changeLayerModelNumber( false );
}


void uiStratLayerModel::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


void uiStratLayerModel::setWinTitle()
{
    BufferString txt( "Layer modeling" );
    if ( descctio_.ioobj )
	txt.add( " [" ).add( descctio_.ioobj->name() ).add( "]" );
    setCaption( txt );
}


const char* uiStratLayerModel::levelName() const
{
    const char* nm = modtools_->selLevel();
    return !nm || !*nm || (*nm == '-' && *(nm+1) == '-') ? 0 : nm;
}


const SeisTrcBuf& uiStratLayerModel::postStackTraces() const
{
    return synthdisp_->postStackTraces();
}


const SeisTrcBuf& uiStratLayerModel::postStackTraces( const char* nm ) const
{
    return synthdisp_->postStackTraces( nm );
}


const SeisTrcBuf& uiStratLayerModel::modelTraces( const PropertyRef& pr ) const
{
    return synthdisp_->postStackTraces( &pr );
}


const StratSynth& uiStratLayerModel::currentStratSynth() const
{
    return const_cast<const uiStratSynthDisp*>(synthdisp_)->curSS();
}


const PropertyRefSelection& uiStratLayerModel::modelProperties() const
{
    return synthdisp_->modelPropertyRefs();
}


const ObjectSet<const TimeDepthModel>& uiStratLayerModel::d2TModels() const
{
    static ObjectSet<const TimeDepthModel> empty;
    const ObjectSet<const TimeDepthModel>* ret = synthdisp_->d2TModels();
    return ret ? *ret : empty;
}


const Wavelet* uiStratLayerModel::wavelet() const
{
    return synthdisp_->getWavelet();
}


void uiStratLayerModel::initWin( CallBacker* cb )
{
    if ( !moddisp_ )
    {
	modtools_->display( false );
	gentools_->display( false );
	uiOBJDISP()->go( this );
    }
    mTriggerInstanceCreatedNotifier();
}


void uiStratLayerModel::dispEachChg( CallBacker* )
{
    synthdisp_->setDispEach( modtools_->dispEach() );
    levelChg( 0 );
}


bool uiStratLayerModel::canShowFlattened() const
{
    TypeSet<float> zlvls = moddisp_->levelDepths();
    for ( int idx=0; idx<zlvls.size(); idx++ )
	if ( !mIsUdf(zlvls[idx]) ) return true;
    return false;
}


void uiStratLayerModel::mkSynthChg( CallBacker* cb )
{
    automksynth_ = modtools_->mkSynthetics();
    synthdisp_->setAutoUpdate( automksynth_ );
    if ( automksynth_ )
    {
	uiWorldRect zoomwr = synthdisp_->curView( false );
	useSyntheticsPars( desc_.getWorkBenchParams() );
	synthdisp_->modelChanged();
	mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
	if ( mfvc ) mfvc->reInitZooms();
	zoomwr_ = zoomwr;
	synthdisp_->setZoomView( zoomwr_ );
    }
}


void uiStratLayerModel::levelChg( CallBacker* cb )
{
    moddisp_->setFlattened( modtools_->showFlattened() );

    const bool canshowflattened = canShowFlattened();
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor(), modtools_->showFlattened() );
    synthdisp_->setSnapLevelSensitive( canshowflattened );
    if ( cb )
	levelChanged.trigger();
}


void uiStratLayerModel::modSelChg( CallBacker* cb )
{
    mCBCapsuleUnpack(int,modidx,cb);
    moddisp_->selectSequence( modidx -1 );
}


void uiStratLayerModel::zoomChg( CallBacker* )
{
    uiWorldRect wr( mUdf(double), 0, 0, 0 );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), false );
    if ( synthdisp_->getSynthetics().size() )
	wr = synthdisp_->curView( true );
    moddisp_->setZoomBox( wr );
    zoomwr_ = synthdisp_->curView( false );
}


bool uiStratLayerModel::checkUnscaledWavelet()
{
    const Wavelet* wvlt = synthdisp_->getWavelet();
    if ( !wvlt )
	return false;
    if ( Wavelet::isScaled(synthdisp_->waveletID()) )
	return true;

    BufferStringSet opts;
    opts.add( "[&Start tool]: Start the wavelet scaling dialog" );
    opts.add( "[&Mark scaled]: The wavelet amplitude is already compatible "
	      "with the seismic data" );
    opts.add( "[&Ignore]: I will not use scaling-sensitive operations" );
    uiGetChoice dlg( this, opts,
	    "The wavelet seems to be unscaled.\n"
	    "For most purposes, you will need a scaled wavelet.\n", true );
    dlg.setHelpID( "110.2.2" );
    dlg.go(); const int choice = dlg.choice();
    if ( choice < 0 )
	return false;
    else if ( choice == 2 )
	return true;
    else if ( choice == 1 )
    {
	Wavelet::markScaled( synthdisp_->waveletID() );
	return true;
    }

    return synthdisp_->haveUserScaleWavelet();
}


void uiStratLayerModel::xPlotReq( CallBacker* )
{
    if ( !checkUnscaledWavelet() )
	return;

    if ( !synthdisp_->getSynthetics().size() ) return;

    uiStratSynthCrossplot dlg( this, layerModel(),synthdisp_->getSynthetics());
    if ( dlg.errMsg() )
	{ uiMSG().error( dlg.errMsg() ); return; } 
    const char* lvlnm = modtools_->selLevel();
    if ( lvlnm && *lvlnm ) dlg.setRefLevel( lvlnm );
    dlg.go();
}


void uiStratLayerModel::wvltChg( CallBacker* cb )
{
    zoomChg( cb );
    levelChg( 0 );
    waveletChanged.trigger();
}


void uiStratLayerModel::modDispRangeChanged( CallBacker* )
{
    mDynamicCastGet( uiFlatViewer*,vwr,moddisp_->getViewer());
    if ( vwr ) 
	synthdisp_->setZDataRange( vwr->getSelDataRange( false ) , true );
}


void uiStratLayerModel::manPropsCB( CallBacker* )
{
    seqdisp_->selProps();
}


void uiStratLayerModel::selElasticPropsCB( CallBacker* )
{
    if ( !elpropsel_ )
	elpropsel_ = new ElasticPropSelection;
    if ( selElasticProps( *elpropsel_ ) )
	genModels(0);
}


bool uiStratLayerModel::selElasticProps( ElasticPropSelection& elsel )
{
    uiElasticPropSelDlg dlg( this, desc_.propSelection(), elsel );
    if ( dlg.go() )
    {
	if ( dlg.propSaved() )
	{
	    desc_.setElasticPropSel( dlg.storedKey() );
	    seqdisp_->setNeedSave( true );
	}

	elsel.fillPar( desc_.getWorkBenchParams() );
	return true;
    }
    return false;
}


bool uiStratLayerModel::saveGenDescIfNecessary( bool allowcncl ) const
{
    if ( !seqdisp_->needSave() )
	return true;

    while ( true )
    {
	const int res = uiMSG().askSave( "Generation description not saved.\n"
					 "Save now?" );
	if ( !allowcncl && res < 0 )
	{
	    uiMSG().error( "Sorry, you cannot cancel right now."
		    	   "Please save or discard your work" );
	    continue;
	}
	if ( res < 1 )
	    return res == 0;
	break;
    }

    return saveGenDesc();
}


bool uiStratLayerModel::saveGenDesc() const
{
    descctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( const_cast<uiStratLayerModel*>(this), descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(false) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    bool rv = false;
    MouseCursorChanger mcch( MouseCursor::Wait );
    
    
    fillWorkBenchPars( desc_.getWorkBenchParams() );
    
    if ( !sd.usable() )
	uiMSG().error( "Cannot open output file" );
    else if ( !desc_.putTo(*sd.ostrm) )
	uiMSG().error(desc_.errMsg());
    else
    {
	rv = true;
	seqdisp_->setNeedSave( false );
	const_cast<uiStratLayerModel*>(this)->setWinTitle();
    }

    sd.close();
    return rv;
}


bool uiStratLayerModel::openGenDesc()
{
    if ( !saveGenDescIfNecessary() )
	return false;

    descctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, descctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    descctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( descctio_.ioobj->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot open input file" ); return false; }

    delete elpropsel_; elpropsel_ = 0;
    desc_.erase();
    MouseCursorChanger mcch( MouseCursor::Wait );
    bool rv = desc_.getFrom( *sd.istrm );
    if ( !rv )
	uiMSG().error(desc_.errMsg());
    sd.close();
    
    //Before calculation
    if ( !gentools_->usePar( desc_.getWorkBenchParams() ) )
	return false;
    
    if ( !rv )
	return false;

    seqdisp_->setNeedSave( false );
    lmp_.setEmpty();
    seqdisp_->descHasChanged();

    delete elpropsel_; elpropsel_ = 0;
    
    BufferString edtyp;
    descctio_.ctxt.toselect.require_.get( sKey::Type(), edtyp );
    BufferString profilestr( "Profile" );
    if ( !profilestr.isStartOf(edtyp) )
    {
	needtoretrievefrpars.setParam( this, "true" );
	gentools_->genReq.trigger();
	//Set when everything is in place.
    }
    else
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
    }


    if ( !useDisplayPars( desc_.getWorkBenchParams() ))
	return false;

    if ( GetEnvVarYN("DTECT_EXPORT_LAYERMODEL") )
    {
	if ( !exportLayerModelGDI( fnm ) )
	    return false;
    }
    
    //Before calculation
    if ( !gentools_->usePar( desc_.getWorkBenchParams() ) )
    setWinTitle();
    return true;
}


MultiID uiStratLayerModel::genDescID() const
{
    MultiID ret;
    if ( descctio_.ioobj )
	ret = descctio_.ioobj->key();
    return ret;
}


void uiStratLayerModel::seqSel( CallBacker* )
{
    synthdisp_->setSelectedTrace( moddisp_->selectedSequence() );
}


void uiStratLayerModel::modEd( CallBacker* )
{
    uiWorldRect zoomwr = synthdisp_->curView( false );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), true );
    useSyntheticsPars( desc_.getWorkBenchParams() );
    synthdisp_->modelChanged();
    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc ) mfvc->reInitZooms();
    zoomwr_ = zoomwr;
    synthdisp_->setZoomView( zoomwr_ );
}


void uiStratLayerModel::genModels( CallBacker* )
{
    const int nrmods = gentools_->nrModels();
    if ( nrmods < 1 )
	{ uiMSG().error("Please enter a valid number of models"); return; }

    const int prevnrmods = lmp_.getCurrent().size();
    bool autoupdatechged = false;
    if ( prevnrmods != nrmods && !automksynth_ )
    {
	automksynth_ = true;
	autoupdatechged = true;
	synthdisp_->setAutoUpdate( automksynth_ );
    }

    Strat::LayerModel* newmodl = new Strat::LayerModel;
    newmodl->propertyRefs() = seqdisp_->desc().propSelection();
    newmodl->setElasticPropSel( lmp_.getCurrent().elasticPropSel() );

    seqdisp_->prepareDesc();
    Strat::LayerModelGenerator ex( desc_, *newmodl, nrmods );
    uiTaskRunner tr( this );
    if ( !tr.execute(ex) )
	{ delete newmodl; return; }

    // transaction succeeded, we move to the new model - period.
    if ( !newmodl->isValid() )
    {
	delete newmodl;
	return uiMSG().error("Layer model is empty" );
    }

    lmp_.setModel( newmodl );
    lmp_.setUseEdited( false );
    synthdisp_->setUseEdited( false );
    moddisp_->setFluidReplOn( false );

    setModelProps();
    setElasticProps();
    useSyntheticsPars( desc_.getWorkBenchParams() );
    synthdisp_->setDisplayZSkip( moddisp_->getDisplayZSkip(), true );
    moddisp_->modelChanged();

    if ( needtoretrievefrpars.getParam(this) == FixedString("true") )
    {
	CBCapsule<IOPar*> caps( &desc_.getWorkBenchParams(),
				const_cast<uiStratLayerModel*>(this) );
	const_cast<uiStratLayerModel*>(this)->retrieveRequired.trigger( &caps );
	needtoretrievefrpars.setParam( this, "false" );
    }
    else
	levelChg( 0 );

    newModels.trigger();

    uiWorldRect zoomwr = synthdisp_->curView( false );
    mDynamicCastGet(uiMultiFlatViewControl*,mfvc,synthdisp_->control());
    if ( mfvc ) mfvc->reInitZooms();
    zoomwr_ = zoomwr;
    synthdisp_->setZoomView( zoomwr_ );

    if ( autoupdatechged )
    {
	automksynth_ = false;
	synthdisp_->setAutoUpdate( automksynth_ );
    }
}


void uiStratLayerModel::setModelProps()
{
    BufferStringSet nms;
    const Strat::LayerModel& lm = lmp_.getCurrent();
    for ( int idx=1; idx<lm.propertyRefs().size(); idx++ )
	nms.add( lm.propertyRefs()[idx]->name() );
    modtools_->setProps( nms );
    nms.erase(); const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
	nms.add( lvls.levels()[idx]->name() );
    modtools_->setLevelNames( nms );
    nms.erase();
    const Strat::ContentSet& conts = lm.refTree().contents();
    for ( int idx=0; idx<conts.size(); idx++ )
	nms.add( conts[idx]->name() );
    modtools_->setContentNames( nms );
}


void uiStratLayerModel::setElasticProps()
{
    if ( !elpropsel_ )
    {
	elpropsel_ = new ElasticPropSelection;
	if ( !elpropsel_->usePar(desc_.getWorkBenchParams()) )
	{
	    delete elpropsel_;
	    elpropsel_ = 0;
	}
    }

    if ( !elpropsel_ )
    {
	elpropsel_ = ElasticPropSelection::get( desc_.elasticPropSel() );
	if ( !elpropsel_ )
	{
	    elpropsel_ = new ElasticPropSelection;
	    ElasticPropGuess( desc_.propSelection(), *elpropsel_ );
	}
    }

    BufferString errmsg;
    if ( !elpropsel_->isValidInput( &errmsg) )
    {
	if ( !errmsg.isEmpty() )
	{
	    errmsg += "\nPlease define a new value. ";
	    uiMSG().message( errmsg.buf() );
	}
	if ( !selElasticProps( *elpropsel_ ) )
	    return;
    }

    lmp_.getEdited(true).setElasticPropSel( *elpropsel_ );
    lmp_.getEdited(false).setElasticPropSel( *elpropsel_ );
}


bool uiStratLayerModel::closeOK()
{
    return saveGenDescIfNecessary();
}


void uiStratLayerModel::displayFRResult( bool usefr, bool parschanged, bool fwd )
{
    lmp_.setUseEdited( usefr );
    mostlyfilledwithbrine_ = !fwd;
    if ( !usefr )
	mostlyfilledwithbrine_ = !mostlyfilledwithbrine_;

    uiWorldRect prevzoomwr = zoomwr_;
    synthdisp_->setUseEdited( usefr );
    if ( parschanged ) 
	useSyntheticsPars( desc_.getWorkBenchParams() );
    synthdisp_->modelChanged();
    synthdisp_->setDispMrkrs( modtools_->selLevel(), moddisp_->levelDepths(),
		    modtools_->selLevelColor(), modtools_->showFlattened() );

    moddisp_->setBrineFilled( fwd );
    moddisp_->setFluidReplOn( usefr );
    moddisp_->modelChanged();
    if ( !mIsUdf(prevzoomwr.left()) )
	synthdisp_->setZoomView( prevzoomwr );
}


void uiStratLayerModel::prepareFluidRepl()
{
    lmp_.initEditing();
    Strat::LayerModel& edlm = lmp_.getEdited( true );
    if ( edlm.propertyRefs().find(Strat::LayerModel::defSVelStr()) == -1 )
	edlm.propertyRefs() += new PropertyRef( Strat::LayerModel::defSVelStr(),
					 PropertyRef::Vel );
}


const Strat::LayerModel& uiStratLayerModel::layerModelOriginal() const
{
    return lmp_.getEdited( false );
}


Strat::LayerModel& uiStratLayerModel::layerModelOriginal()
{
    return lmp_.getEdited( false );
}


const Strat::LayerModel& uiStratLayerModel::layerModelEdited() const
{
    return lmp_.getEdited( true );
}


Strat::LayerModel& uiStratLayerModel::layerModelEdited()
{
    return lmp_.getEdited( true );
}


const Strat::LayerModel& uiStratLayerModel::layerModel() const
{
    return lmp_.getCurrent();
}


Strat::LayerModel& uiStratLayerModel::layerModel()
{
    return lmp_.getCurrent();
}


bool uiStratLayerModel::useDisplayPars( const IOPar& par )
{
    if ( !modtools_->usePar( par ) )
	return false;
    
    return true;
}

    

void uiStratLayerModel::fillWorkBenchPars( IOPar& par ) const
{
    par.setEmpty();
    CBCapsule<IOPar*> caps( &par, const_cast<uiStratLayerModel*>(this) );
    const_cast<uiStratLayerModel*>(this)->saveRequired.trigger( &caps );
    gentools_->fillPar( par );
    if ( elpropsel_ )
	elpropsel_->fillPar( par );
    fillDisplayPars( par );
    fillSyntheticsPars( par );
}


bool uiStratLayerModel::useSyntheticsPars( const IOPar& par ) 
{
    if ( !automksynth_ || !synthdisp_->prepareElasticModel() )
	return false;
    return synthdisp_->usePar( par );
}


void uiStratLayerModel::fillSyntheticsPars( IOPar& par ) const
{
    synthdisp_->fillPar( par );
}


void uiStratLayerModel::fillDisplayPars( IOPar& par ) const
{
    modtools_->fillPar( par );
}


void uiStratLayerModel::helpCB( CallBacker* )
{ uiMainWin::provideHelp( "110.2.0" ); }


void uiStratLayerModel::syntheticsChangedCB( CallBacker* )
{
    fillSyntheticsPars( desc_.getWorkBenchParams() );
}


void uiStratLayerModel::infoChanged( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(IOPar,pars,caller,cb);
    mDynamicCastGet(uiStratLayerModelDisp*,moddisp,caller);
    if ( !moddisp )
    {
	BufferString mesg;
	uiFlatViewWin::makeInfoMsg( mesg, pars );
	statusBar()->message( mesg.buf() );
    }
    else
    {
	BufferString msg;
	for ( int idx=0; idx<pars.size(); idx++ )
	{
	    msg += pars.getKey( idx );
	    msg +=": ";
	    msg += pars.getValue( idx );
	    msg += "\t";
	}
	statusBar()->message( msg.buf() );
    }
} 


bool uiStratLayerModel::exportLayerModelGDI(BufferString fnm) const
{
    BufferString fnmlm;
    fnmlm = fnm;
    fnmlm += ".txt";
    StreamData* outstreamdata =
       			  new StreamData( StreamProvider(fnmlm).makeOStream() );
    if ( !outstreamdata->usable() )
	{ uiMSG().error( "Cannot open '", fnmlm,"' for write" ); return false; }

    ascostream astrm( *outstreamdata->ostrm );
    const char* typ = "Well group";
    astrm.putHeader( typ );
    std::ostream& strm = astrm.stream();

    const int nrpswells = layerModel().size();
    BufferString str;
    str = "Name: ";
    str += nrpswells;
    str += " pseudowells";
    astrm << str << std::endl;
    strm << "!\n";

    for ( int iwell=0; iwell<nrpswells; iwell++ )
    {
	str = "Name: ";
	str += iwell + 1;
	str += "-pseudowell";
	astrm << str << std::endl;
	str = "Inline: ";
	const int inlnb = SI().inlRange(true).stop + SI().inlStep();
	str += inlnb;
	astrm << str << std::endl;
	str = "Crossline: ";
	const int crlnb = SI().crlRange(true).stop +
	   		  SI().crlStep() * ( iwell + 1 );
	str += crlnb;
	astrm << str << std::endl;
	str = "Rep Area: ";
	str += inlnb; str += "`"; str += inlnb; str += "`";
	str += crlnb; str += "`"; str += crlnb;
	astrm << str << std::endl;
	str = "Simulated: Yes";
	astrm << str << std::endl;
	strm << "!\n";

	str = "Reference depth: ";
	str += layerModel().sequence(iwell).startDepth();
	astrm << str << std::endl;
	const int nrvals = layerModel().sequence(iwell).layers()[0]->nrValues();
	str = "Compact: Yes";
	astrm << str << std::endl;
	BufferString propnm;
	for ( int ival=0; ival<nrvals; ival++ )
	{
	    str = "Quantity: ";
	    propnm = layerModel().propertyRefs()[ival]->name();
	    cleanupString( propnm.buf(), false, false, true );
	    str += propnm;
	    astrm << str << std::endl;
	}
	strm << "!\n";

	const int nrlayers = layerModel().sequence(iwell).size();
	BufferString prevunitnm;
	TypeSet<int> nrliths;
	TypeSet<BufferString> lithnms;
	for ( int ilayer=0; ilayer<nrlayers; ilayer++ )
	{
	    BufferString unitnm = layerModel().sequence(iwell).layers()[ilayer]
				   ->unitRef().parentCode().buf();
	    BufferString lith = layerModel().sequence(iwell).layers()[ilayer]
				->unitRef().code();
	    if ( unitnm == prevunitnm )
	    {
		const int idx = lithnms.isPresent(lith) ?
		   		lithnms.indexOf(lith) : 
				lithnms.size();
		if ( lithnms.isPresent(lith) )
		{
		    nrliths[idx]++;
		}
		else
		{
		    nrliths += 1;
		    lithnms += lith;
		}
	    }
	    else
	    {
		lithnms.erase();
		nrliths.erase();
		lithnms += lith;
		nrliths += 1;
	    }
	    prevunitnm = unitnm;
	    str = unitnm; str += "."; str += lith;
	    if ( nrliths[lithnms.indexOf(lith)] > 1 )
	    {
		str += "(";
		str += nrliths[lithnms.indexOf(lith)];
		str += ")";
	    }
	    if ( !layerModel().sequence(iwell).layers()[ilayer]->content().
		    name().isEmpty() )
	    {
		str+= ",";
		str+= layerModel().sequence(iwell).layers()[ilayer]
		      ->content().name();
	    }
	    for ( int ival=0; ival<nrvals; ival++ )
	    {
		str += " ";
		str+=layerModel().sequence(iwell).layers()[ilayer]->value(ival);
	    }
	    astrm << str << std::endl;
	}
	strm << "!\n!\n!\n";
    }
    const bool res = strm.good();
    outstreamdata->close();
    delete outstreamdata;
    outstreamdata = 0;

    return res;
}
