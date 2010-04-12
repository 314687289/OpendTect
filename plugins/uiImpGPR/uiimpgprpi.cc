/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uiimpgprpi.cc,v 1.11 2010-04-12 15:02:06 cvsbert Exp $";

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uiseissel.h"
#include "uitaskrunner.h"
#include "dztimporter.h"
#include "survinfo.h"
#include "strmprov.h"
#include "filepath.h"
#include "plugins.h"


static const char* menunm = "&GPR: DZT ...";


mExternC int GetuiImpGPRPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiImpGPRPluginInfo()
{
    static PluginInfo retpi = {
	"GPR: .DZT import",
	"Bert",
	"0.0.2",
	"Imports GPR data in DZT format."
	"\nThanks to Matthias Schuh (m.schuh@neckargeo.net) for information,"
	"\ntest data and comments." };
    return &retpi;
}


class uiImpGPRMgr :  public CallBacker
{
public:

			uiImpGPRMgr(uiODMain&);

    uiODMain&		appl_;
    void		updMnu(CallBacker*);
    void		doWork(CallBacker*);
};


uiImpGPRMgr::uiImpGPRMgr( uiODMain& a )
	: appl_(a)
{
    appl_.menuMgr().dTectMnuChanged.notify( mCB(this,uiImpGPRMgr,updMnu) );
    updMnu(0);
}

void uiImpGPRMgr::updMnu( CallBacker* )
{
    if ( SI().has2D() )
	appl_.menuMgr().getMnu( true, uiODApplMgr::Seis )->
		insertItem(new uiMenuItem(menunm,mCB(this,uiImpGPRMgr,doWork)));
}


class uiDZTImporter : public uiDialog
{
public:

uiDZTImporter( uiParent* p )
    : uiDialog(p,Setup("Import GPR Seismics","Import DZT Seismics","103.0.16"))
    , inpfld_(0)
{
    if ( !SI().has2D() )
	{ new uiLabel(this,"TODO: implement 3D loading"); }

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( "*.dzt" ).forread( true );
    inpfld_ = new uiFileInput( this, "Input DZT file", fisu );
    inpfld_->valuechanged.notify( mCB(this,uiDZTImporter,inpSel) );

    nrdeffld_ = new uiGenInput( this, "Trace number definition: start, step",
				IntInpSpec(1), IntInpSpec(1) );
    nrdeffld_->attach( alignedBelow, inpfld_ );
    startposfld_ = new uiGenInput( this, "Start position (X,Y)",
	    			PositionInpSpec(SI().minCoord(true)) );
    startposfld_->attach( alignedBelow, nrdeffld_ );
    stepposfld_ = new uiGenInput( this, "Step in X/Y", FloatInpSpec(),
	    				FloatInpSpec(0) );
    stepposfld_->attach( alignedBelow, startposfld_ );

    zfacfld_ = new uiGenInput( this, "Z Factor", FloatInpSpec(1) );
    zfacfld_->attach( alignedBelow, stepposfld_ );

    lnmfld_ = new uiGenInput( this, "Output Line name" );
    lnmfld_->attach( alignedBelow, zfacfld_ );

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
	    		     uiSeisSel::Setup(Seis::Line) );
    outfld_->attach( alignedBelow, lnmfld_ );
}

void inpSel( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) return;

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() ) return;

    DZT::FileHeader fh; BufferString emsg;
    if ( !fh.getFrom(*sd.istrm,emsg) ) return;

    FilePath fp( fnm ); fp.setExtension( "", true );
    lnmfld_->setText( fp.fileName() );

    const float tdist = fh.spm ? 1. / ((float)fh.spm) : SI().inlDistance();
    stepposfld_->setValue( tdist, 0 );

}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    if ( !inpfld_ ) return true;

    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) mErrRet("Please enter the input file name")
    const BufferString lnm( lnmfld_->text() );
    if ( lnm.isEmpty() ) mErrRet("Please enter the output line name")

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj ) return false;

    DZT::Importer importer( fnm, *ioobj, LineKey(lnm,outfld_->attrNm()) );
    importer.fh_.nrdef_.start = nrdeffld_->getIntValue(0);
    importer.fh_.nrdef_.step = nrdeffld_->getIntValue(0);
    importer.fh_.cstart_ = startposfld_->getCoord();
    importer.fh_.cstep_.x = stepposfld_->getfValue(0);
    importer.fh_.cstep_.y = stepposfld_->getfValue(1);
    importer.zfac_ = zfacfld_->getfValue();

    uiTaskRunner tr( this );
    return tr.execute( importer );
}

    uiFileInput*	inpfld_;
    uiGenInput*		lnmfld_;
    uiGenInput*		nrdeffld_;
    uiGenInput*		startposfld_;
    uiGenInput*		stepposfld_;
    uiGenInput*		zfacfld_;
    uiSeisSel*		outfld_;

};


void uiImpGPRMgr::doWork( CallBacker* )
{
    uiDZTImporter dlg( &appl_ );
    dlg.go();
}


mExternC const char* InituiImpGPRPlugin( int, char** )
{
    (void)new uiImpGPRMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
