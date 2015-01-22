/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uiunitsel.h"
#include "uiwellsel.h"

#include "ctxtioobj.h"
#include "file.h"
#include "iodir.h"
#include "ioobj.h"
#include "iodirentry.h"
#include "iopar.h"
#include "oddirs.h"
#include "randcolor.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welltrack.h"
#include "wellimpasc.h"
#include "od_helpids.h"


static const char* trackcollbls[] = { "X", "Y", "Z", "MD", 0 };
static const int nremptyrows = 5;
static const int cXCol = 0;
static const int cYCol = 1;
static const int cZCol = 2;
static const int cMDTrackCol = 3;

static BufferString getWinTitle( const char* objtyp, const MultiID& wllky,
				 bool& iswr )
{
    iswr = Well::Writer::isFunctional( wllky );
    return BufferString( iswr ? "Edit" : "View", " ", objtyp );
}


#define mGetDlgSetup(wd,objtyp,hid) \
    uiDialog::Setup( getWinTitle(objtyp,wd.multiID(),writable_), \
		     mNoDlgTitle, mODHelpKey(hid) )
#define mTDName(iscksh) iscksh ? "Checkshot Data" : "Time-Depth Model"
#define mTDOpName(op,iscksh) \
    BufferString( op, " ", mTDName(iscksh) )

#define mAddSetBut(fld,cb) \
    if ( writable_ ) \
    { \
	setbut = new uiPushButton( actbutgrp, tr("Set"), \
				mCB(this,uiWellTrackDlg,cb), true ); \
	setbut->attach( rightOf, fld ); \
    }

uiWellTrackDlg::uiWellTrackDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,mGetDlgSetup(d,"Well Track",mWellTrackDlgHelpID))
	, wd_(d)
	, track_(d.track())
	, orgtrack_(new Well::Track(d.track()))
	, fd_( *Well::TrackAscIO::getDesc() )
	, origpos_(mUdf(Coord3))
	, origgl_(d.info().groundelev)
{
    tbl_ = new uiTable( this, uiTable::Setup().rowdesc("Point")
					      .rowgrow(true)
					      .defrowlbl("")
					      .removeselallowed(false),
		        "Well Track Table" );
    tbl_->setColumnLabels( trackcollbls );
    tbl_->setNrRows( nremptyrows );
    tbl_->setPrefWidth( 500 );
    tbl_->setPrefHeight( 400 );
    tbl_->setTableReadOnly( !writable_ );

    zinftfld_ = new uiCheckBox( this, tr("Z in Feet") );
    zinftfld_->setChecked( SI().depthsInFeet() );
    zinftfld_->activated.notify( mCB( this,uiWellTrackDlg,fillTable ) );
    zinftfld_->activated.notify( mCB( this,uiWellTrackDlg,fillSetFields ) );
    zinftfld_->attach( rightAlignedBelow, tbl_ );

    uiGroup* actbutgrp = new uiGroup( this, "Action buttons grp" );

    uiPushButton* updbut = !writable_ ? 0
		: new uiPushButton( actbutgrp, tr("Update display"),
				   mCB(this,uiWellTrackDlg,updNow), true );
    uiPushButton* setbut = 0;
    wellheadxfld_ = new uiGenInput( actbutgrp, tr("X-Coordinate of well head"),
				    DoubleInpSpec(mUdf(double)) );
    mAddSetBut( wellheadxfld_, updateXpos )
    if ( !updbut )
	actbutgrp->attach( ensureBelow, zinftfld_ );
    else
    {
	wellheadxfld_->attach( ensureBelow, updbut );
	actbutgrp->attach( ensureBelow, tbl_ );
    }
    if ( !writable_ ) wellheadxfld_-> setReadOnly( true );

    wellheadyfld_ = new uiGenInput( actbutgrp, tr("Y-Coordinate of well head"),
				    DoubleInpSpec(mUdf(double)) );
    wellheadyfld_->attach( alignedBelow, wellheadxfld_ );
    mAddSetBut( wellheadyfld_, updateYpos )
    if ( !writable_ ) wellheadyfld_-> setReadOnly( true );

    kbelevfld_ = new uiGenInput( actbutgrp, tr("Reference Datum Elevation"),
				 FloatInpSpec(mUdf(float)) );
    mAddSetBut( kbelevfld_, updateKbElev )
    kbelevfld_->attach( alignedBelow, wellheadyfld_ );
    if ( !writable_ ) kbelevfld_-> setReadOnly( true );

    uiGroup* iobutgrp = new uiButtonGroup( this, "Input/output buttons",
					   OD::Horizontal );
    uiButton* readbut = !writable_ ? 0
		: new uiPushButton( iobutgrp, uiStrings::sImport(),
				    mCB(this,uiWellTrackDlg,readNew), false );
    uiButton* expbut = new uiPushButton( iobutgrp, uiStrings::sExport(),
					 mCB(this,uiWellTrackDlg,exportCB),
					 false );
    if ( readbut )
	expbut->attach( rightOf, readbut );
    iobutgrp->attach( leftAlignedBelow, actbutgrp );

    if ( !track_.isEmpty() )
	origpos_ = track_.pos(0);

    fillTable();
    fillSetFields();
}


uiWellTrackDlg::~uiWellTrackDlg()
{
    delete orgtrack_;
    delete &fd_;
}


bool uiWellTrackDlg::fillTable( CallBacker* )
{
    RowCol curcell( tbl_->currentCell() );

    const int sz = track_.size();
    int newsz = sz + nremptyrows;
    if ( newsz < 8 ) newsz = 8;
    tbl_->setNrRows( newsz );
    tbl_->clearTable();
    if ( !sz ) return false;

    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() )
	fac = zinft ? mToFeetFactorF : 1;
    else
    {
	if ( SI().zInFeet() && !zinft )
	    fac = mFromFeetFactorF;
	else if ( SI().zInMeter() && zinft )
	    fac = mToFeetFactorF;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c( track_.pos(idx) );
	setX( idx, c.x );
	setY( idx, c.y );
	setZ( idx, mCast(float,c.z) * fac );
	setMD( idx, track_.dah(idx) * fac );
    }

    if ( curcell.row() >= newsz ) curcell.row() = newsz-1;
    tbl_->setCurrentCell( curcell );
    return true;
}


void uiWellTrackDlg::fillSetFields( CallBacker* )
{
    NotifyStopper nsx( wellheadxfld_->updateRequested );
    NotifyStopper nsy( wellheadyfld_->updateRequested );
    NotifyStopper nskbelev( kbelevfld_->updateRequested );

    const BufferString xyunit( SI().getXYUnitString() );
    const BufferString depthunit =
			      getDistUnitString( zinftfld_->isChecked(), true );
    BufferString coordlbl( "-Coordinate of well head ", xyunit );

    wellheadxfld_->setTitleText( BufferString("X", coordlbl) );
    wellheadyfld_->setTitleText( BufferString("Y", coordlbl) );
    kbelevfld_->setTitleText(
			tr("Reference Datum Elevation %1").arg(depthunit) );

    Coord wellhead = wd_.info().surfacecoord;
    if ( mIsZero(wellhead.x,0.001) )
	{ wellhead.x = wellhead.y = mUdf(float); }
    wellheadxfld_->setValue( wellhead.x );
    wellheadyfld_->setValue( wellhead.y );

    float kbelev = track_.getKbElev();
    if ( !mIsUdf(kbelev) && zinftfld_->isChecked() )
	kbelev *= mToFeetFactorF;
    kbelevfld_->setValue( kbelev );
}


double uiWellTrackDlg::getX( int row ) const
{
    return tbl_->getdValue( RowCol(row,cXCol) );
}


double uiWellTrackDlg::getY( int row ) const
{
    return tbl_->getdValue( RowCol(row,cYCol) );
}


double uiWellTrackDlg::getZ( int row ) const
{
    return tbl_->getdValue( RowCol(row,cZCol) );
}


float uiWellTrackDlg::getMD( int row ) const
{
    return tbl_->getfValue( RowCol(row,cMDTrackCol) );
}


void uiWellTrackDlg::setX( int row, double x )
{
    tbl_->setValue( RowCol(row,cXCol), x );
}


void uiWellTrackDlg::setY( int row, double y )
{
    tbl_->setValue( RowCol(row,cYCol), y );
}


void uiWellTrackDlg::setZ( int row, double z )
{
    tbl_->setValue( RowCol(row,cZCol), z );
}


void uiWellTrackDlg::setMD( int row, float md )
{
    tbl_->setValue( RowCol(row,cMDTrackCol), md );
}



class uiWellTrackReadDlg : public uiDialog
{ mODTextTranslationClass(uiWellTrackReadDlg);
public:

uiWellTrackReadDlg( uiParent* p, Table::FormatDesc& fd, Well::Track& track )
	: uiDialog(p,uiDialog::Setup(tr("Import New Well Track"),mNoDlgTitle,
                                     mODHelpKey(mWellTrackReadDlgHelpID)))
	, track_(track)
{
    setOkText( uiStrings::sImport() );
    wtinfld_ = new uiFileInput( this, "Well Track File",
    uiFileInput::Setup().withexamine(true) );

    uiTableImpDataSel* dataselfld = new uiTableImpDataSel(this, fd, mNoHelpKey);
    dataselfld->attach( alignedBelow, wtinfld_ );
}


bool acceptOK( CallBacker* )
{
    track_.setEmpty();
    fnm_ = wtinfld_->fileName();
    if ( File::isEmpty(fnm_.buf()) )
	{ uiMSG().error( uiStrings::sInvInpFile() ); return false; }

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
	od_istream strm( dlg.fnm_ );
	if ( !strm.isOK() )
	    { uiMSG().error( uiStrings::sCantOpenInpFile() ); return; }

	Well::TrackAscIO wellascio(fd_, strm );
	if ( !wellascio.getData( wd_, true ) )
	    uiMSG().error( uiStrings::sFailConvCompData() );

	tbl_->clearTable();
	if ( !fillTable() )
	    return;

	wd_.trackchanged.trigger();
    }
    else
	uiMSG().error( tr("Please select a file") );
}


bool uiWellTrackDlg::updNow( CallBacker* )
{
    track_.setEmpty();
    const int nrrows = tbl_->nrRows();
    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() && zinft )
	fac = mFromFeetFactorF;
    else if ( SI().zInFeet() && !zinft )
	fac = mToFeetFactorF;
    else if ( SI().zInMeter() && zinft )
	fac = mFromFeetFactorF;

    bool needfill = false;
    for ( int idx=0; idx<nrrows; idx++ )
    {
	if ( rowIsIncomplete(idx) )
	    continue;

	const double xval = getX( idx );
	const double yval = getY( idx );
	const double zval = getZ( idx ) * fac;
	float dahval = getMD( idx ) * fac;
	const Coord3 newc( xval, yval, zval );
	if ( !SI().isReasonable(newc) )
	{
	    uiString msg = tr("Found undefined values in row %1. "
			      "Please enter valid values")
			 .arg(idx + 1);
	    uiMSG().message( msg );
	    return false;
	}

	if ( idx > 0 && mIsUdf(dahval) )
	{
	    dahval = track_.dah(idx-1) +
		     mCast(float, track_.pos(idx-1).distTo( newc ) );
	    needfill = true;
	}

	track_.addPoint( newc, mCast(float,newc.z), dahval );
    }

    if ( track_.size() > 1 )
    {
	wd_.info().surfacecoord = track_.pos(0);
	fillSetFields();
	wd_.trackchanged.trigger();
    }
    else
    {
	uiMSG().error( tr("Please define at least two points.") );
	return false;
    }

    if ( needfill && !fillTable() )
	return false;

    return true;
}


void uiWellTrackDlg::updateXpos( CallBacker* )
{
    updatePos( true );
}


void uiWellTrackDlg::updateYpos( CallBacker* )
{
    updatePos( false );
}


void uiWellTrackDlg::updatePos( bool isx )
{
    uiGenInput* posfld = isx ? wellheadxfld_ : wellheadyfld_;
    const Coord surfacecoord = wd_.info().surfacecoord;
    double surfacepos = isx ? surfacecoord.x : surfacecoord.y;
    if ( mIsUdf(surfacepos) && !track_.isEmpty() )
	surfacepos = isx ? track_.pos(0).x : track_.pos(0).y;

    const double newpos = posfld->getdValue();
    if ( mIsUdf(newpos) )
    {
	uiMSG().error( tr("Please enter a valid coordinate") );
	posfld->setValue( surfacepos );
	return;
    }

    if ( track_.isEmpty() )
    {
	if ( isx )
	{
	    setX( 0, newpos );
	    setX( 1, newpos );
	}
	else
	{
	    setY( 0, newpos );
	    setY( 1, newpos );
	}
	return;
    }

    updNow(0); //ensure the table contains only the clean track
    posfld->setValue( newpos );
    const int icol = isx ? cXCol : cYCol;
    const double shift = newpos - surfacepos;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const double tblpos = isx ? getX(irow) : getY(irow);
	if ( mIsUdf(tblpos) )
	    continue;

	tbl_->setValue( RowCol(irow,icol), tblpos + shift );
    }

    updNow(0); //write the new table data back to the track
}


void uiWellTrackDlg::updateKbElev( CallBacker* )
{
    float newkbelev = kbelevfld_->getfValue();
    float kbelevorig = track_.isEmpty() ? 0.f : track_.getKbElev();
    if ( mIsUdf(newkbelev) )
    {
	uiMSG().error( tr("Please enter a valid elevation") );
	kbelevfld_->setValue( kbelevorig );
	return;
    }

    if ( track_.isEmpty() )
    {
	setZ( 0, -1.f * newkbelev );
	setMD( 0, 0.f );
	return;
    }

    const bool zinft = zinftfld_->isChecked();
    float fac = 1;
    if ( (SI().zIsTime() && zinft) || (SI().zInMeter() && zinft) )
	fac = mFromFeetFactorF;
    else if ( SI().zInFeet() && !zinft )
	fac = mToFeetFactorF;

    updNow(0); //ensure the table contains only the clean track
    kbelevfld_->setValue( newkbelev );
    kbelevorig *= fac;
    const float shift = kbelevorig - newkbelev;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const double zval = getZ( irow );
	if ( mIsUdf(zval) )
	    continue;

	setZ( irow, mCast(float,zval) + shift * fac );
    }

    updNow(0); //write the new table data back to the track
}


bool uiWellTrackDlg::rowIsIncomplete( int row ) const
{
    return mIsUdf(getX(row)) || mIsUdf(getY(row)) ||
	   mIsUdf(getZ(row)) || mIsUdf(getMD(row));
}


bool uiWellTrackDlg::rejectOK( CallBacker* )
{
    track_ = *orgtrack_;
    wd_.info().surfacecoord = origpos_;
    wd_.info().groundelev = origgl_;
    wd_.trackchanged.trigger();
    return true;
}


bool uiWellTrackDlg::acceptOK( CallBacker* )
{
    if ( !writable_ )
	return true;

    if ( !updNow( 0 ) )
	return false;

    const int nrpts = track_.size();
    if ( nrpts < 2 ) return false;
    const int orgnrpts = orgtrack_->size();
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
	uiString msg = tr("You have changed at least one MD value.\nMarkers%1"
			  " are based on the old MD values.\n. "
			  "They may therefore become invalid.\n\nContinue?");
	if ( SI().zIsTime() )
	    msg.arg(tr(", logs, T/D and checkshot models"));
	else
	    msg.arg(tr(" and logs"));
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    return true;
}


void uiWellTrackDlg::exportCB( CallBacker* )
{
    if ( !track_.size() )
    {
	uiMSG().message( tr("No data available to export") );
	return;
    }

    uiFileDialog fdlg( this, false, 0, 0, tr("File name for export") );
    fdlg.setDirectory( GetDataDir() );
    if ( !fdlg.go() )
	return;

    od_ostream strm( fdlg.fileName() );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr( "Cannot open '%1' for write").arg(fdlg.fileName()) );
	return;
    }

    for ( int idx=0; idx<track_.size(); idx++ )
    {
	const Coord3 coord( track_.pos(idx) );
	strm << coord.x << od_tab << coord.y << od_tab << coord.z
	     << od_tab << track_.dah(idx) << od_newline;
    }
}


// ==================================================================


static const char* sKeyMD()		{ return "MD"; }
static const char* sKeyTVD()		{ return "TVD"; }
static const char* sKeyTVDGL()		{ return "TVDGL"; }
static const char* sKeyTVDSD()		{ return "TVDSD"; }
static const char* sKeyTVDSS()		{ return "TVDSS"; }
static const char* sKeyTWT()		{ return "TWT"; }
static const char* sKeyOWT()		{ return "OWT"; }
static const char* sKeyVint()		{ return "Vint"; }
static const int cMDCol = 0;
static const int cTVDCol = 1;

#define mD2TModel (cksh_ ? wd_.checkShotModel() : wd_.d2TModel())


uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,mGetDlgSetup(wd,mTDName(cksh),mD2TModelDlgHelpID))
	, wd_(wd)
	, cksh_(cksh)
	, orgd2t_(mD2TModel ? new Well::D2TModel(*mD2TModel) : 0)
	, origreplvel_(wd.info().replvel)
        , tbl_(0)
	, replvelfld_(0)
        , unitfld_(0)
        , timefld_(0)
{
    tbl_ = new uiTable( this, uiTable::Setup()
				.rowdesc(cksh_ ? "Measure point" : "Control Pt")
				.rowgrow(true)
				.defrowlbl("")
				.selmode(uiTable::Single)
				.removeselallowed(false), mTDName(cksh) );
    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );
    tbl_->setNrRows( nremptyrows );
    tbl_->valueChanged.notify( mCB(this,uiD2TModelDlg,dtpointChangedCB) );
    tbl_->rowDeleted.notify( mCB(this,uiD2TModelDlg,dtpointRemovedCB) );
    tbl_->setColumnToolTip( cMDCol,
	   "Measured depth along the borehole, origin at Reference Datum" );
    tbl_->setColumnToolTip( cTVDCol,
	    "True Vertical Depth, origin at Reference Datum" );
    tbl_->setColumnToolTip( getTVDSSCol(),
	    "True Vertical Depth Sub-Sea, positive downwards" );
    tbl_->setColumnToolTip( getTimeCol(),
	    "Two-way travel times with same origin as seismic survey: SRD=0" );
    tbl_->setColumnToolTip( getVintCol(),
	    "Interval velocity above this control point (read-only)" );
    tbl_->setPrefWidth( 700 );
    tbl_->setTableReadOnly( !writable_ );

    timefld_ = new uiCheckBox( this, tr(" Time is TWT") );
    timefld_->setChecked( true );
    timefld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );
    timefld_->attach( rightAlignedBelow, tbl_ );

    unitfld_ = new uiCheckBox( this, tr(" Z in feet") );
    unitfld_->setChecked( SI().depthsInFeet() );
    unitfld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );
    unitfld_->attach( leftOf, timefld_ );

    if ( !cksh_ )
    {
	replvelfld_ = new uiGenInput( this, tr("Replacement velocity"),
				      FloatInpSpec(mUdf(float)) );
	if ( !writable_ )
	    replvelfld_-> setReadOnly( true );
	else
	{
	    uiButton* updbut = new uiPushButton( this, tr("Update display"),
				       mCB(this,uiD2TModelDlg,updNow), true );
	    updbut->attach( ensureBelow, tbl_ );
	    replvelfld_->updateRequested.notify(
					mCB(this,uiD2TModelDlg,updReplVelNow) );
	    uiPushButton* setbut = new uiPushButton( this, tr("Set"),
			      mCB(this,uiD2TModelDlg,updReplVelNow), true );
	    setbut->attach( rightOf, replvelfld_ );
	}
	replvelfld_->attach( ensureBelow, unitfld_ );
	unitfld_->activated.notify( mCB(this,uiD2TModelDlg,fillReplVel) );
    }

    uiGroup* iobutgrp = new uiButtonGroup( this, "Input/output buttons",
					   OD::Horizontal );
    if ( writable_ )
	new uiPushButton( iobutgrp, uiStrings::sImport(),
	    mCB(this,uiD2TModelDlg,readNew), false );
    new uiPushButton( iobutgrp, uiStrings::sExport(),
        mCB(this,uiD2TModelDlg,expData), false );
    if ( replvelfld_ )
	iobutgrp->attach( ensureBelow, replvelfld_ );
    else
	iobutgrp->attach( ensureBelow, unitfld_ );

    correctD2TModelIfInvalid();

    fillTable(0);
    if ( !cksh_ )
	fillReplVel(0);
}


void uiD2TModelDlg::getColLabels( BufferStringSet& lbls ) const
{
    bool zinfeet = false;
    if ( unitfld_ )
	zinfeet = unitfld_->isChecked();

    bool timeisoneway = false;
    if ( timefld_ )
	timeisoneway = !timefld_->isChecked();

    const BufferString depthunit = getDistUnitString( zinfeet, true );

    BufferString curlbl( sKeyMD() );
    curlbl.add( depthunit );
    lbls.add( curlbl );

    curlbl.set( sKeyTVD() );
    curlbl.add( depthunit );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDGLCol()) )
    {
	curlbl.set( sKeyTVDGL() );
	curlbl.add( depthunit );
	lbls.add( curlbl );
    }

    curlbl.set( sKeyTVDSS() );
    curlbl.add( depthunit );
    lbls.add( curlbl );

    if ( !mIsUdf(getTVDSDCol()) )
    {
	curlbl.set( sKeyTVDSD() );
	curlbl.add( depthunit );
	lbls.add( curlbl );
    }

    curlbl.set( timeisoneway ? sKeyOWT() : sKeyTWT() );
    curlbl.add( "(ms)" );
    lbls.add( curlbl );

    curlbl.set( sKeyVint() );
    curlbl.add( "(" ).add( getDistUnitString(zinfeet,false) ).add( "/s)" );
    lbls.add( curlbl );
}


int uiD2TModelDlg::getTVDGLCol() const
{
    return mIsUdf( wd_.info().groundelev ) ? mUdf( int ) : cTVDCol + 1;
}


int uiD2TModelDlg::getTVDSSCol() const
{
    return mIsUdf( getTVDGLCol() ) ? cTVDCol + 1 : getTVDGLCol() + 1;
}


int uiD2TModelDlg::getTVDSDCol() const
{
    return mIsZero(SI().seismicReferenceDatum(),1e-3) ? mUdf( int )
						      : getTVDSSCol() + 1;
}


int uiD2TModelDlg::getTimeCol() const
{
    return mIsUdf( getTVDSDCol() ) ? getTVDSSCol() + 1
				   : getTVDSDCol() + 1;
}


int uiD2TModelDlg::getVintCol() const
{
    return getTimeCol() + 1;
}

#define mGetVel(dah,d2t) \
{ \
    const Interval<float> replvellayer( wd_.track().getKbElev(), srd ); \
    vint = replvellayer.includes( -1.f * wd_.track().getPos(dah).z, true ) && \
	   !mIsUdf(wd_.info().replvel) \
	 ? wd_.info().replvel \
	 : mCast(float,d2t->getVelocityForDah( dah, wd_.track() )); \
}


void uiD2TModelDlg::fillTable( CallBacker* )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 )
	return;

    if ( tracksz<2 )
    {
	uiMSG().error( tr("Invalid track") );
	return;
    }

    const int dtsz = d2t->size();
    tbl_->setNrRows( dtsz + nremptyrows );
    BufferStringSet header;
    getColLabels( header );
    tbl_->setColumnLabels( header );

    const float zfac = unitfld_->isChecked() ? mToFeetFactorF : 1;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    float vint;
    for ( int idx=0; idx<dtsz; idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mCast(float,track.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	mGetVel(dah,d2t)
	tbl_->setValue( RowCol(idx,cMDCol), dah * zfac );
	tbl_->setValue( RowCol(idx,cTVDCol), tvd * zfac );
	if ( hastvdgl )
	{
	    const float tvdgl = ( tvdss + groundevel ) * zfac;
	    tbl_->setValue( RowCol(idx,getTVDGLCol()), tvdgl );
	}

	if ( hastvdsd )
	{
	    const float tvdsd = ( tvdss + srd ) * zfac;
	    tbl_->setValue( RowCol(idx,getTVDSDCol()), tvdsd );
	}

	tbl_->setValue( RowCol(idx,getTVDSSCol()), tvdss * zfac );
	tbl_->setValue( RowCol(idx,getTimeCol()), d2t->t(idx) * twtfac );
	tbl_->setValue( RowCol(idx,getVintCol()), vint * zfac );
    }
    tbl_->setColumnReadOnly( getVintCol(), true );
}


void uiD2TModelDlg::fillReplVel( CallBacker* )
{
    NotifyStopper ns( replvelfld_->updateRequested );
    BufferString lbl( "Replacement velocity (",
		      getDistUnitString(unitfld_->isChecked(),false), "/s)" );
    replvelfld_->setTitleText( lbl );
    float replvel = wd_.info().replvel;
    if ( !mIsUdf(replvel) && unitfld_->isChecked() )
	replvel *= mToFeetFactorF;

    replvelfld_->setValue( replvel );
}


uiD2TModelDlg::~uiD2TModelDlg()
{
    delete orgd2t_;
}


void uiD2TModelDlg::dtpointChangedCB( CallBacker* )
{
    const int row = tbl_->currentRow();
    const int col = tbl_->currentCol();

    if ( col < getTimeCol() )
	updateDtpointDepth( row );
    else if ( col == getTimeCol() )
	updateDtpointTime( row );
}


void uiD2TModelDlg::dtpointRemovedCB( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size()<3 )
    {
	uiMSG().error(tr("Invalid time-depth model"));
	return;
    }

    const int row = tbl_->currentRow();
    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
    int idah = d2t->indexOf( tbl_->getfValue(RowCol(row,cMDCol)) / zfac );
    if ( ( rowIsIncomplete(row) && !mIsUdf(getNextCompleteRowIdx(row)) ) ||
	 mIsUdf(idah) )
	return;

    d2t->remove( idah-1 );
    const int nextrow = getNextCompleteRowIdx( row-1 );
    if ( mIsUdf(nextrow) )
	return;

    idah = d2t->indexOf( tbl_->getfValue(RowCol(nextrow,cMDCol)) / zfac );
    const float olddah = d2t->dah( idah );
    updateDtpoint( nextrow, olddah );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
bool uiD2TModelDlg::updateDtpointDepth( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
    const bool newrow = rowIsIncomplete( row );

    int incol = tbl_->currentCol();
    const bool md2tvd = incol == cMDCol;
    const bool inistvd = incol == cTVDCol;
    bool inistvdss = incol == getTVDSSCol();
    if ( incol == getTimeCol() )
    {
	inistvdss = true; // special case for replacement velocity zone update
	incol = getTVDSSCol();
    }

    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool inistvdgl = hastvdgl && incol == getTVDGLCol();

    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    const bool inistvdsd = hastvdsd && incol == getTVDSDCol();

    const float olddah = newrow ? mUdf(float) : d2t->dah( row );
    const float oldtvdss = newrow ? mUdf(float) : (float)track.getPos(olddah).z;
    float oldval = mUdf( float );
    if ( inistvd && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + kbelev;
    else if ( inistvdgl && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + groundevel;
    else if ( inistvdsd && !mIsUdf(oldtvdss) )
	oldval = oldtvdss + srd;
    else if ( inistvdss && !mIsUdf(oldtvdss) )
	oldval = oldtvdss;

    const RowCol rcin(row,incol);
    if ( mIsUdf(tbl_->getfValue(rcin)) )
    {
	uiMSG().error( tr("Please enter a valid number") );
	if ( !newrow )
	    tbl_->setValue( rcin, oldval * zfac );

	return false;
    }

    float inval = tbl_->getfValue( rcin ) / zfac;
    Interval<float> dahrg = track.dahRange();
    Interval<float> zrg( track.value( 0 ), track.value( tracksz - 1 ) );
    if ( inistvd )
	zrg.shift( kbelev );
    else if ( inistvdgl )
	zrg.shift( groundevel );
    else if ( inistvdsd )
	zrg.shift( srd );

    const Interval<float>& zrange = md2tvd ? dahrg : zrg;
    Interval<float> tblrg;
    const int prevrow = getPreviousCompleteRowIdx( row );
    const int nextrow = getNextCompleteRowIdx( row );
    tblrg.start = row == 0 || mIsUdf(prevrow) ? zrange.start
	       : tbl_->getfValue( RowCol(prevrow,incol) ) / zfac;
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? zrange.stop
	       : tbl_->getfValue( RowCol(nextrow,incol) ) / zfac;

    BufferString lbl;
    if ( md2tvd ) lbl = sKeyMD();
    else if ( inistvd ) lbl =  sKeyTVD();
    else if ( inistvdgl ) lbl = sKeyTVDGL();
    else if ( inistvdsd ) lbl = sKeyTVDSD();
    else if ( inistvdss ) lbl = sKeyTVDSS();

    uiString errmsg = tr("The entered %1");
    if ( !zrange.includes(inval,true) )
    {
	errmsg.arg(tr("%1 value %2 is outside of track range\n[%3, %4]%5 (%6)")
	      .arg(lbl).arg(inval * zfac).arg(zrange.start * zfac)
	      .arg(zrange.stop * zfac)
	      .arg(getDistUnitString(unitfld_->isChecked(), false)).arg(lbl));
	tbl_->setValue( rcin, !newrow ? oldval * zfac : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( !tblrg.includes(inval,true) )
    {
	errmsg.arg(tr("%1 is not between the depths of the previous and "
		      "next control points").arg(lbl));
	tbl_->setValue( rcin, !newrow ? oldval * zfac : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( !md2tvd )
    {
	if ( inistvd ) inval -= kbelev;
	else if ( inistvdgl ) inval -= groundevel;
	else if ( inistvdsd ) inval -= srd;
    }

    const float dah = md2tvd ? inval : track.getDahForTVD( inval );
    const float tvdss = !md2tvd ? inval : mCast(float,track.getPos(inval).z);
    if ( !md2tvd )
	tbl_->setValue( RowCol(row,cMDCol), dah * zfac );

    if ( !inistvdss )
	tbl_->setValue( RowCol(row,getTVDSSCol()), tvdss * zfac );

    if ( !inistvd )
	tbl_->setValue( RowCol(row,cTVDCol), ( tvdss + kbelev ) * zfac );

    if ( hastvdgl && !inistvdgl )
	tbl_->setValue( RowCol(row,getTVDGLCol()), ( tvdss + groundevel )*zfac);

    if ( hastvdsd && !inistvdsd )
	tbl_->setValue( RowCol(row,getTVDSDCol()), ( tvdss + srd ) * zfac );

    const float twtfac = timefld_->isChecked() ? 2000.f : 1000.f;
    Interval<float> replvelint( track.getKbElev(), srd );
    if ( replvelint.includes(-1.f*tvdss,true) && !mIsUdf(wd_.info().replvel) )
    {
	const float twt = twtfac * (tvdss + srd ) / wd_.info().replvel;
	tbl_->setValue( RowCol(row,getTimeCol() ), twt );
    }

    return updateDtpoint( row, olddah );
}


bool uiD2TModelDlg::updateDtpointTime( int row )
{
    NotifyStopper ns( tbl_->valueChanged );
    const Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    const RowCol rcin(row,getTimeCol());
    const bool newrow = rowIsIncomplete( row );
    const float oldval = newrow ? mUdf(float) : d2t->value( row );
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    if ( mIsUdf(tbl_->getfValue(rcin)) )
    {
	uiMSG().error( tr("Please enter a valid number") );
	if ( !newrow )
	    tbl_->setValue( rcin, oldval * twtfac );

	return false;
    }

    const float inval = tbl_->getfValue( rcin ) / twtfac;
    Interval<float> dahrg = track.dahRange();
    Interval<float> timerg;
    timerg.start = d2t->getTime( dahrg.start, track );
    timerg.stop = d2t->getTime( dahrg.stop, track );
    Interval<float> tblrg;
    const int prevrow = getPreviousCompleteRowIdx( row );
    const int nextrow = getNextCompleteRowIdx( row );
    tblrg.start = row == 0 || mIsUdf(prevrow) ? timerg.start
	       : tbl_->getfValue( RowCol(prevrow,getTimeCol()) ) / twtfac;
    tblrg.stop = d2t->size() < row || mIsUdf(nextrow) ? timerg.stop
	       : tbl_->getfValue( RowCol(nextrow,getTimeCol()) ) / twtfac;

    if ( !tblrg.includes(inval,true) &&
	 !mIsUdf(getPreviousCompleteRowIdx(row)) &&
	 !mIsUdf(getNextCompleteRowIdx(row)) )
    {
	uiString errmsg = tr("The entered time is not between the times "
			     "of the previous and next control points" );
	tbl_->setValue( rcin, !newrow ? oldval * twtfac : mUdf(float) );
	mErrRet(errmsg)
    }

    if ( inval < 0.f && !mIsUdf(wd_.info().replvel) )
    {
	const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
	const float tvdss = ( wd_.info().replvel * inval * 500.f / twtfac
			  - mCast(float, SI().seismicReferenceDatum()) ) * zfac;
	const RowCol rc( row, getTVDSSCol() );
	tbl_->setValue( rc, tvdss );
	tbl_->setSelected( rc );
	updateDtpointDepth( row );
    }

    return updateDtpoint( row, oldval );
}


bool uiD2TModelDlg::updateDtpoint( int row, float oldval )
{
    NotifyStopper ns( tbl_->valueChanged );
    tbl_->setColumnReadOnly( getVintCol(), false );
    Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	mErrRet(errmsg)
    }

    const RowCol rcdah(row,cMDCol);
    const RowCol rctwt(row,getTimeCol());
    if ( mIsUdf(tbl_->getfValue(rcdah)) || mIsUdf(tbl_->getfValue(rctwt)) )
	return true; // not enough data yet to proceed

    if ( !rowIsIncomplete(row) )
    {
	const bool oldvalisdah = tbl_->currentCol() < getTimeCol();
	const float olddah = oldvalisdah ? oldval : d2t->getDah( oldval, track);
	const int dahidx = d2t->indexOf( olddah );
	d2t->remove( dahidx );
    }

    const float zfac = !unitfld_->isChecked() ? 1.f : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float dah = tbl_->getfValue( rcdah ) / zfac;
    const float twt = tbl_->getfValue( rctwt ) / twtfac;
    d2t->insertAtDah( dah, twt );
    wd_.d2tchanged.trigger();

    const float srd = mCast(float,SI().seismicReferenceDatum());
    float vint;
    mGetVel(dah,d2t);
    const RowCol rcvint(row,getVintCol());
    tbl_->setValue( rcvint, vint * zfac );

    for ( int irow=row+1; irow<tbl_->nrRows(); irow++ )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	const float nextdah = tbl_->getfValue( RowCol(irow,cMDCol) ) / zfac;
	mGetVel(nextdah,d2t);
	tbl_->setValue( RowCol(irow,getVintCol()), vint * zfac );
	break;
    }

    tbl_->setColumnReadOnly( getVintCol(), true );
    return true;
}


bool uiD2TModelDlg::rowIsIncomplete( int row ) const
{
    Well::D2TModel* d2t = mD2TModel;
    if ( !d2t || d2t->size()<2 )
	mErrRet( tr("Invalid time-depth model") )

    if ( row >= d2t->size() )
	return true;

    return mIsUdf( tbl_->getfValue( RowCol(row,getVintCol()) ) );
}


int uiD2TModelDlg::getPreviousCompleteRowIdx( int row ) const
{
    for ( int irow=row-1; irow>=0; irow-- )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	return irow;
    }

    return mUdf(int);
}


int uiD2TModelDlg::getNextCompleteRowIdx( int row ) const
{
    for ( int irow=row+1; irow<tbl_->nrRows(); irow++ )
    {
	if ( rowIsIncomplete(irow) )
	    continue;

	return irow;
    }

    return mUdf(int);
}



class uiD2TModelReadDlg : public uiDialog
{ mODTextTranslationClass(uiD2TModelReadDlg);
public:

uiD2TModelReadDlg( uiParent* p, Well::Data& wd, bool cksh )
	: uiDialog(p,uiDialog::Setup( mTDOpName("Import",cksh), mNoDlgTitle,
		     mODHelpKey(mD2TModelReadDlgHelpID) ))
	, cksh_(cksh)
	, wd_(wd)
{
    setOkText( uiStrings::sImport() );
    uiD2TModelGroup::Setup su( false );
    su.filefldlbl( "File name" );
    d2tgrp = new uiD2TModelGroup( this, su );
}


bool acceptOK( CallBacker* )
{
    if ( !d2tgrp->getD2T(wd_,cksh_) )
    {
	if ( !d2tgrp->errMsg().isEmpty() )
	    uiMSG().error( d2tgrp->errMsg() );

	return false;
    }
    if ( !d2tgrp->warnMsg().isEmpty() )
	uiMSG().warning( d2tgrp->warnMsg() );

    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	d2t->deInterpolate();

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

    if ( !dlg.d2tgrp->getD2T(wd_,cksh_) )
	return;
    else
    {
	tbl_->clearTable();
	fillTable(0);
	wd_.d2tchanged.trigger();
    }
}


void uiD2TModelDlg::expData( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );
    if ( d2t->size() < 2 )
	{ uiMSG().error( tr("No valid data entered") ); return; }

    uiFileDialog dlg( this, false, 0, 0, tr("Filename for export") );
    dlg.setDirectory( GetDataDir() );
    if ( !dlg.go() )
	return;

    const BufferString fnm( dlg.fileName() );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open '%1' for write").arg(fnm) );
	return;
    }

    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactorF;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const float kbelev = wd_.track().getKbElev();
    const float groundevel = wd_.info().groundelev;
    const float srd = mCast(float,SI().seismicReferenceDatum());
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    BufferStringSet header;
    getColLabels( header );

    strm << header.get( cMDCol ) << od_tab <<  header.get( cTVDCol ) << od_tab;
    if ( hastvdgl )
	strm << header.get( getTVDGLCol() ) << od_tab;

    strm << header.get( getTVDSSCol() ) << od_tab;
    if ( hastvdsd )
	strm << header.get( getTVDSDCol() ) << od_tab;

    strm << header.get( getTimeCol() ) << od_tab;
    strm << header.get( getVintCol() ) << od_newline;
    float vint;
    for ( int idx=0; idx<d2t->size(); idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mCast(float,wd_.track().getPos(dah).z) * zfac;
	const float tvd = tvdss + kbelev * zfac;
	const float twt = d2t->t(idx) * twtfac;
	mGetVel(dah,d2t);
	strm << dah * zfac << od_tab << tvd << od_tab;
	if ( hastvdgl )
	{
	    const float tvdgl = tvdss + groundevel * zfac;
	    strm << tvdgl << od_tab;
	}
	strm << tvdss << od_tab;
	if ( hastvdsd )
	{
	    const float tvdsd = tvdss + srd * zfac;
	    strm << tvdsd << od_tab;
	}
	strm << twt << od_tab << vint * zfac << od_newline;
    }
}


bool uiD2TModelDlg::getFromScreen()
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );

    if ( wd_.track().zRange().stop < SI().seismicReferenceDatum() && !d2t )
	return true;

    if ( d2t->size() < 2 )
	mErrRet( tr("Please define at least two control points.") )

    return true;
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    if ( !getFromScreen() )
	return;

    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	wd_.d2tchanged.trigger();

    wd_.trackchanged.trigger();
}


void uiD2TModelDlg::updReplVelNow( CallBacker* )
{
    float replvel = replvelfld_->getfValue();
    if ( mIsUdf(replvel) || replvel < 0.001f )
    {
	uiMSG().error( tr("Please enter a valid replacement velocity") );
	replvelfld_->setValue( !unitfld_->isChecked() ? wd_.info().replvel
			       : wd_.info().replvel * mToFeetFactorF );
	return;
    }

    if ( unitfld_->isChecked() )
	replvel *= mFromFeetFactorF;

    Well::D2TModel* d2t = mD2TModel;
    const Well::Track& track = wd_.track();
    if ( track.zRange().stop < SI().seismicReferenceDatum() && !d2t )
    {
	wd_.info().replvel = replvel;
	updNow(0);
	return;
    }

    const int tracksz = wd_.track().size();
    if ( !d2t || d2t->size()<2 || tracksz<2 )
    {
	uiString errmsg = tracksz<2 ? tr("Invalid track")
				    : tr("Invalid time-depth model");
	uiMSG().error( errmsg );
	return;
    }

    const float kbelev = track.getKbElev();
    const float srdelev = mCast(float,SI().seismicReferenceDatum());
    const bool kbabovesrd = srdelev < kbelev;
    const float firstdah = kbabovesrd ? track.getDahForTVD(-1.f*srdelev) : 0.f;
    const float firsttwt = d2t->getTime( firstdah, track );
    if ( mIsUdf(firstdah) || mIsUdf(firsttwt) )
	return;

    const float zwllhead = track.value(0);
    const float replveldz = zwllhead + srdelev;
    const float timeshift = kbabovesrd ? firsttwt
				       : 2.f * replveldz / replvel - firsttwt;

    const float zfac = unitfld_->isChecked() ? mFromFeetFactorF : 1.f;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    for( int irow=tbl_->nrRows()-1; irow>=0; irow-- )
    {
	const float dah = tbl_->getfValue( RowCol(irow,cMDCol) ) * zfac;
	const float twt = tbl_->getfValue( RowCol(irow,getTimeCol()) );
	if ( mIsUdf(dah) || mIsUdf(twt) )
	    continue;

	if ( dah < firstdah || twt + timeshift * twtfac < 0.f)
	    tbl_->removeRow( irow );
    }

    NotifyStopper ns( tbl_->valueChanged );
    for( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	const float twt = tbl_->getfValue( RowCol(irow,getTimeCol()) );
	if ( mIsUdf(twt) )
	    continue;

	tbl_->setValue( RowCol(irow,getTimeCol()), twt + timeshift * twtfac );
    }

    wd_.info().replvel = replvel;
    updNow(0);
}


void uiD2TModelDlg::getModel( Well::D2TModel& d2t )
{
    d2t.setEmpty();
    const float zfac = unitfld_->isChecked() ? mToFeetFactorF : 1;
    const float twtfac = timefld_->isChecked() ? 1000.f : 500.f;
    const int nrrows = tbl_->nrRows();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( mIsUdf(tbl_->getfValue( RowCol(irow,getVintCol()))) )
	    continue;

	const float dah = tbl_->getfValue( RowCol(irow,cMDCol) ) / zfac;
	const float twt = tbl_->getfValue( RowCol(irow,getTimeCol()) ) / twtfac;
	d2t.add( dah, twt );
    }
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	*d2t = *orgd2t_;

    wd_.d2tchanged.trigger();
    wd_.trackchanged.trigger();
    wd_.info().replvel = origreplvel_;

    return true;
}


void uiD2TModelDlg::correctD2TModelIfInvalid()
{
    Well::D2TModel* d2t = mD2TModel;
    bool needrestore = false;
    if ( !d2t->ensureValid( wd_.track(), wd_.info().replvel ) ||
	 d2t->size() < 2 )
    {
	uiMSG().warning( tr("Invalid model detected\n"
			    "But could not autocorrect the current model") );
	if ( *d2t != *orgd2t_ )
	    needrestore = true;
    }
    else
    {
	if ( *d2t != *orgd2t_ )
	{
	    needrestore = !uiMSG().askGoOn( tr("Invalid model detected\n"
					       "Auto-correct?\n"
	    "(New model will only by saved on disk on successful exit of"
	    " the editor)") );

	    if ( !needrestore )
	    {
		uiMSG().message( tr("Time-depth model succesfully corrected") );
	    }
	}
    }

    if ( needrestore )
	*d2t = *orgd2t_;

    wd_.d2tchanged.trigger();
    wd_.trackchanged.trigger();
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    if ( !writable_ )
	return true;

    if ( !getFromScreen() )
	return false;

    wd_.d2tchanged.trigger();
    wd_.trackchanged.trigger();
    return true;
}


//============================================================================

uiNewWellDlg::uiNewWellDlg( uiParent* p )
        : uiGetObjectName(p,uiGetObjectName::Setup(tr("New Well"),mkWellNms())
				.inptxt(tr("New well name")) )
{
    setHelpKey( mODHelpKey(mNewWellTrackDlgHelpID) );
    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
				      .lbltxt(uiStrings::sColor()) );
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
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( ioobj )
	    nms_->add( ioobj->name() );
    }
    return *nms_;
}


bool uiNewWellDlg::acceptOK( CallBacker* )
{
    BufferString newnm( text() );
    if ( newnm.trimBlanks().isEmpty() )
	mErrRet( tr("Please enter a name") )

    if ( nms_->isPresent(newnm) )
	mErrRet( tr("Please specify a new name.\n"
		    "Wells can be removed in 'Manage wells'") )

    name_ = newnm;
    return true;
}


const Color& uiNewWellDlg::getWellColor()
{
    return colsel_->color();
}


//============================================================================

static const char* collbls[] = { "Well name","Log name",
				 "Unit of measure", 0 };
uiWellLogUOMDlg::uiWellLogUOMDlg( uiParent* p, ObjectSet<Well::LogSet> wls,
				  const BufferStringSet wellnms,
				  const BufferStringSet lognms )
    : uiDialog(p,uiDialog::Setup(tr("Set units of measure for logs"),
				 mNoDlgTitle,mNoHelpKey))
{
    fillTable( wls, wellnms, lognms );
}


void uiWellLogUOMDlg::fillTable( ObjectSet<Well::LogSet> wls,
			    const BufferStringSet& wellnms,
			    const BufferStringSet& lognms )
{
    uominfotbl_ = new uiTable( this, uiTable::Setup()
				    .manualresize(true)
				    .fillrow(true)
				    .removeselallowed(false),
				"Units info" );
    uominfotbl_->setPrefWidth( 520 );
    uominfotbl_->setPrefHeight( 400 );
    uominfotbl_->setTableReadOnly( true );
    uominfotbl_->setColumnLabels( collbls );
    uominfotbl_->setColumnResizeMode( uiTable::ResizeToContents );
    const int nrwls = wls.size();
    const int nrlogs = lognms.size();
    const int nrrows = nrwls*nrlogs;
    uominfotbl_->setNrRows( nrrows );
    int rowidx = -1;
    for ( int wlsidx=0; wlsidx<nrwls; wlsidx++ )
    {
	for ( int lidx=0; lidx<nrlogs; lidx++ )
	{
	    rowidx++;
	    Well::Log* log = wls[wlsidx]->getLog( lognms.get(lidx ) );
	    if ( !log )
	    {
		uominfotbl_->removeRow( rowidx );
		rowidx--;
		continue;
	    }

	    logs_ += log;
	    const char* curruom = log->unitMeasLabel();
	    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( curruom );

	    PropertyRef::StdType ptyp = PropertyRef::Other;
	    if ( uom ) ptyp = uom->propType();
	    uiUnitSel::Setup ussu( ptyp, "" );
	    ussu.selproptype( true );
	    uiUnitSel* unfld = new uiUnitSel( 0, ussu );
	    unfld->setUnit( uom );
	    unflds_ += unfld;
	    uominfotbl_->setText( RowCol(rowidx,0), wellnms.get(wlsidx) );
	    uominfotbl_->setText( RowCol(rowidx,1), lognms.get(lidx ) );
	    uominfotbl_->setCellGroup( RowCol(rowidx,2), unfld );
	}
    }
}


bool uiWellLogUOMDlg::setUoMValues()
{
    const int logssz = logs_.size();
    if ( !logssz || logssz!=uominfotbl_->nrRows() )
    {
	uiMSG().message( tr("No logs found.") );
	return false;
    }

    const int nrrows = uominfotbl_->nrRows();
    for ( int lidx=0; lidx<nrrows; lidx++ )
    {
	Well::Log* log = logs_[lidx];
	if ( !log )
	    continue;

	const UnitOfMeasure* newuom = unflds_[lidx]->getUnit();
	log->setUnitMeasLabel( newuom ? newuom->name().buf() : 0 );
    }

    return true;
}


bool uiWellLogUOMDlg::acceptOK( CallBacker* )
{
    return setUoMValues();
}
