/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.31 2008-05-05 05:42:29 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiimppickset.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipickpartserv.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "filegen.h"
#include "pickset.h"
#include "picksettr.h"

#include <math.h>

static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    "Horizon",
    0
};


uiImpExpPickSet::uiImpExpPickSet( uiPickPartServer* p, bool imp )
    : uiDialog(p->parent(),uiDialog::Setup(imp ? "Import Pickset"
			: "Export PickSet", "Specify pickset parameters",
			imp ? "105.0.1" : "105.0.2"))
    , serv_(p)
    , ctio_(*mMkCtxtIOObj(PickSet))
    , import_(imp)
    , fd_(*PickSetAscIO::getDesc(true))
    , zfld_(0)
    , constzfld_(0)
    , dataselfld_(0)
{
    BufferString label( import_ ? "Input " : "Output " );
    label += "Ascii file";
    filefld_ = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import_)
					    .forread(import_) );
    filefld_->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Loc) );

    ctio_.ctxt.forread = !import_;
    ctio_.ctxt.maychdir = false;
    label = import_ ? "Output " : "Input "; label += "PickSet";
    objfld_ = new uiIOObjSel( this, ctio_, label );

    if ( import_ )
    {
	zfld_ = new uiLabeledComboBox( this, "Get Z values from" );
	zfld_->box()->addItems( zoptions );
	zfld_->box()->selectionChanged.notify( mCB(this,uiImpExpPickSet,
		    		formatSel) );
	zfld_->attach( alignedBelow, filefld_ );

	BufferString constzlbl = "Specify Constatnt Z value";
	constzlbl += SI().getZUnit();
	constzfld_ = new uiGenInput( this, constzlbl, FloatInpSpec(0) );
	constzfld_->attach( alignedBelow, zfld_ );
	constzfld_->display( zfld_->box()->currentItem() == 1 );

	horinpfld_ = new uiLabeledComboBox( this, "Select Horizon" );
	serv_->fetchHors( false );
	const ObjectSet<SurfaceInfo> hinfos = serv_->horInfos();
	for ( int idx=0; idx<hinfos.size(); idx++ )
	    horinpfld_->box()->addItem( hinfos[idx]->name );
	horinpfld_->attach( alignedBelow, zfld_ );
	horinpfld_->display( zfld_->box()->currentItem() == 2 );

	uiSeparator* sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, constzfld_ );

	dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
	dataselfld_->attach( alignedBelow, constzfld_ );
	dataselfld_->attach( ensureBelow, sep );

	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, dataselfld_ );

	objfld_->attach( alignedBelow, constzfld_ );
	objfld_->attach( ensureBelow, sep );

	colorfld_ = new uiColorInput( this,
	       		           uiColorInput::Setup(getRandStdDrawColor()).
	       			   lbltxt("Color") );
	colorfld_->attach( alignedBelow, objfld_ );

	polyfld_ = new uiCheckBox( this, "Import as Polygon" );
	polyfld_->attach( rightTo, colorfld_ );
    }
    else
	filefld_->attach( alignedBelow, objfld_ );
}


uiImpExpPickSet::~uiImpExpPickSet()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImpExpPickSet::formatSel( CallBacker* cb )
{
    const int zchoice = zfld_->box()->currentItem(); 
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    horinpfld_->display( zchoice == 2 );
    PickSetAscIO::updateDesc( fd_, iszreq );
    dataselfld_->updateSummary();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImpExpPickSet::doImport()
{
    const char* fname = filefld_->fileName();
    StreamData sdi = StreamProvider( fname ).makeIStream();
    if ( !sdi.usable() ) 
    { 
	sdi.close();
	mErrRet( "Could not open input file" )
    }

    const char* psnm = objfld_->getInput();
    Pick::Set ps( psnm );
    const int zchoice = zfld_->box()->currentItem();
    float constz = zchoice==1 ? constzfld_->getfValue() : 0;
    if ( SI().zIsTime() ) constz /= 1000;

    ps.disp_.color_ = colorfld_->color();
    PickSetAscIO aio( fd_ );
    aio.get( *sdi.istrm, ps, zchoice==0, constz );
    sdi.close();

    if ( zchoice == 2 )
    {
	serv_->fillZValsFrmHor( &ps, horinpfld_->box()->currentItem() );
    }

    const bool ispolygon = polyfld_->isChecked();
    if ( ispolygon )
    {
	ps.disp_.connect_ = Pick::Set::Disp::Close;
	ctio_.ioobj->pars().set( sKey::Type, sKey::Polygon );
	IOM().commitChanges( *ctio_.ioobj );
    }
    else
	ps.disp_.connect_ = Pick::Set::Disp::None;

    BufferString errmsg;
    if ( !PickSetTranslator::store(ps,ctio_.ioobj,errmsg) )
	mErrRet(errmsg);

    return true;
}


bool uiImpExpPickSet::doExport()
{
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,ctio_.ioobj,errmsg) )
	mErrRet(errmsg);

    const char* fname = filefld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() ) 
    { 
	sdo.close();
	mErrRet( "Could not open output file" )
    }

    BufferString buf;
    for ( int locidx=0; locidx<ps.size(); locidx++ )
    {
	ps[locidx].toString( buf );
	*sdo.ostrm << buf.buf() << '\n';
    }

    *sdo.ostrm << '\n';
    sdo.close();
    return true;
}


bool uiImpExpPickSet::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;
    bool ret = import_ ? doImport() : doExport();
    return ret;
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( import_ && !File_exists(filenm) )
	mErrRet( "Please select input file" );

    if ( !import_ && filenm.isEmpty() )
	mErrRet( "Please select output file" );

    if ( !objfld_->commitInput( true ) )
	mErrRet( "Please select PickSet" );

    if ( !dataselfld_->commit() )
	mErrRet( "Please specify data format" );

    const int zchoice = zfld_->box()->currentItem();
    if ( zchoice == 1 )
    {
	float constz = constzfld_->getfValue();
	if ( SI().zIsTime() ) constz /= 1000;

	if ( !SI().zRange(false).includes( constz ) )
	    mErrRet( "Please Enter a valid Z value" );
    }

    return true;
}
