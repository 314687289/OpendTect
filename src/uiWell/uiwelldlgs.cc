/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldlgs.cc,v 1.81 2009-06-22 12:50:24 cvsbert Exp $";

#include "uiwelldlgs.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uiwellpartserv.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "iopar.h"
#include "oddirs.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welllog.h"
#include "welltransl.h"
#include "welllogset.h"
#include "welltrack.h"


#define mFromFeetFac 0.3048
static const char* trackcollbls[] = { "X", "Y", "Z", "MD", 0 };
static const int nremptyrows = 5;

uiWellTrackDlg::uiWellTrackDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well Track",
				     "Edit Well Track",
				     mTODOHelpID))
	, wd_(d)
    	, track_(d.track())
    	, orgtrack_(new Well::Track(d.track()))
	, fd_( *Well::TrackAscIO::getDesc() )

{
    setPrefWidth( 500 ); setPrefHeight( 400 );
    tbl_ = new uiTable( this, uiTable::Setup().rowdesc("Point")
	    				       .rowgrow(true) 
					       .defrowlbl(""), "Table" );
    tbl_->setColumnLabels( trackcollbls );
    tbl_->setNrRows( nremptyrows );

    zinftfld_ = new uiCheckBox( this, "Z in Feet" );
    zinftfld_->setChecked( SI().depthsInFeetByDefault() );
    zinftfld_->activated.notify( mCB(this,uiWellTrackDlg,fillTable) );
    zinftfld_->attach( ensureBelow, tbl_ );
    zinftfld_->attach( rightBorder );

    uiGroup* actbutgrp = new uiGroup( this, "Action buttons grp" );
    uiButton* updnowbut = new uiPushButton( actbutgrp, "&Update display",
	    				    mCB(this,uiWellTrackDlg,updNow),
					    true );
    uiButton* readbut = new uiPushButton( actbutgrp, "&Read new",
	    				    mCB(this,uiWellTrackDlg,readNew),
					    false );
    readbut->attach( rightOf, updnowbut );
    actbutgrp->attach( centeredBelow, tbl_ );
    zinftfld_->attach( ensureRightOf, actbutgrp );

    fillTable();
}


void uiWellTrackDlg::fillTable( CallBacker* )
{
    RowCol curcell( tbl_->currentCell() );

    const int sz = track_.nrPoints();
    int newsz = sz + nremptyrows;
    if ( newsz < 8 ) newsz = 8;
    tbl_->setNrRows( newsz );
    tbl_->clearTable();
    if ( !sz ) return;

    const bool zinft = zinftfld_->isChecked();
    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c( track_.pos(idx) );
	tbl_->setValue( RowCol(idx,0), c.x );
	tbl_->setValue( RowCol(idx,1), c.y );
	if ( !zinft )
	{
	    tbl_->setValue( RowCol(idx,2), c.z );
	    tbl_->setValue( RowCol(idx,3), track_.dah(idx) );
	}
	else
	{
	    tbl_->setValue( RowCol(idx,2), c.z*mToFeetFactor );
	    tbl_->setValue( RowCol(idx,3), track_.dah(idx)*mToFeetFactor );
	}
    }

    if ( curcell.row >= newsz ) curcell.row = newsz-1;
    tbl_->setCurrentCell( curcell );
}


uiWellTrackDlg::~uiWellTrackDlg()
{
    delete orgtrack_;
    delete &fd_;
}


class uiWellTrackReadDlg : public uiDialog
{
public:

uiWellTrackReadDlg( uiParent* p, Table::FormatDesc& fd, Well::Track& track )
    	: uiDialog(p,uiDialog::Setup("Read new Well Track",
		    		     "Specify new Well Track",mTODOHelpID))
	, track_(track)							  
{
    wtinfld_ = new uiFileInput( this, "Well Track File",
    uiFileInput::Setup().withexamine(true) );
    wtinfld_->setDefaultSelectionDir(
		    IOObjContext::getDataDirName(IOObjContext::WllInf) );
    
    uiTableImpDataSel* dataselfld = new uiTableImpDataSel( this, fd, 0 );
    dataselfld->attach( alignedBelow, wtinfld_ );
}


bool acceptOK( CallBacker* )
{
    track_.erase();
    fnm_ = wtinfld_->fileName();
    if ( File_isEmpty(fnm_.buf()) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    return true;
}

    uiFileInput*	wtinfld_;
    BufferString	fnm_;
    Well::Track&        track_;
};


void uiWellTrackDlg::readNew( CallBacker* )
{
    uiWellTrackReadDlg dlg( this, fd_, track_ );
    if ( !dlg.go() ) return;

    if ( !dlg.fnm_.isEmpty() )
    {
	StreamData sd = StreamProvider( dlg.fnm_ ).makeIStream();
	if ( !sd.usable() )
	uiMSG().error( "Cannot open input file" );

	Well::TrackAscIO wellascio(fd_, *sd.istrm );
	if ( !wellascio.getData( wd_, true ) )
	uiMSG().error( "Failed to convert into compatible data" );

	sd.close();
	
	tbl_->clearTable();
	fillTable();
	wd_.trackchanged.trigger();
    }
    else
	uiMSG().error( "Please select a file" );
}


void uiWellTrackDlg::updNow( CallBacker* )
{
    track_.erase();
    const int nrrows = tbl_->nrRows();
    const bool zinft = zinftfld_->isChecked();
    bool needfill = false;
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = tbl_->text( RowCol(idx,0) );
	if ( !*sval ) continue;
	const float xval = atof(sval);
	sval = tbl_->text( RowCol(idx,1) );
	if ( !*sval ) continue;
	const float yval = atof(sval);
	sval = tbl_->text( RowCol(idx,2) );
	if ( !*sval ) continue;
	float zval = atof(sval); if ( zinft ) zval *= mFromFeetFactor;

	const Coord3 newc( xval, yval, zval );
	sval = tbl_->text( RowCol(idx,3) );
	float dahval = 0;
	if ( *sval )
	    { dahval = atof(sval); if ( zinft ) dahval *= mFromFeetFactor; }
	else if ( idx > 0 )
	{
	    dahval = track_.dah(idx-1) + track_.pos(idx-1).distTo( newc );
	    needfill = true;
	}

	track_.addPoint( newc, newc.z, dahval );
    }
    if ( track_.nrPoints() > 1 )
	wd_.trackchanged.trigger();
    else
	uiMSG().error( "Please define at least two points." );

    if ( needfill )
	fillTable();
}


bool uiWellTrackDlg::rejectOK( CallBacker* )
{
    track_ = *orgtrack_;
    wd_.trackchanged.trigger();
    return true;
}


bool uiWellTrackDlg::acceptOK( CallBacker* )
{
    updNow( 0 );
    const int nrpts = track_.nrPoints();
    if ( nrpts < 2 ) return false;
    const int orgnrpts = orgtrack_->nrPoints();
    bool dahchg = nrpts != orgnrpts;
    if ( !dahchg )
    {
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    const float dah = track_.dah(idx);
	    const float orgdah = orgtrack_->dah(idx);
	    if ( !mIsEqual(dah,orgdah,0.001) )
		{ dahchg = true; break; }
	}
    }

    if ( dahchg )
    {
	BufferString msg( "You have changed at least one MD value.\nMarkers" );
	if ( SI().zIsTime() )
	    msg += ", logs, T/D and checkshot models";
	else
	    msg += " and logs";
	msg += " are based on the old MD values.\n"
	       "They may therefore become invalid.\n\nContinue?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    return true;
}



// ==================================================================


static const char* t2dcollbls[] = { "Depth (MD)", "Time (ms)", 0 };
#define mSetSIDepthInFeetDef(zinft) \
    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt(), zinft ); \
    SI().savePars()

#define mD2TModel (cksh_ ? wd_.checkShotModel() : wd_.d2TModel())

uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& d, bool cksh )
	: uiDialog(p,uiDialog::Setup("Depth/Time Model",
				     "Edit velocity model",
				     "107.1.5"))
	, wd_(d)
    	, cksh_(cksh)
    	, orgd2t_(mD2TModel ? new Well::D2TModel(*mD2TModel) : 0)
{
    tbl_ = new uiTable( this, uiTable::Setup()
	    			.rowdesc(cksh_ ? "Measure point" : "Control Pt")
				.rowgrow(true).defrowlbl(""), "Table" );
    tbl_->setColumnLabels( t2dcollbls );
    tbl_->setNrRows( nremptyrows );

    uiGroup* actbutgrp = new uiGroup( this, "Action buttons grp" );
    uiButton* updnowbut = 0;
    if ( !cksh_ )
	updnowbut = new uiPushButton( actbutgrp, "&Update display",
					mCB(this,uiD2TModelDlg,updNow),
					true );
    uiButton* readbut = new uiPushButton( actbutgrp, "&Read new",
	    				    mCB(this,uiD2TModelDlg,readNew),
					    false );
    if ( updnowbut ) readbut->attach( rightOf, updnowbut );
    actbutgrp->attach( centeredBelow, tbl_ );

    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    unitfld_ = new uiGenInput( this, "Depth unit", mft );
    unitfld_->attach( leftAlignedBelow, tbl_ );
    unitfld_->attach( ensureBelow, actbutgrp );

    fillTable();
}


void uiD2TModelDlg::fillTable()
{
    const Well::D2TModel* d2t = mD2TModel;
    const int sz = d2t ? d2t->size() : 0;
    if ( !sz ) return;
    tbl_->setNrRows( sz + nremptyrows );

    const float zfac = unitfld_->getBoolValue() ? 1 : 1./mFromFeetFac;
    for ( int idx=0; idx<sz; idx++ )
    {
	tbl_->setValue( RowCol(idx,0), d2t->dah(idx) * zfac );
	tbl_->setValue( RowCol(idx,1), d2t->t(idx) * 1000 );
    }
}


uiD2TModelDlg::~uiD2TModelDlg()
{
    delete orgd2t_;
}

class uiD2TModelReadDlg : public uiDialog
{
public:

uiD2TModelReadDlg( uiParent* p, Well::Data& wd, bool cksh )
    	: uiDialog(p,uiDialog::Setup("Read new data",
		    		     "Specify input file","107.1.6"))
	, cksh_(cksh)
	, wd_(wd)
{
    uiD2TModelGroup::Setup su( false );
    su.filefldlbl( "File name" );
    d2tgrp = new uiD2TModelGroup( this, su );
}


bool acceptOK( CallBacker* )
{
    const char* errmsg = d2tgrp->getD2T( wd_, cksh_ );
    if ( errmsg && *errmsg )
	{ uiMSG().error( errmsg ); return false; }

    if ( wd_.d2TModel() )
	wd_.d2TModel()->deInterpolate();

    return true;
}

    uiD2TModelGroup*	d2tgrp;
    Well::Data&		wd_;
    const bool		cksh_;

};


void uiD2TModelDlg::readNew( CallBacker* )
{
    uiD2TModelReadDlg dlg( this, wd_, cksh_ );
    if ( !dlg.go() ) return;

    const BufferString errmsg = dlg.d2tgrp->getD2T( wd_, cksh_ );
    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );
    else
    {
	tbl_->clearTable();
	fillTable();
	wd_.d2tchanged.trigger();
    }
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;

    d2t->erase();
    const float zfac = unitfld_->getBoolValue() ? 1 : mFromFeetFac;
    const int nrrows = tbl_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = tbl_->text( RowCol(idx,0) );
	if ( !sval || !*sval ) continue;
	float dah = atof(sval) * zfac;
	sval = tbl_->text( RowCol(idx,1) );
	if ( !sval || !*sval ) continue;
	float tm = atof(sval) * 0.001;
	d2t->add( dah, tm );
    }
    if ( d2t->size() > 1 )
	wd_.d2tchanged.trigger();
    else
	uiMSG().error( "Please define at least two control points." );
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	*d2t = *orgd2t_;
    wd_.d2tchanged.trigger();
    return true;
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    updNow( 0 );
    return mD2TModel && mD2TModel->size() > 0;
}



// ==================================================================

static const float defundefval = -999.25;
static const float feetfac = 0.3048;
#ifdef __win__
    static const char* lasfileflt = "Las files (*.las *.dat)";
#else
    static const char* lasfileflt = "Las files (*.las *.LAS *.dat *.DAT)";
#endif


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters","107.1.2"))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup().filter(lasfileflt)
			      			  .withexamine(true) );
    lasfld->setDefaultSelectionDir( GetDataDir() );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load (empty=all)",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );
    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    intvunfld = new uiGenInput( this, "", mft );
    intvunfld->attach( rightOf, intvfld );

    unitlbl = new uiLabel( this, "XXXX" );
    unitlbl->attach( rightOf, intvfld );
    unitlbl->display( false );

    istvdfld = new uiGenInput( this, "Depth values are",
	    			BoolInpSpec(false,"TVDSS","MD") );
    istvdfld->attach( alignedBelow, intvfld );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, istvdfld );

    logsfld = new uiLabeledListBox( this, "Select logs", true );
    logsfld->attach( alignedBelow, udffld );
}


void uiLoadLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd_; Well::LASImporter wdai( wd_ );
    Well::LASImporter::FileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->empty();
    logsfld->box()->addItems( lfi.lognms );
    logsfld->box()->selectAll( true );

    BufferString lbl( "(" ); lbl += lfi.zunitstr.buf(); lbl += ")";
    unitlbl->setText( lbl );
    unitlbl->display( true );
    bool isft = *lfi.zunitstr.buf() == 'f' || *lfi.zunitstr.buf() == 'F';
    intvunfld->setValue( !isft );
    intvunfld->display( false );

    udffld->setValue( lfi.undefval );
    if ( isft )
    {
	if ( !mIsUdf(lfi.zrg.start) ) lfi.zrg.start /= feetfac;
	if ( !mIsUdf(lfi.zrg.stop) ) lfi.zrg.stop /= feetfac;
    }
    intvfld->setValue( lfi.zrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::LASImporter wdai( wd );
    Well::LASImporter::FileInfo lfi;

    lfi.undefval = udffld->getfValue();
    lfi.zrg.setFrom( intvfld->getFInterval() );
    const bool zinft = !intvunfld->getBoolValue();
    if ( zinft )
    {
	if ( !mIsUdf(lfi.zrg.start) ) lfi.zrg.start *= feetfac;
	if ( !mIsUdf(lfi.zrg.stop) ) lfi.zrg.stop *= feetfac;
    }
    mSetSIDepthInFeetDef( zinft );

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
	{ uiMSG().error("Enter valid filename"); return false; }

    BufferStringSet lognms;
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }
    if ( lognms.size() == 0 )
	{ uiMSG().error("Please select at least one log"); return false; }
    lfi.lognms = lognms;

    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld->getBoolValue() );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}



// ==================================================================


static const char* exptypes[] =
{
    "MD/Value",
    "XYZ/Value",
    "ICZ/Value",
    0
};


uiExportLogs::uiExportLogs( uiParent* p, const Well::Data& wd_, 
			  const BoolTypeSet& sel_ )
    : uiDialog(p,uiDialog::Setup("Export Well logs",
				 "Specify format","107.1.3"))
    , wd(wd_)
    , logsel(sel_)
{
    const bool zinft = SI().depthsInFeetByDefault();
    BufferString lbl( "Depth range " ); lbl += zinft ? "(ft)" : "(m)";
    zrangefld = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld = new uiGenInput( this, "Output format", 
	    		      StringListInpSpec(exptypes) );
    typefld->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld->attach( alignedBelow, zrangefld );

    zunitgrp = new uiButtonGroup( this, "", false );
    zunitgrp->attach( alignedBelow, typefld );
    uiLabel* zlbl = new uiLabel( this, "Output Z-unit" );
    zlbl->attach( leftOf, zunitgrp );
    uiRadioButton* meterbut = new uiRadioButton( zunitgrp, "meter" );
    uiRadioButton* feetbut = new uiRadioButton( zunitgrp, "feet" );
    if ( SI().zIsTime() && wd.d2TModel() )
    {
	uiRadioButton* secbut = new uiRadioButton( zunitgrp, "sec" );
	uiRadioButton* msecbut = new uiRadioButton( zunitgrp, "msec" );
    }
    zunitgrp->selectButton( zinft );

    outfld = new uiFileInput( this, "Output file",
	   			uiFileInput::Setup().forread(false) );
    outfld->setDefaultSelectionDir(
			IOObjContext::getDataDirName(IOObjContext::WllInf) );
    outfld->attach( alignedBelow, zunitgrp );
    typeSel(0);
}


void uiExportLogs::setDefaultRange( bool zinft )
{
    StepInterval<float> dahintv;
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
        const Well::Log& log = wd.logs().getLog(idx);
        const int logsz = log.size();
        if ( !logsz ) continue;

	dahintv.setFrom( wd.logs().dahInterval() );
        const float width = log.dah(logsz-1) - log.dah(0);
        dahintv.step = width / (logsz-1);
	break;
    }

    if ( zinft )
    {
	dahintv.start /= mFromFeetFac;
	dahintv.stop /= mFromFeetFac;
	dahintv.step /= mFromFeetFac;
    }

    zrangefld->setValue( dahintv );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp->setSensitive( 2, typefld->getIntValue() );
    zunitgrp->setSensitive( 3, typefld->getIntValue() );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiExportLogs::acceptOK( CallBacker* )
{
    BufferString fname = outfld->fileName();
    if ( !*fname ) mErrRet( "Please select filename" )

    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" )
    }

    writeHeader( sdo );
    writeLogs( sdo );
    sdo.close();
    return true;
}


void uiExportLogs::writeHeader( StreamData& sdo )
{
    const char* units[] = { "(m)", "(ft)", "(s)", "(ms)", 0 };
    
    if ( typefld->getIntValue() == 1 )
	*sdo.ostrm << "X\tY\t";
    else if ( typefld->getIntValue() == 2 )
	*sdo.ostrm << "Inline\tCrossline\t";

    const int unitid = zunitgrp->selectedId();
    BufferString zstr( unitid<2 ? "Depth" : "Time" );
    *sdo.ostrm << zstr << units[unitid];
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	if ( !logsel[idx] ) continue;
	const Well::Log& log = wd.logs().getLog(idx);
	BufferString lognm( log.name() );
	cleanupString( lognm.buf(), 0, 0, 0 );
	replaceCharacter( lognm.buf(), '+', '_' );
	replaceCharacter( lognm.buf(), '-', '_' );
	*sdo.ostrm << "\t" << lognm;
	if ( *log.unitMeasLabel() )
	    *sdo.ostrm << "(" << log.unitMeasLabel() << ")";
    }
    
    *sdo.ostrm << '\n';
}


void uiExportLogs::writeLogs( StreamData& sdo )
{
    bool inmeter = zunitgrp->selectedId() == 0;
    bool infeet = zunitgrp->selectedId() == 1;
    bool insec = zunitgrp->selectedId() == 2;
    bool inmsec = zunitgrp->selectedId() == 3;

    const bool zinft = SI().depthsInFeetByDefault();
    const int outtypesel = typefld->getIntValue();
    const bool dobinid = outtypesel == 2;
    const StepInterval<float> intv = zrangefld->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFac;
	if ( outtypesel == 0 )
	{
	    const float mdout = infeet ? md/mFromFeetFac : md;
	    *sdo.ostrm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( md );
	    if ( !pos.x && !pos.y && !pos.z ) continue;

	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
	    {
		*sdo.ostrm << getStringFromDouble(0,pos.x) << '\t';
		*sdo.ostrm << getStringFromDouble(0,pos.y);
	    }

	    float z = pos.z;
	    if ( infeet ) z /= mFromFeetFac;
	    else if ( insec ) z = wd.d2TModel()->getTime( md );
	    else if ( inmsec ) z = wd.d2TModel()->getTime( md ) * 1000;
	    *sdo.ostrm << '\t' << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    if ( !logsel[logidx] ) continue;
	    const float val = wd.logs().getLog(logidx).getValue( md );
	    if ( mIsUdf(val) )
		*sdo.ostrm << '\t' << "1e30";
	    else
		*sdo.ostrm << '\t' << val;
	}

	*sdo.ostrm << '\n';
    }
}


//============================================================================

uiStoreWellDlg::uiStoreWellDlg( uiParent* p, const BufferString& wellname )
    : uiDialog(p,uiDialog::Setup("Store Well",
				 "Specify well parameters", "107.0.1"))
    , ctio_(*mMkCtxtIOObj(Well))
    , usemodelfld(0)
    , constvelfld(0)
{
    uiGroup* topgrp = 0;
    if ( SI().zIsTime() )
    {
	topgrp = new uiGroup( this, "time survey group" );
	constvelfld =
	    new uiGenInput( topgrp, "Constant velocity (m/s)", FloatInpSpec() );
	topgrp->setHAlignObj( constvelfld );
    }

    ctio_.ctxt.forread = false;
    uiIOObjSel::Setup su( "Output Well" );
    su.keepmytxt( true );
    outfld = new uiIOObjSel( this, ctio_, su );
    if ( topgrp )
	outfld->attach( alignedBelow, topgrp );

    outfld->setInputText( wellname );
}


uiStoreWellDlg::~uiStoreWellDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiStoreWellDlg::setWellCoords( const TypeSet<Coord3>& newcoords )
{ wellcoords_ = newcoords; }


MultiID uiStoreWellDlg::getMultiID() const
{ return ctio_.ioobj ? ctio_.ioobj->key() : -1; }


bool uiStoreWellDlg::acceptOK( CallBacker* )
{
    return checkInpFlds() && storeWell();
}


bool uiStoreWellDlg::checkInpFlds()
{
    if ( SI().zIsTime() )
    {
	if ( constvelfld )
	{
	    const float vel = constvelfld->getfValue();
	    if ( mIsUdf(vel) || vel<=0 )
		mErrRet( "Please specify correct (positive) velocity value" )
	}
    }
	    
    if ( !outfld->commitInput() )
	mErrRet( "Please select an output" )

    return true;
}


bool uiStoreWellDlg::storeWell()
{
    PtrMan<Translator> tr = ctio_.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,tr.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
			 "Another type object with this name already exists." );

    const char* wellname = outfld->getInput();
    PtrMan<Well::Data> well = new Well::Data( wellname );
    const bool res = setWellTrack( well );
    if ( !res ) return false;

    well->info().surfacecoord = Coord( well->track().pos(0).x, 
	    			       well->track().pos(0).y );
    if ( !wtr->write(*well,*ctio_.ioobj) ) mErrRet( "Cannot write well" );

    return true;
}


bool uiStoreWellDlg::setWellTrack( Well::Data* well )
{
    TypeSet<float> times;
    if ( SI().zIsTime() )
    {
	const float vel = constvelfld->getfValue();
	for ( int idx=0; idx<wellcoords_.size(); idx++ )
	{
	    times += wellcoords_[idx].z;
	    wellcoords_[idx].z *= vel; 
	}
    }

    for ( int idx=0; idx<wellcoords_.size(); idx++ )
	well->track().insertPoint( Coord(wellcoords_[idx].x,wellcoords_[idx].y),
				   wellcoords_[idx].z );

    if ( SI().zIsTime() )
    {
	const float vel = constvelfld->getfValue();
	Well::D2TModel* d2t = new Well::D2TModel();
	for ( int idx=0; idx<wellcoords_.size(); idx++ )
	    d2t->add( well->track().dah(idx), times[idx] );
	
	well->setD2TModel(d2t);
    }

    return true;
}


//============================================================================

uiNewWellDlg::uiNewWellDlg( uiParent* p )
        : uiGetObjectName(p,uiGetObjectName::Setup("New Well",mkWellNms())
	       			.inptxt("New well name") )
{
    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
	    			      .lbltxt("Color") );
    colsel_->attach( alignedBelow, inpFld() );
}


uiNewWellDlg::~uiNewWellDlg()
{
    delete nms_;
}


const BufferStringSet& uiNewWellDlg::mkWellNms()
{
    nms_ = new BufferStringSet;
    IOObjContext ctxt( WellTranslatorGroup::ioContext() );
    IOM().to( ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( ioobj )
	    nms_->add( ioobj->name() );
    }
    return *nms_;
}

bool uiNewWellDlg::acceptOK( CallBacker* )
{
    BufferString tmp( text() );
    char* ptr = tmp.buf();
    mTrimBlanks(ptr);
    if ( !*ptr )
	mErrRet( "Please enter a name" )

    if ( nms_->indexOf(ptr) >= 0 )
	mErrRet( "Please specify a new name.\n"
		 "Wells can be removed in 'Manage wells'" )

    name_ = ptr;
    return true;
}


const Color& uiNewWellDlg::getWellColor()
{
    return colsel_->color();
}
