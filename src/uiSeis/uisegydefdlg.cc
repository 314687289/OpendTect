/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegydefdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
#include "uisegyexamine.h"
#include "uitoolbar.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiseparator.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uitable.h"
#include "uimsg.h"
#include "filepath.h"
#include "keystrs.h"
#include "segydirectdef.h"
#include "segytr.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "od_helpids.h"

#define sKeySettNrTrcExamine \
    IOPar::compKey("SEG-Y",uiSEGYExamine::Setup::sKeyNrTrcs)


uiSEGYDefDlg::Setup::Setup()
    : uiDialog::Setup("SEG-Y tool","Specify basic properties",
                      mODHelpKey(mSEGYDefDlgHelpID) )
    , defgeom_(Seis::Vol)
{
}


uiSEGYDefDlg::uiSEGYDefDlg( uiParent* p, const uiSEGYDefDlg::Setup& su,
			  IOPar& iop )
    : uiVarWizardDlg(p,su,iop,Start)
    , setup_(su)
    , geomfld_(0)
    , geomtype_(Seis::Vol)
    , readParsReq(this)
    , writeParsReq(this)
{
    const bool havevol = su.geoms_.isPresent( Seis::Vol );
    const bool havevolps = su.geoms_.isPresent( Seis::VolPS );
    const bool havevlineps = su.geoms_.isPresent( Seis::LinePS );
    uiSEGYFileSpec::Setup sgyfssu( havevol || havevolps || havevlineps );
    sgyfssu.forread(true).pars(&iop);
    sgyfssu.canbe3d( havevol || havevolps );
    filespecfld_ = new uiSEGYFileSpec( this, sgyfssu );
    filespecfld_->fileSelected.notify( mCB(this,uiSEGYDefDlg,fileSel) );

    uiGroup* lastgrp = filespecfld_;
    if ( su.geoms_.size() == 1 )
    {
	geomtype_ = su.geoms_[0];
    }
    else
    {
	if ( su.geoms_.isEmpty() )
	    uiSEGYRead::Setup::getDefaultTypes( setup_.geoms_ );
	if ( !setup_.geoms_.isPresent( setup_.defgeom_ ) )
	    setup_.defgeom_ = setup_.geoms_[0];

	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "File type" );
	geomfld_ = lcb->box();
	for ( int idx=0; idx<su.geoms_.size(); idx++ )
	    geomfld_->addItem( Seis::nameOf( (Seis::GeomType)su.geoms_[idx] ) );
	geomfld_->setCurrentItem( setup_.geoms_.indexOf(setup_.defgeom_) );
	geomfld_->selectionChanged.notify( mCB(this,uiSEGYDefDlg,geomChg) );
	lcb->attach( alignedBelow, filespecfld_ );
	lastgrp = lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "hor sep", OD::Horizontal, false);
    sep->attach( stretchedBelow, lastgrp );

    int nrex = 100; Settings::common().get( sKeySettNrTrcExamine, nrex );
    nrtrcexfld_ = new uiGenInput( this, "Number of traces to examine",
			      IntInpSpec(nrex).setName("Traces to Examine") );
    nrtrcexfld_->attach( alignedBelow, lastgrp );
    nrtrcexfld_->attach( ensureBelow, sep );
    savenrtrcsbox_ = new uiCheckBox( this, sSaveAsDefault() );
    savenrtrcsbox_->attach( rightOf, nrtrcexfld_ );
    fileparsfld_ = new uiSEGYFilePars( this, true, &iop );
    fileparsfld_->attach( alignedBelow, nrtrcexfld_ );
    fileparsfld_->readParsReq.notify( mCB(this,uiSEGYDefDlg,readParsCB) );
    fileparsfld_->writeParsReq.notify( mCB(this,uiSEGYDefDlg,writeParsCB) );

    postFinalise().notify( mCB(this,uiSEGYDefDlg,initFlds) );
	// Need this to get zero padding right
}


void uiSEGYDefDlg::initFlds( CallBacker* )
{
    usePar( pars_ );
    geomChg( 0 );
}


Seis::GeomType uiSEGYDefDlg::geomType() const
{
    if ( !geomfld_ )
	return geomtype_;

    return Seis::geomTypeOf( geomfld_->textOfItem( geomfld_->currentItem() ) );
}


int uiSEGYDefDlg::nrTrcExamine() const
{
    const int nr = nrtrcexfld_->getIntValue();
    return nr < 0 || mIsUdf(nr) ? 0 : nr;
}


void uiSEGYDefDlg::use( const IOObj* ioobj, bool force )
{
    filespecfld_->use( ioobj, force );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
    {
	if ( geomfld_ )
	{
	    geomfld_->setCurrentItem( Seis::nameOf(oinf.geomType()) );
	    geomChg( 0 );
	}
	fileparsfld_->usePar( ioobj->pars() );
	useSpecificPars( ioobj->pars() );
    }
}


void uiSEGYDefDlg::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    filespecfld_->fillPar( iop );
    fileparsfld_->fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, nrTrcExamine() );
    iop.set( sKey::Geometry(), Seis::nameOf(geomType()) );
}


void uiSEGYDefDlg::usePar( const IOPar& iop )
{
    if ( &iop != &pars_ )
    {
	SEGY::FileReadOpts::shallowClear( pars_ );
	pars_.merge( iop );
    }
    filespecfld_->usePar( pars_ );
    fileparsfld_->usePar( pars_ );
    useSpecificPars( iop );
}


void uiSEGYDefDlg::useSpecificPars( const IOPar& iop )
{
    int nrex = nrTrcExamine();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrex );
    nrtrcexfld_->setValue( nrex );
    const char* res = iop.find( sKey::Geometry() );
    if ( res && *res && geomfld_ )
    {
	geomfld_->setCurrentItem( res );
	geomChg( 0 );
    }
}


void uiSEGYDefDlg::fileSel( CallBacker* )
{
    const bool allswpd = filespecfld_->isProbablySwapped();
    const bool dataswpd = filespecfld_->isProbablySeisWare()
			&& filespecfld_->isIEEEFmt();
    fileparsfld_->setBytesSwapped( allswpd, dataswpd );
}


void uiSEGYDefDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYDefDlg::writeParsCB( CallBacker* )
{
    writeParsReq.trigger();
}


void uiSEGYDefDlg::geomChg( CallBacker* )
{
    filespecfld_->setInp2D( Seis::is2D(geomType()) );
}


bool uiSEGYDefDlg::acceptOK( CallBacker* )
{
    if ( savenrtrcsbox_->isChecked() )
    {
	const int nrex = nrTrcExamine();
	Settings::common().set( sKeySettNrTrcExamine, nrex );
	Settings::common().write();
    }

    IOPar tmp;
    if ( !filespecfld_->fillPar(tmp) || !fileparsfld_->fillPar(tmp) )
	return false;

    fillPar( pars_ );
    return true;
}


static const char* getFileNameKey( int index )
{
    BufferString filekey( "File ", index );
    return IOPar::compKey( filekey, sKey::FileName() );
}


#define mErrLabelRet(s) { lbl = new uiLabel( this, s ); return; }
uiEditSEGYFileDataDlg::uiEditSEGYFileDataDlg( uiParent* p, const IOObj& obj )
    : uiDialog(p,uiDialog::Setup(tr("SEGYDirect File Editor"),obj.name(),
				 mTODOHelpKey))
    , dirsel_(0),filetable_(0)
    , ioobj_(obj)
    , filepars_(*new IOPar)
{
    const BufferString deffnm = obj.fullUserExpr( true );
    uiLabel* lbl = 0;
    if ( !SEGY::DirectDef::readFooter(deffnm,filepars_,fileparsoffset_) )
	mErrLabelRet(tr("Cannot read SEGY file info for %1").arg(obj.name()));

    FilePath fp = filepars_.find( getFileNameKey(0) );
    if ( fp.isEmpty() )
	mErrLabelRet(tr("No SEGY Files linked to %1").arg(obj.name()));

    uiString olddirtxt( tr("Old location of SEGY files:  %1")
			.arg(fp.pathOnly()) );
    lbl = new uiLabel( this, olddirtxt );

    dirsel_ = new uiFileInput( this, "New location", fp.pathOnly() );
    dirsel_->setSelectMode( uiFileDialog::Directory );
    dirsel_->valuechanged.notify( mCB(this,uiEditSEGYFileDataDlg,dirSelCB) );
    dirsel_->attach( leftAlignedBelow, lbl );

    fillFileTable();
    filetable_->attach( stretchedBelow, dirsel_ );
}


void uiEditSEGYFileDataDlg::fillFileTable()
{
    int nrfiles = 0;
    if ( !filepars_.get("Number of files",nrfiles) || !nrfiles )
	return;

    uiTable::Setup su( nrfiles, 3 );
    filetable_ = new uiTable( this, su, "FileTable" );
    filetable_->setColumnLabel( 0, "Old File Name" );
    filetable_->setColumnLabel( 1, "New File Name" );
    filetable_->setColumnLabel( 2, " " );
    filetable_->setColumnReadOnly( 0, true );
    filetable_->valueChanged.notify( mCB(this,uiEditSEGYFileDataDlg,editCB) );

    FilePath oldfp = filepars_.find( getFileNameKey(0) );
    const BufferString olddir = oldfp.pathOnly();
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FilePath fp = filepars_.find( getFileNameKey(idx) );
	const BufferString oldfilename = fp.fileName();
	filetable_->setText( RowCol(idx,0),
			fp.pathOnly() == olddir ? oldfilename : fp.fullPath() );
	uiButton* selbut = new uiPushButton( 0, uiStrings::sSelect(),
			       mCB(this,uiEditSEGYFileDataDlg,fileSelCB), true);
	filetable_->setCellObject( RowCol(idx,2), selbut );
    }

    updateFileTable( -1 );
}


void uiEditSEGYFileDataDlg::updateFileTable( int rowidx )
{
    int nrfiles = 0;
    if ( !filepars_.get("Number of files",nrfiles) || !nrfiles )
	return;

    const BufferString seldir = dirsel_->fileName();
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	if ( rowidx >= 0 && rowidx != idx )
	    continue;

	FilePath oldfp = filepars_.find( getFileNameKey(idx) );
	const BufferString oldfnm = oldfp.fileName();
	RowCol rc( idx, 1 );
	BufferString newfnm = filetable_->text( rc );
	if ( newfnm.isEmpty() )
	{
	    newfnm = oldfnm;
	    filetable_->setText( RowCol(idx,1), newfnm );
	}

	FilePath fp( seldir, newfnm );
	if ( File::exists(fp.fullPath()) )
	{
	    filetable_->setCellToolTip( rc, 0 );
	    filetable_->setColor( rc, Color::White() );
	}
	else
	{
	    BufferString tttext( "File ", fp.fullPath(), " does not exist" );
	    filetable_->setCellToolTip( rc, tttext );
	    filetable_->setColor( rc, Color::Red() );
	}
    }
}


void uiEditSEGYFileDataDlg::editCB( CallBacker* )
{
    if ( filetable_->currentCol() == 0 )
	return;

    updateFileTable( filetable_->currentRow() );
}


void uiEditSEGYFileDataDlg::fileSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiObject*,uiobj,cb)
    if ( !uiobj ) return;

    const int rowidx = filetable_->getCell( uiobj ).row();
    if ( rowidx < 0 ) return;

    BufferString newfnm = filetable_->text( RowCol(rowidx,1) );
    FilePath fp( dirsel_->fileName(), newfnm );
    const bool selexists = File::exists( fp.fullPath() );
    uiFileDialog dlg( this, true, selexists ? fp.fullPath() : 0,
		      uiSEGYFileSpec::fileFilter() );
    if ( !selexists )
	dlg.setDirectory( fp.pathOnly() );
    if ( !dlg.go() )
	return;

    FilePath newfp( dlg.fileName() );
    if ( newfp.pathOnly() != fp.pathOnly() )
    {
	uiMSG().error( tr("Please select a file from %1").arg(fp.pathOnly()) );
	return;
    }

    filetable_->setText( RowCol(rowidx,1), newfp.fileName() );
    updateFileTable( rowidx );
}


void uiEditSEGYFileDataDlg::dirSelCB( CallBacker* )
{
    updateFileTable( -1 );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }
bool uiEditSEGYFileDataDlg::acceptOK( CallBacker* )
{
    int nrfiles = 0;
    if ( !filepars_.get("Number of files",nrfiles) || !nrfiles )
	mErrRet(tr("No SEGY Files were found linked to %1").arg(ioobj_.name()))

    const BufferString seldir = dirsel_->fileName();
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	BufferString newfnm = filetable_->text( RowCol(idx,1) );
	if ( newfnm.isEmpty() )
	    mErrRet( tr("New file name cannot be empty") );

	FilePath fp( seldir, newfnm );
	if ( !File::exists(fp.fullPath()) )
	    mErrRet( tr("File %1 does not exist").arg(fp.fullPath()) )

	filepars_.set( getFileNameKey(idx), fp.fullPath() );
    }

    return SEGY::DirectDef::updateFooter( ioobj_.fullUserExpr(true), filepars_,
					  fileparsoffset_ );
}


