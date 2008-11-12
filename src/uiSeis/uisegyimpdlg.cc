/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.cc,v 1.12 2008-11-12 15:06:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "uisegydef.h"
#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseparator.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "segyhdr.h"
#include "seisioobjinfo.h"
#include "seisimporter.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "filegen.h"
#include "dirlist.h"
#include "ioman.h"
#include "iostrm.h"



uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop )
    : uiSEGYReadDlg(p,su,iop)
    , morebut_(0)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_))
{
    ctio_.ctxt.forread = false;
    if ( setup().dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Import " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += fs.fname_; ttl += "'";
	setTitleText( ttl );
    }

    uiSeparator* sep = optsgrp_ ? new uiSeparator( this, "Hor sep" ) : 0;

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    transffld_ = new uiSeisTransfer( outgrp, uiSeisTransfer::Setup(setup_.geom_)
				    .withnullfill(false)
				    .fornewentry(true) );
    outgrp->setHAlignObj( transffld_ );
    if ( sep )
    {
	sep->attach( stretchedBelow, optsgrp_ );
	outgrp->attach( alignedBelow, optsgrp_ );
	outgrp->attach( ensureBelow, sep );
    }

    seissel_ = new uiSeisSel( outgrp, ctio_, uiSeisSel::Setup(setup_.geom_) );
    seissel_->attach( alignedBelow, transffld_ );

    if ( setup_.geom_ == Seis::Line )
    {
	morebut_ = new uiCheckBox( outgrp, "Import more, similar files" );
	morebut_->attach( alignedBelow, seissel_ );
    }
}


uiSEGYImpDlg::~uiSEGYImpDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSEGYImpDlg::use( const IOObj* ioobj, bool force )
{
    uiSEGYReadDlg::use( ioobj, force );
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


class uiSEGYImpSimilarDlg : public uiDialog
{
public:

uiSEGYImpSimilarDlg( uiSEGYImpDlg* p, const IOObj& iio, const IOObj& oio,
		     const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-import",
		    		     "Specify file details","103.0.6"))
	, inioobj_(iio)
	, outioobj_(oio)
	, impdlg_(p)
	, attrnm_(anm)
{
    const BufferString fnm( inioobj_.fullUserExpr(true) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Imp "); setupnm += uiSEGYFileSpec::sKeyLineNmToken;

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken );
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Input ('" ); txt += uiSEGYFileSpec::sKeyLineNmToken;
    txt += "' will become line name)";
    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(true) );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File_isDirectory(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken; msg += "'\n";
	msg += "That will then become the line name";
	uiMSG().error( msg );
	return false;
    }

    IOM().to( outioobj_.key() );
    return doImp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = inioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    bool res = impdlg_->impFile( *newioobj, outioobj_, lnm, attrnm_ );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askGoOn("Continue with next?") )
	    return false;
    }
    return true;
}


bool doImp( const FilePath& fp )
{
    BufferString mask( fp.fileName() );
    replaceString( mask.buf(), uiSEGYFileSpec::sKeyLineNmToken, "*" );
    FilePath maskfp( fp ); maskfp.setFileName( mask );
    const int nrtok = countCharacter( mask.buf(), '*' );
    DirList dl( fp.pathOnly(), DirList::FilesOnly, mask );
    if ( dl.size() < 1 )
    {
	uiMSG().error( "Cannot find any match for file name" );
	return false;
    }

    BufferString fullmaskfnm( maskfp.fullPath() );
    int lnmoffs = strstr( fullmaskfnm.buf(), "*" ) - fullmaskfnm.buf();
    const int orglen = fullmaskfnm.size();
    bool nofails = true;

    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirlistfnm( dl.get(idx) );
	FilePath newfp( maskfp );
	newfp.setFileName( dirlistfnm );
	const BufferString fullfnm( newfp.fullPath() );
	const int newlen = fullfnm.size();
	const int lnmlen = (newlen - orglen + 1) / nrtok;
	BufferString lnm( fullfnm.buf() + lnmoffs );
	*(lnm.buf() + lnmlen) = '\0';

	IOObj* newioobj = getSubstIOObj( fullfnm );
	if ( !doWork( newioobj, lnm, idx > dl.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiSEGYImpDlg*	impdlg_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;
    const char*		attrnm_;

};



bool uiSEGYImpDlg::doWork( const IOObj& inioobj )
{
    if ( !seissel_->commitInput(true) )
    {
	uiMSG().error( "Please select the output data" );
	return false;
    }

    const IOObj& outioobj = *ctio_.ioobj;
    const bool is2d = Seis::is2D( setup_.geom_ );
    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D() ?
		      transffld_->selFld2D()->selectedLine() : 0;

    if ( !morebut_ || !morebut_->isChecked() )
	return impFile( inioobj, outioobj, lnm, attrnm );

    uiSEGYImpSimilarDlg dlg( this, inioobj, outioobj, attrnm );
    return dlg.go();
}


bool uiSEGYImpDlg::impFile( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool isps = Seis::isPS( setup_.geom_ );
    const bool is2d = Seis::is2D( setup_.geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo;
    if ( !isps )
    {
	ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
	if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	    return false;
    }

    SEGY::TxtHeader::info2d = is2d;
    transffld_->scfmtfld->updateIOObj( const_cast<IOObj*>(&outioobj), true );
    PtrMan<SeisTrcWriter> wrr = new SeisTrcWriter( &outioobj );
    SeisStdImporterReader* rdr = new SeisStdImporterReader( inioobj, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->scfmtfld->getScaler() );
    Seis::SelData* sd = transffld_->getSelData();
    if ( is2d )
    {
	if ( linenm && *linenm )
	    sd->lineKey().setLineName( linenm );
	if ( !isps )
	    sd->lineKey().setAttrName( attrnm );
	wrr->setSelData( sd->clone() );
    }
    rdr->setSelData( sd );

    PtrMan<SeisImporter> imp = new SeisImporter( rdr, *wrr, setup_.geom_ );
    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( imp->name() );
	nm += " ("; nm += linenm; nm += ")";
	imp->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *imp );
    BufferStringSet warns;
    if ( imp && imp->nrSkipped() > 0 )
	warns += new BufferString("During import, ", imp->nrSkipped(),
				  " traces were rejected" );
    SeisTrcTranslator* tr = rdr->reader().seisTranslator();
    if ( tr && tr->haveWarnings() )
	warns.add( tr->warnings(), false );
    imp.erase(); wrr.erase(); // closes output cube

    displayWarnings( warns );
    if ( rv && !is2d && ioobjinfo )
	rv = ioobjinfo->provideUserInfo();

    SEGY::TxtHeader::info2d = false;
    return rv;
}
