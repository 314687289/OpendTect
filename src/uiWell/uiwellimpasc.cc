/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpasc.cc,v 1.70 2011-11-23 11:35:56 cvsbert Exp $";

#include "uiwellimpasc.h"

#include "ctxtioobj.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "strmprov.h"
#include "tabledef.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welltransl.h"

#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uiselsurvranges.h"
#include "uitblimpexpdatasel.h"

static const char* sHelpID = "107.0.0";
static const char* nHelpID = "107.0.4";


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Import Well Track",sHelpID))
    , ctio_(*mMkCtxtIOObj(Well))
    , fd_(*Well::TrackAscIO::getDesc())
    , trckinpfld_(0)
{
    setCtrlStyle( DoAndStay );

    havetrckbox_ = new uiCheckBox( this, "" );
    havetrckbox_->activated.notify( mCB(this,uiWellImportAsc,haveTrckSel) );
    havetrckbox_->setChecked( true );
    trckinpfld_ = new uiFileInput( this, "Well Track File",
	    			   uiFileInput::Setup().withexamine(true) );
    trckinpfld_->attach( rightOf, havetrckbox_ );
    vertwelllbl_ = new uiLabel( this, "-> Vertical well" );
    vertwelllbl_->attach( rightTo, havetrckbox_ );
    vertwelllbl_->attach( alignedWith, trckinpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.0.2" );
    dataselfld_->attach( alignedBelow, trckinpfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiWellImportAsc,trckFmtChg) );

    coordfld_ = new uiGenInput( this, "Coordinate",
	    			PositionInpSpec(PositionInpSpec::Setup(true)) );
    coordfld_->attach( alignedBelow, trckinpfld_ );

    kbelevfld_ = new uiGenInput( this, "KB Elevation", FloatInpSpec(0) );
    kbelevfld_->attach( alignedBelow, coordfld_ );
    tdfld_ = new uiGenInput( this, "TD", FloatInpSpec() );
    tdfld_->attach( alignedBelow, kbelevfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    const bool zistime = SI().zIsTime();
    if ( zistime )
    {
	uiD2TModelGroup::Setup su; su.asksetcsmdl( true );
	d2tgrp_ = new uiD2TModelGroup( this, su );
	d2tgrp_->attach( alignedBelow, dataselfld_ );
	d2tgrp_->attach( ensureBelow, sep );
	sep = new uiSeparator( this, "H sep 2" );
	sep->attach( stretchedBelow, d2tgrp_ );
    }

    uiButton* but = new uiPushButton( this, "Advanced/Optional",
	    				mCB(this,uiWellImportAsc,doAdvOpt),
					false );
    but->attach( alignedBelow, zistime ? (uiObject*)d2tgrp_
	    			       : (uiObject*)dataselfld_ );
    but->attach( ensureBelow, sep );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Well" );
    outfld_->attach( alignedBelow, but );

    postFinalise().notify( mCB(this,uiWellImportAsc,haveTrckSel) );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


void uiWellImportAsc::haveTrckSel( CallBacker* )
{
    if ( !trckinpfld_ ) return;
    const bool havetrck = havetrckbox_->isChecked();
    trckinpfld_->display( havetrck );
    dataselfld_->display( havetrck );
    vertwelllbl_->display( !havetrck );
    coordfld_->display( !havetrck );
    kbelevfld_->display( !havetrck );
    tdfld_->display( !havetrck );
}


void uiWellImportAsc::trckFmtChg( CallBacker* )
{
    const Table::FormatDesc& fd = dataselfld_->desc();
    if ( fd.isGood() )
    {
	for ( int idx=0; idx<fd.bodyinfos_.size(); idx++ )
	{
	    const Table::TargetInfo& ti = *fd.bodyinfos_[idx];
	    if ( ti.name() == "Z" || ti.name() == "MD" )
	    {
		if ( ti.selection_.isInFile(0) )
		    return;
	    }
	}
	uiMSG().error( "The format you defined has neither Z nor MD."
			"\nYou should define at least one."
			"\nAs it is now, the track will not load." );
    }
}


class uiWellImportAscOptDlg : public uiDialog
{
public:

uiWellImportAscOptDlg( uiWellImportAsc* p )
    : uiDialog(p,uiDialog::Setup("Import well: Advanced/Optional",
				 "Advanced and Optional",nHelpID))
    , uwia_(p)
{
    const Well::Info& info = uwia_->wd_.info();

    PositionInpSpec::Setup possu( true );
    if ( !mIsZero(info.surfacecoord.x,0.1) )
	possu.coord_ = info.surfacecoord;
    coordfld = new uiGenInput( this,
	"Surface coordinate (if different from first coordinate in track file)",
	PositionInpSpec(possu).setName( "X", 0 ).setName( "Y", 1 ) );

    float dispval = -info.surfaceelev;
    if ( SI().depthsInFeetByDefault() && !mIsUdf(info.surfaceelev) ) 
	dispval *= mToFeetFactor;
    if ( mIsZero(dispval,0.01) ) dispval = 0;
    elevfld = new uiGenInput( this,
	    "Surface Reference Datum (SRD)", FloatInpSpec(dispval) );
    elevfld->attach( alignedBelow, coordfld );
    zinftbox = new uiCheckBox( this, "Feet" );
    zinftbox->attach( rightOf, elevfld );
    zinftbox->setChecked( SI().depthsInFeetByDefault() );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, elevfld );

    idfld = new uiGenInput( this, "Well ID (UWI)", StringInpSpec(info.uwid) );
    idfld->attach( alignedBelow, elevfld );
    
    operfld = new uiGenInput( this, "Operator", StringInpSpec(info.oper) );
    operfld->attach( alignedBelow, idfld );
    
    statefld = new uiGenInput( this, "State", StringInpSpec(info.state) );
    statefld->attach( alignedBelow, operfld );

    countyfld = new uiGenInput( this, "County", StringInpSpec(info.county) );
    countyfld->attach( rightTo, statefld );
}


bool acceptOK( CallBacker* )
{
    Well::Info& info = uwia_->wd_.info();

    if ( *coordfld->text() )
	info.surfacecoord = coordfld->getCoord();
    if ( *elevfld->text() )
    {
	info.surfaceelev = -elevfld->getfValue();
	if ( zinftbox->isChecked() && !mIsUdf(info.surfaceelev) ) 
	    info.surfaceelev *= mFromFeetFactor;
    }

    info.uwid = idfld->text();
    info.oper = operfld->text();
    info.state = statefld->text();
    info.county = countyfld->text();

    return true;
}

    uiWellImportAsc*	uwia_;
    uiGenInput*		coordfld;
    uiGenInput*		elevfld;
    uiGenInput*		idfld;
    uiGenInput*		operfld;
    uiGenInput*		statefld;
    uiGenInput*		countyfld;
    uiCheckBox*		zinftbox;
};


void uiWellImportAsc::doAdvOpt( CallBacker* )
{
    uiWellImportAscOptDlg dlg( this );
    dlg.go();
}


bool uiWellImportAsc::acceptOK( CallBacker* )
{
    if ( checkInpFlds() )
    {
	doWork();
	wd_.info().surfacecoord.x = wd_.info().surfacecoord.y = 0;
	wd_.info().surfaceelev = 0;
    }
    return false;
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiWellImportAsc::doWork()
{
    wd_.empty();
    wd_.info().setName( outfld_->getInput() );

    if ( havetrckbox_->isChecked() )
    {
	BufferString fnm( trckinpfld_->fileName() );
	if ( !fnm.isEmpty() )
	{
	    StreamData sd = StreamProvider( trckinpfld_->fileName() )
					.makeIStream();
	    if ( !sd.usable() )
		mErrRet( "Cannot open track file" )
	    Well::TrackAscIO wellascio( fd_, *sd.istrm );
	    if ( !wellascio.getData(wd_,true) )
	    {
		BufferString msg( "The track file cannot be loaded:\n" );
		msg += wellascio.errMsg();
		mErrRet( msg.buf() );
	    }
	    sd.close();
	}
    }
    else
    {
	float kbelev = kbelevfld_->getfValue();
	if ( mIsUdf(kbelev) ) kbelev = 0;
	else if ( SI().depthsInFeetByDefault() ) kbelev *= mFromFeetFactor;

	float td = tdfld_->getfValue();
	if ( !mIsUdf(td) && SI().depthsInFeetByDefault() )
	    td *= mFromFeetFactor;
	if ( mIsUdf(td) || td < 1e-6 )
	{
	    float survzstop = SI().zRange(false).stop;
	    if ( SI().zIsTime() )
		survzstop *= 2000;
	    td = survzstop - kbelev;
	}

	Interval<float> zrg( -kbelev, td-kbelev );
	const Coord c( coordfld_->getCoord() );
	wd_.track().addPoint( c, zrg.start, 0 );
	wd_.track().addPoint( c, zrg.stop, td );
    }

    if ( SI().zIsTime() )
    {
	const char* errmsg = d2tgrp_->getD2T( wd_, false );
	if ( errmsg ) mErrRet( errmsg );
	if ( d2tgrp_->wantAsCSModel() )
	    d2tgrp_->getD2T( wd_, true );
    }

    PtrMan<Translator> t = ctio_.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(wd_,*ctio_.ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Well successfully created" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    if ( havetrckbox_->isChecked() )
    {
	if ( !*trckinpfld_->fileName() )
	    mErrRet("Please specify a well track file")

	if ( !dataselfld_->commit() )
	    return false;
    }
    else
    {
	if ( !SI().isReasonable(coordfld_->getCoord()) )
	{
	    if ( !uiMSG().askGoOn(
			"Well coordinate seems to be far outside the survey."
		    	"\nIs this correct?") )
		return false;
	}
    }

    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please enter a name for the well" : 0 )

    return true;
}
