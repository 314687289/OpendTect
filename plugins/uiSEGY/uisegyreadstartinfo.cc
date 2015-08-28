/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyreadstartinfo.h"
#include "segyuiscandata.h"
#include "segyhdr.h"
#include "uitable.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uisegyimptype.h"
#include "uisegydef.h"

#define mNrInfoRows 9
#define mNrInfoCols 4

#define mRevRow 0
#define mDataFormatRow 1
#define mNrSamplesRow 2
#define mZRangeRow 3
#define mKey1Row 4
#define mKey2Row 5
#define mXRow 6
#define mYRow 7
#define mPSRow 8
#define mHaveSpecRows (nrrows_ > mKey1Row)
#define mHavePSRows (nrrows_ > mPSRow)

#define mItemCol 0
#define mQSResCol 1
#define mUseTxtCol 2
#define mUseCol 3

#define mGetTypeVars \
    const Seis::GeomType gt = imptype_.geomType(); \
    const bool isvsp = imptype_.isVSP(); \
    const bool is2d = Seis::is2D( gt ); \
    const bool isps = Seis::isPS( gt ); \
    const bool isrev0 = loaddef_.isRev0()

static const char* sBytePos = "from header";
static const Color qscellcolor = Color( 255, 255, 225 ); // palish yellow


class uiSEGYByteNr : public uiComboBox
{
public:

uiSEGYByteNr( uiParent* p, const char* nm )
    : uiComboBox(p,nm)
    , hdef_(SEGY::TrcHeader::hdrDef())
{
    for ( int idx=0; idx<hdef_.size(); idx++ )
    {
	const SEGY::HdrEntry& he = *hdef_[idx];
	BufferString txt( he.name(), "(byte " );
        txt.add( he.bytepos_+1 ).add( ") - \"" );
        txt.add( he.description() ).add( "\"" );
	addItem( txt );
    }
}

SEGY::HdrEntry hdrEntry() const
{
    const int selidx = currentItem();
    SEGY::HdrEntry ret;
    if( selidx >= 0 )
    {
	ret = *hdef_[selidx];
	ret.bytepos_++; // convert to 'user' byte number
    }
    return ret;
}

void setByteNr( short bnr )
{
    bnr--; // input will be 'user' byte number

    int selidx = -1;
    for ( int idx=0; idx<hdef_.size(); idx++ )
    {
	const SEGY::HdrEntry& he = *hdef_[idx];
	if ( he.bytepos_ == bnr )
	    { selidx = idx; break; }
    }
    if ( selidx >= 0 )
	setCurrentItem( selidx );
}

    const SEGY::HdrDef&	hdef_;

};


uiSEGYReadStartInfo::uiSEGYReadStartInfo( uiParent* p, SEGY::LoadDef& scd,
					  const SEGY::ImpType* imptyp )
    : uiGroup(p,"SEGY read start info")
    , loaddef_(scd)
    , loaddefChanged(this)
    , parsbeingset_(false)
    , xcoordbytefld_(0)
    , ycoordbytefld_(0)
    , key1bytefld_(0)
    , key2bytefld_(0)
    , offsetbytefld_(0)
    , inptypfixed_(imptyp)
{
    nrrows_ = mNrInfoRows;
    if ( inptypfixed_ )
    {
	imptype_ = *imptyp;
	if ( imptype_.isVSP() )
	    nrrows_ = 4;
	else if ( !Seis::isPS(imptype_.geomType()) )
	    nrrows_ = 8;
    }

    tbl_ = new uiTable( this, uiTable::Setup(nrrows_,mNrInfoCols)
				  .manualresize(true), "Info table" );
    tbl_->setColumnLabel( mItemCol, "" );
    tbl_->setColumnLabel( mQSResCol, "Quick scan result" );
    tbl_->setColumnLabel( mUseTxtCol, "" );
    tbl_->setColumnLabel( mUseCol, "Actually use" );
    tbl_->setColumnStretchable( mItemCol, false );
    tbl_->setColumnStretchable( mQSResCol, true );
    tbl_->setColumnStretchable( mUseTxtCol, false );
    tbl_->setColumnStretchable( mUseCol, true );
    tbl_->setPrefWidthInChar( 120 );
    tbl_->setPrefHeightInRows( nrrows_ );
    tbl_->setTableReadOnly( true );
    tbl_->setLeftHeaderHidden( true );
    setCellTxt( mItemCol, mRevRow, "SEG-Y Revision" );
    setCellTxt( mUseTxtCol, mRevRow, "" );
    setCellTxt( mItemCol, mDataFormatRow, "Data format" );
    setCellTxt( mUseTxtCol, mDataFormatRow, "" );
    setCellTxt( mItemCol, mNrSamplesRow, "Number of samples" );
    setCellTxt( mUseTxtCol, mNrSamplesRow, "" );
    setCellTxt( mItemCol, mZRangeRow, "Z Range" );
    setCellTxt( mUseTxtCol, mZRangeRow, "start / interval" );

    mkCommonLoadDefFields();
}


#   define mAddToTbl(fld,row) \
    fld->setStretch( 2, 1 ); \
    tbl_->setCellObject( RowCol(row,mUseCol), fld );

#   define mRemoveFromTable(fld,row) { \
    if ( fld ) \
    { \
	tbl_->clearCellObject( RowCol(row,mUseCol) ); \
	fld = 0; \
    } }


void uiSEGYReadStartInfo::mkCommonLoadDefFields()
{
    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );

    const char* revstrs[] = { "0", "1", "2", 0 };
    revfld_ = new uiComboBox( 0, revstrs, "Revision" );
    revfld_->selectionChanged.notify( mCB(this,uiSEGYReadStartInfo,revChg) );
    mAddToTbl( revfld_, mRevRow );

    fmtfld_ = new uiComboBox( 0, SEGY::FilePars::getFmts(false), "Format" );
    fmtfld_->selectionChanged.notify( parchgcb );
    mAddToTbl( fmtfld_, mDataFormatRow );

    nsfld_ = new uiSpinBox( 0, 0, "Samples" );
    nsfld_->setInterval( 1, mMaxReasonableNS, 1 );
    nsfld_->valueChanged.notify( parchgcb );
    mAddToTbl( nsfld_, mNrSamplesRow );

    uiGroup* grp = new uiGroup( 0, "Z Range" );
    zstartfld_ = new uiLineEdit( grp, FloatInpSpec(0.f), "Z Start" );
    zstartfld_->editingFinished.notify( parchgcb );
    zstartfld_->setStretch( 2, 1 );
    srfld_ = new uiLineEdit( grp, FloatInpSpec(0.f,0.f), "Z Interval" );
    srfld_->editingFinished.notify( parchgcb );
    srfld_->setStretch( 2, 1 );
    srfld_->attach( rightOf, zstartfld_ );
    tbl_->setCellGroup( RowCol(mZRangeRow,mUseCol), grp );
}


void uiSEGYReadStartInfo::manSpecificLoadDefFields()
{
    if ( mHaveSpecRows )
    {
	mGetTypeVars;
	const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );

	if ( isvsp || !isrev0 )
	{
	    mRemoveFromTable( xcoordbytefld_, mXRow )
	    mRemoveFromTable( ycoordbytefld_, mYRow )
	    mRemoveFromTable( key2bytefld_, mKey2Row )
	}
	else if ( !xcoordbytefld_ )
	{
	    xcoordbytefld_ = new uiSEGYByteNr( 0, "X-coord byte" );
	    xcoordbytefld_->selectionChanged.notify( parchgcb );
	    mAddToTbl( xcoordbytefld_, mXRow );
	    ycoordbytefld_ = new uiSEGYByteNr( 0, "Y-coord byte" );
	    ycoordbytefld_->selectionChanged.notify( parchgcb );
	    mAddToTbl( ycoordbytefld_, mYRow );
	    key2bytefld_ = new uiSEGYByteNr( 0, "Key2 byte" );
	    key2bytefld_->selectionChanged.notify( parchgcb );
	    mAddToTbl( key2bytefld_, mKey2Row );
	}

	if ( isvsp || (!isrev0 && !is2d) )
	    mRemoveFromTable( key1bytefld_, mKey1Row )
	else if ( !key1bytefld_ )
	{
	    key1bytefld_ = new uiSEGYByteNr( 0, "Key1 byte" );
	    key1bytefld_->selectionChanged.notify( parchgcb );
	    mAddToTbl( key1bytefld_, mKey1Row );
	}

	if ( mHavePSRows )
	{
	    if ( isvsp || !isps )
		mRemoveFromTable( offsetbytefld_, mPSRow )
	    else if ( !offsetbytefld_ )
	    {
		offsetbytefld_ = new uiSEGYByteNr( 0, "Offset byte" );
		offsetbytefld_->selectionChanged.notify( parchgcb );
		mAddToTbl( offsetbytefld_, mPSRow );
	    }
	}
    }

    useLoadDef();
}


void uiSEGYReadStartInfo::setCellTxt( int col, int row, const char* txt )
{
    tbl_->setText( RowCol(row,col), txt );
    if ( col == mItemCol || col == mUseTxtCol )
	tbl_->resizeColumnToContents( col );
    if ( col == mQSResCol )
	tbl_->setColor( RowCol(row,mQSResCol), *txt ? qscellcolor
						    : Color::White() );
}


void uiSEGYReadStartInfo::revChg( CallBacker* )
{
    parChg( 0 );
}


void uiSEGYReadStartInfo::parChg( CallBacker* )
{
    if ( parsbeingset_ )
	return;

    fillLoadDef();
    showRelevantInfo();
    loaddefChanged.trigger();
}


void uiSEGYReadStartInfo::setImpTypIdx( int idx )
{
    if ( inptypfixed_ )
    {
	pErrMsg( "Input type fixed, cannot set" );
	return;
    }

    imptype_.tidx_ = idx;
    showRelevantInfo();
}


void uiSEGYReadStartInfo::showRelevantInfo()
{
    if ( !mHaveSpecRows )
	return;

    mGetTypeVars;

    const char* xittxt; const char* yittxt;
    const char* ky1ittxt; const char* ky2ittxt; const char* offsittxt;
    const char* xustxt; const char* yustxt;
    const char* ky1ustxt; const char* ky2ustxt; const char* offsustxt;
    xittxt = yittxt = ky1ittxt = ky2ittxt = offsittxt =
    xustxt = yustxt = ky1ustxt = ky2ustxt = offsustxt = "";
    const char* nrtrcsusrtxt = "";

    if ( isvsp )
	nrtrcsusrtxt = "(1 trace used)";
    else
    {
	xittxt = "X-Coordinate range"; yittxt = "Y-Coordinate range";
	ky1ittxt = is2d ? "Trace number range" : "Inline range";
	ky2ittxt = is2d ? "Ref/SP number range" : "Crossline range";
	if ( isps )
	    { offsittxt = "Offset range"; offsustxt = sBytePos; }

	if ( isrev0 )
	    xustxt = yustxt = ky1ustxt = ky2ustxt = sBytePos;
    }

    setCellTxt( mItemCol, mXRow, xittxt );
    setCellTxt( mItemCol, mYRow, yittxt );
    setCellTxt( mItemCol, mKey1Row, ky1ittxt );
    setCellTxt( mItemCol, mKey2Row, ky2ittxt );
    if ( mHavePSRows )
	setCellTxt( mItemCol, mPSRow, offsittxt );
    setCellTxt( mQSResCol, mKey1Row,
		isvsp ? "" : (is2d ? trcnrinfotxt_ : inlinfotxt_).buf() );
    setCellTxt( mQSResCol, mKey2Row,
		isvsp ? "" : (is2d ? refnrinfotxt_ : crlinfotxt_).buf() );
    setCellTxt( mQSResCol, mXRow, isvsp ? "" : xinfotxt_ );
    setCellTxt( mQSResCol, mYRow, isvsp ? "" : yinfotxt_ );
    setCellTxt( mQSResCol, mPSRow, isvsp || !isps ? "" : offsetinfotxt_.buf() );
    setCellTxt( mUseTxtCol, mNrSamplesRow, nrtrcsusrtxt );
    setCellTxt( mUseTxtCol, mKey1Row, ky1ustxt );
    setCellTxt( mUseTxtCol, mKey2Row, ky2ustxt );
    setCellTxt( mUseTxtCol, mXRow, xustxt );
    setCellTxt( mUseTxtCol, mYRow, yustxt );
    if ( mHavePSRows )
	setCellTxt( mUseTxtCol, mPSRow, offsustxt );

    manSpecificLoadDefFields();
}


void uiSEGYReadStartInfo::clearInfo()
{
    xinfotxt_.setEmpty(); yinfotxt_.setEmpty();
    inlinfotxt_.setEmpty(); crlinfotxt_.setEmpty();
    trcnrinfotxt_.setEmpty(); refnrinfotxt_.setEmpty();
    offsetinfotxt_.setEmpty();

    showRelevantInfo();

    for ( int irow=0; irow<mKey1Row; irow++ )
	setCellTxt( mQSResCol, irow, "" );
}


void uiSEGYReadStartInfo::setScanInfo( const SEGY::ScanInfo& si )
{
    tbl_->setColumnLabel( mQSResCol, si.fullscan_ ? "Full scan result"
						  : "Quick scan result" );
    clearInfo();
    if ( !si.isUsable() )
	return;

    BufferString txt;
    const SEGY::BasicFileInfo& bi = si.basicinfo_;

    txt.set( bi.revision_ );
    setCellTxt( mQSResCol, mRevRow, txt );

    const char** fmts = SEGY::FilePars::getFmts(false);
    txt.set( bi.format_ < 4 ? fmts[bi.format_-1]
	    : (bi.format_==8 ? fmts[4] : fmts[3]) );
    if ( bi.hdrsswapped_ && bi.dataswapped_ )
	txt.add( " (all bytes swapped)" );
    else if ( bi.hdrsswapped_ )
	txt.add( " (header bytes swapped)" );
    else if ( bi.dataswapped_ )
	txt.add( " (data bytes swapped)" );
    setCellTxt( mQSResCol, mDataFormatRow, txt );

    txt.set( bi.ns_ ).add( " (" ).add( si.nrtrcs_ )
	.add( si.nrtrcs_ == 1 ? " trace)" : " traces)" );
    setCellTxt( mQSResCol, mNrSamplesRow, txt );

    if ( mIsUdf(bi.sampling_.step) )
	txt.set( "" );
    else
    {
	const float endz = loaddef_.sampling_.start
			 + (bi.ns_-1) * loaddef_.sampling_.step;
	txt.set( loaddef_.sampling_.start ).add( " - " ).add( endz )
	    .add( " (s or " ).add( si.infeet_ ? "ft)" : "m)" );
    }
    setCellTxt( mQSResCol, mZRangeRow, txt );

    inlinfotxt_.set( si.inls_.start ).add( " - " ).add( si.inls_.stop );
    crlinfotxt_.set( si.crls_.start ).add( " - " ).add( si.crls_.stop );
    trcnrinfotxt_.set( si.trcnrs_.start ).add( " - " ).add( si.trcnrs_.stop );
    xinfotxt_.set( si.xrg_.start ).add( " - " ).add( si.xrg_.stop );
    yinfotxt_.set( si.yrg_.start ).add( " - " ).add( si.yrg_.stop );
    offsetinfotxt_.set( si.offsrg_.start ).add( " - " ).add( si.offsrg_.stop );
    if ( mIsUdf(si.refnrs_.start) )
	refnrinfotxt_.set( "<no data>" );
    else
	refnrinfotxt_.set(si.refnrs_.start).add(" - ").add(si.refnrs_.stop);

    useLoadDef();
    showRelevantInfo();
}


void uiSEGYReadStartInfo::useLoadDef()
{
    parsbeingset_ = true;

    revfld_->setCurrentItem( loaddef_.revision_ );

    const char** fmts = SEGY::FilePars::getFmts(false);
    const char* fmt = *fmts;
    for ( int idx=0; fmt; idx++ )
    {
	fmt = fmts[idx];
	if ( !fmt )
	    { pErrMsg("Format not found"); break; }
	else if ( (short)(*fmt - '0') == loaddef_.format_ )
	    { fmtfld_->setCurrentItem( idx ); break; }
    }

    nsfld_->setValue( loaddef_.ns_ );
    zstartfld_->setValue( loaddef_.sampling_.start );
    srfld_->setValue( loaddef_.sampling_.step );
    if ( imptype_.isVSP() )
	{ parsbeingset_ = false; return; }

#   define mSetToByteNr(fld,memb) \
    if ( fld ) fld->setByteNr( loaddef_.hdrdef_->memb.bytepos_ )

    mSetToByteNr( xcoordbytefld_, xcoord_  );
    mSetToByteNr( ycoordbytefld_, ycoord_  );
    mSetToByteNr( offsetbytefld_, offs_  );

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	mSetToByteNr( key1bytefld_, trnr_  );
	mSetToByteNr( key2bytefld_, refnr_  );
    }
    else
    {
	mSetToByteNr( key1bytefld_, inl_  );
	mSetToByteNr( key2bytefld_, crl_  );
    }

    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillLoadDef()
{
    loaddef_.revision_ = revfld_->currentItem();
    loaddef_.format_ = (short)(*fmtfld_->text() - '0');
    loaddef_.ns_ = nsfld_->getIntValue();
    loaddef_.sampling_.start = zstartfld_->getFValue();
    loaddef_.sampling_.step = srfld_->getFValue();

    if ( imptype_.isVSP() )
	return;

#   define mSetByteNr(fld,memb) \
    if ( fld ) loaddef_.hdrdef_->memb = fld->hdrEntry()

    mSetByteNr( xcoordbytefld_, xcoord_  );
    mSetByteNr( ycoordbytefld_, ycoord_  );
    mSetByteNr( offsetbytefld_, offs_  );

    if ( Seis::is2D(imptype_.geomType()) )
    {
	mSetByteNr( key1bytefld_, trnr_  );
	mSetByteNr( key2bytefld_, refnr_  );
    }
    else
    {
	mSetByteNr( key1bytefld_, inl_  );
	mSetByteNr( key2bytefld_, crl_  );
    }
}
