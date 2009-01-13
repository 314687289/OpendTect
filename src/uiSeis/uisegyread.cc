/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyread.cc,v 1.28 2009-01-13 13:52:02 cvsbert Exp $";

#include "uisegyread.h"
#include "uisegydef.h"
#include "uisegydefdlg.h"
#include "uisegyimpdlg.h"
#include "uisegyscandlg.h"
#include "uisegyexamine.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifileinput.h"
#include "uitaskrunner.h"
#include "uiobjdisposer.h"
#include "uimsg.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "segyscanner.h"
#include "segyfiledata.h"
#include "seisioobjinfo.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "filepath.h"
#include "timefun.h"

static const char* sKeySEGYRev1Pol = "SEG-Y Rev. 1 policy";

#define mSetState(st) { state_ = st; nextAction(); return; }


void uiSEGYRead::Setup::getDefaultTypes( TypeSet<Seis::GeomType>& geoms,
       					 bool forsisetup )
{
    if ( forsisetup || SI().has3D() )
    {
	geoms += Seis::Vol;
	geoms += Seis::VolPS;
    }
    if ( forsisetup || SI().has2D() )
    {
	geoms += Seis::Line;
	geoms += Seis::LinePS;
    }
}


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su,
			const IOPar* iop )
    : setup_(su)
    , parent_(p)
    , geom_(SI().has3D()?Seis::Vol:Seis::Line)
    , state_(su.initialstate_)
    , scanner_(0)
    , rev_(Rev0)
    , revpolnr_(2)
    , defdlg_(0)
    , newdefdlg_(0)
    , examdlg_(0)
    , impdlg_(0)
    , scandlg_(0)
    , rev1qdlg_(0)
    , processEnded(this)
{
    if ( iop )
	usePar( *iop );
    nextAction();
}

// Destructor at end of file (deleting local class)


void uiSEGYRead::closeDown()
{
    uiOBJDISP()->go( this );
    processEnded.trigger();
}


void uiSEGYRead::nextAction()
{
    if ( state_ <= Finished )
	{ closeDown(); return; }

    switch ( state_ )
    {
    case Wait4Dialog:
	return;
    break;
    case BasicOpts:
	getBasicOpts();
    break;
    case SetupImport:
	setupImport();
    break;
    case SetupScan:
	setupScan();
    break;
    }
}


void uiSEGYRead::setGeomType( const IOObj& ioobj )
{
    bool is2d = false; bool isps = false;
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIs2D, is2d );
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIsPS, isps );
    geom_ = Seis::geomTypeOf( is2d, isps );
}


void uiSEGYRead::use( const IOObj* ioobj, bool force )
{
    if ( !ioobj ) return;

    pars_.merge( ioobj->pars() );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
	setGeomType( *ioobj );
}


void uiSEGYRead::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    if ( rev_ == Rev0 )
	iop.setYN( SEGY::FileDef::sKeyForceRev0, true );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    rev_ = iop.isTrue( SEGY::FileDef::sKeyForceRev0 ) ? Rev0 : Rev1;
}


void uiSEGYRead::writeReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    if ( !rddlg ) { pErrMsg("Huh"); return; }

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    BufferString objnm( rddlg->saveObjName() );
    ctio->ctxt.setName( objnm );
    if ( !ctio->fillObj(false) ) return;
    else
    {
	BufferString translnm = ctio->ioobj->translator();
	if ( translnm != "SEG-Y" )
	{
	    ctio->setObj( 0 );
	    objnm += " SGY";
	    if ( !ctio->fillObj(false) ) return;
	    translnm = ctio->ioobj->translator();
	    if ( translnm != "SEG-Y" )
	    {
		uiMSG().error( "Cannot write setup under this name - sorry" );
		return;
	    }
	}
    }

    rddlg->updatePars();
    fillPar( ctio->ioobj->pars() );
    ctio->ioobj->pars().removeWithKey( uiSEGYExamine::Setup::sKeyNrTrcs );
    ctio->ioobj->pars().removeWithKey( sKey::Geometry );
    SEGY::FileSpec::ensureWellDefined( *ctio->ioobj );
    IOM().commitChanges( *ctio->ioobj );
    delete ctio->ioobj;
}


void uiSEGYRead::readReq( CallBacker* cb )
{
    uiDialog* parnt = defdlg_;
    uiSEGYReadDlg* rddlg = 0;
    if ( parnt )
	geom_ = defdlg_->geomType();
    else
    {
	mDynamicCastGet(uiSEGYReadDlg*,dlg,cb)
	if ( !dlg ) { pErrMsg("Huh"); return; }
	rddlg = dlg;
    }

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    uiIOObjSelDlg dlg( parnt, *ctio, "Select SEG-Y setup" );
    dlg.setModal( true );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    if ( rddlg )
	rddlg->use( ioobj, false );
    else
    {
	pars_.merge( ioobj->pars() );
	defdlg_->use( ioobj, false );
    }
}


class uiSEGYReadPreScanner : public uiDialog
{
public:

uiSEGYReadPreScanner( uiParent* p, Seis::GeomType gt, const IOPar& pars )
    : uiDialog(p,uiDialog::Setup("SEG-Y Scan",0,mNoHelpID))
    , pars_(pars)
    , geom_(gt)
    , scanner_(0)
    , res_(false)
    , rep_("SEG-Y scan report")
{
    nrtrcsfld_ = new uiGenInput( this, "Limit to number of traces",
	    			 IntInpSpec(1000) );
    nrtrcsfld_->setWithCheck( true );
    nrtrcsfld_->setChecked( true );

    SEGY::FileSpec fs; fs.usePar( pars );
    BufferString fnm( fs.fname_ );
    replaceCharacter( fnm.buf(), '*', 'x' );
    FilePath fp( fnm );
    fp.setExtension( "txt" );
    saveasfld_ = new uiFileInput( this, "Save report as",
	    			  GetProcFileName(fp.fileName()) );
    saveasfld_->setWithCheck( true );
    saveasfld_->attach( alignedBelow, nrtrcsfld_ );
    saveasfld_->setChecked( true );

    setModal( true );
}

~uiSEGYReadPreScanner()
{
    delete scanner_;
}

bool acceptOK( CallBacker* )
{
    scanner_= new SEGY::Scanner( pars_, geom_ );
    const int nrtrcs = nrtrcsfld_->isChecked() ? nrtrcsfld_->getIntValue() : 0;
    scanner_->setRichInfo( true );
    scanner_->setMaxNrtraces( nrtrcs );
    uiTaskRunner uitr( this );
    uitr.execute( *scanner_ );
    res_ = true;
    if ( scanner_->fileDataSet().isEmpty() )
    {
	uiMSG().error( "No traces found" );
	return false;
    }

    const char* fnm = saveasfld_->isChecked() ? saveasfld_->fileName() : 0;
    uiSEGYScanDlg::presentReport( parent(), *scanner_, fnm );
    return true;
}

    const Seis::GeomType geom_;
    const IOPar&	pars_;

    uiGenInput*		nrtrcsfld_;
    uiFileInput*	saveasfld_;

    bool		res_;
    IOPar		rep_;
    SEGY::Scanner*	scanner_;

};


void uiSEGYRead::preScanReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    if ( !rddlg ) { pErrMsg("Huh"); return; }
    rddlg->updatePars();
    fillPar( pars_ );

    uiSEGYReadPreScanner dlg( rddlg, geom_, pars_ );
    if ( !dlg.go() || !dlg.res_ ) return;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread, Seis::GeomType gt )
{
    CtxtIOObj* ret = mMkCtxtIOObj( SeisTrc );
    IOObjContext& ctxt = ret->ctxt;
    ctxt.deftransl = ctxt.trglobexpr = "SEG-Y";
    ctxt.forread = forread;
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIs2D, Seis::is2D(gt) );
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIsPS, Seis::isPS(gt) );
    ctxt.includeconstraints = ctxt.allownonreaddefault = true;
    ctxt.allowcnstrsabsent = false;
    return ret;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread ) const
{
    return getCtio( forread, geom_ );
}


static const char* rev1info =
    "The file was marked as SEG-Y Revision 1 by the producer."
    "\nUnfortunately, not all files are correct in this respect."
    "\n\nPlease specify:";
static const char* rev1txts[] =
{
    "Yes - I know the file is 100% correct SEG-Y Rev.1",
    "Yes - It's Rev. 1 but I may need to overrule some things",
    "No - the file is not SEG-Y Rev.1 - treat as legacy SEG-Y Rev. 0",
    "Cancel - Something must be wrong - take me back",
    0
};

class uiSEGYReadRev1Question : public uiDialog
{
public:

uiSEGYReadRev1Question( uiParent* p, int pol )
    : uiDialog(p,Setup("Determine SEG-Y revision",rev1info,mNoHelpID)
	    	.modal(false) )
    , initialpol_(pol)
{
    uiButtonGroup* bgrp = new uiButtonGroup( this, "" );
    for ( int idx=0; rev1txts[idx]; idx++ )
	buts_ += new uiRadioButton( bgrp, rev1txts[idx] );
    bgrp->setRadioButtonExclusive( true );
    buts_[pol-1]->setChecked( true );

    dontaskfld_ = new uiCheckBox( this, "Don't ask again for this survey" );
    dontaskfld_->attach( alignedBelow, bgrp );
}

bool acceptOK( CallBacker* )
{
    pol_ = 3;
    for ( int idx=0; idx<buts_.size(); idx++ )
    {
	if ( buts_[idx]->isChecked() )
	    { pol_ = idx + 1; break; }
    }
    int storepol = dontaskfld_->isChecked() ? -pol_ : pol_;
    if ( storepol != initialpol_ )
    {
	SI().getPars().set( sKeySEGYRev1Pol, storepol );
	SI().savePars();
    }
    return true;
}

bool isGoBack() const
{
    return pol_ == buts_.size();
}

    ObjectSet<uiRadioButton>	buts_;
    uiCheckBox*			dontaskfld_;
    int				pol_, initialpol_;

};

#define mSetreadReqCB() readParsReq.notify( mCB(this,uiSEGYRead,readReq) )
#define mSetwriteReqCB() writeParsReq.notify( mCB(this,uiSEGYRead,writeReq) )
#define mSetpreScanReqCB() preScanReq.notify( mCB(this,uiSEGYRead,preScanReq) )
#define mLaunchDlg(dlg,fn) \
	dlg->windowClosed.notify( mCB(this,uiSEGYRead,fn) ); \
	dlg->setDeleteOnClose( true ); dlg->go()

void uiSEGYRead::getBasicOpts()
{
    uiSEGYDefDlg::Setup bsu; bsu.geoms_ = setup_.geoms_;
    bsu.defgeom( geom_ ).modal( false );
    defdlg_ = new uiSEGYDefDlg( parent_, bsu, pars_ );
    defdlg_->mSetreadReqCB();
    mLaunchDlg(defdlg_,defDlgClose);
    mSetState(Wait4Dialog);
}


void uiSEGYRead::basicOptsGot()
{
    if ( !defdlg_ ) return;
    if ( !defdlg_->uiResult() )
	mSetState(Cancelled);
    geom_ = defdlg_->geomType();

    uiSEGYExamine::Setup exsu( defdlg_->nrTrcExamine() );
    exsu.modal( false ); exsu.usePar( pars_ );
    BufferString emsg;
    const int exrev = uiSEGYExamine::getRev( exsu, emsg );
    if ( exrev < 0 )
    {
	rev_ = Rev0; uiMSG().error( emsg );
	getBasicOpts(); newdefdlg_ = defdlg_;
	return;
    }

    delete examdlg_; examdlg_ = 0;
    if ( exsu.nrtrcs_ > 0 )
    {
	examdlg_ = new uiSEGYExamine( parent_, exsu );
	mLaunchDlg(examdlg_,examDlgClose);
    }

    rev_ = exrev ? WeakRev1 : Rev0;
    revpolnr_ = exrev;
    if ( rev_ != Rev0 )
    {
	SI().pars().get( sKeySEGYRev1Pol, revpolnr_ );
	if ( revpolnr_ < 0 )
	    revpolnr_ = -revpolnr_;
	else
	{
	    rev1qdlg_ = new uiSEGYReadRev1Question( parent_, revpolnr_ );
	    mLaunchDlg(rev1qdlg_,rev1qDlgClose);
	    mSetState(Wait4Dialog);
	}
    }

    determineRevPol();
}



void uiSEGYRead::determineRevPol()
{
    if ( rev1qdlg_ )
    {
	if ( !rev1qdlg_->uiResult() || rev1qdlg_->isGoBack() )
	    mSetState(BasicOpts)
	revpolnr_ = rev1qdlg_->pol_;
    }
    rev_ = revpolnr_ == 1 ? Rev1 : (revpolnr_ == 2 ? WeakRev1 : Rev0);
    mSetState( setup_.purpose_ == Import ? SetupImport : SetupScan );
}



void uiSEGYRead::setupScan()
{
    delete scanner_; scanner_ = 0;
    uiSEGYReadDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    if ( setup_.purpose_ == SurvSetup && Seis::is2D(geom_) )
	uiMSG().warning(
	"Scanning a 2D file can provide valuable info on your survey.\n"
	"But to actually set up your survey, you need to select\n"
	"'Set for 2D only' and press 'Go'\n"
	"In the survey setup window" );
    scandlg_ = new uiSEGYScanDlg( parent_, su, pars_,
	    			  setup_.purpose_ == SurvSetup );
    scandlg_->mSetreadReqCB();
    scandlg_->mSetwriteReqCB();
    scandlg_->mSetpreScanReqCB();
    mLaunchDlg(scandlg_,scanDlgClose);
    mSetState( Wait4Dialog );
}


void uiSEGYRead::setupImport()
{
    uiSEGYImpDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    impdlg_ = new uiSEGYImpDlg( parent_, su, pars_ );
    impdlg_->mSetreadReqCB();
    impdlg_->mSetwriteReqCB();
    impdlg_->mSetpreScanReqCB();
    mLaunchDlg(impdlg_,impDlgClose);
    mSetState( Wait4Dialog );
}


void uiSEGYRead::defDlgClose( CallBacker* )
{
    basicOptsGot();
    defdlg_ = newdefdlg_ ? newdefdlg_ : 0;
    newdefdlg_ = 0;
}


void uiSEGYRead::examDlgClose( CallBacker* )
{
    examdlg_ = 0;
}


void uiSEGYRead::scanDlgClose( CallBacker* )
{
    if ( !scandlg_->uiResult() )
	{ scandlg_ = 0; mSetState( BasicOpts ); }

    scanner_ = scandlg_->getScanner();
    scandlg_ = 0;
    mSetState( Finished );
}


void uiSEGYRead::impDlgClose( CallBacker* )
{
    State newstate = impdlg_->uiResult() ? Finished : BasicOpts;
    impdlg_ = 0;
    mSetState( newstate );
}


void uiSEGYRead::rev1qDlgClose( CallBacker* )
{
    determineRevPol();
    rev1qdlg_ = 0;
}


uiSEGYRead::~uiSEGYRead()
{
    delete defdlg_;
    delete impdlg_;
    delete scandlg_;
    delete rev1qdlg_;
}
