/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellman.cc,v 1.40 2008-12-24 14:11:26 cvsbert Exp $";

#include "uiwellman.h"

#include "bufstringset.h"
#include "ioobj.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uigroup.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitextedit.h"
#include "uiwelldlgs.h"
#include "uiwellmarkerdlg.h"


uiWellMan::uiWellMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Well file management","Manage wells",
				     "107.1.0").nrstatusflds(1),
	           WellTranslatorGroup::ioContext() )
    , welldata(0)
    , wellrdr(0)
    , fname("")
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    uiGroup* logsgrp = new uiGroup( this, "Logs group" );
    uiLabel* lbl = new uiLabel( logsgrp, "Logs" );
    logsfld = new uiListBox( logsgrp, "Available logs", true );
    logsfld->attach( alignedBelow, lbl );

    uiPushButton* logsbut = new uiPushButton( logsgrp, "Add &Logs", false );
    logsbut->activated.notify( mCB(this,uiWellMan,addLogs) );
    logsbut->attach( centeredBelow, logsfld );
    logsgrp->attach( rightOf, selgrp );

    uiManipButGrp* butgrp = new uiManipButGrp( logsgrp );
    butgrp->addButton( uiManipButGrp::Rename, mCB(this,uiWellMan,renameLogPush),
	    	       "Rename selected log" );
    butgrp->addButton( uiManipButGrp::Remove, mCB(this,uiWellMan,removeLogPush),
	    	       "Remove selected log" );
    butgrp->addButton( "export.png", mCB(this,uiWellMan,exportLogs),
	    	       "Export log" );
    butgrp->attach( rightOf, logsfld );
    
    uiPushButton* markerbut = new uiPushButton( this, "&Markers", false);
    markerbut->activated.notify( mCB(this,uiWellMan,edMarkers) );
    markerbut->attach( ensureBelow, selgrp );
    markerbut->attach( ensureBelow, logsgrp );

    uiPushButton* d2tbut = 0;
    if ( SI().zIsTime() )
    {
	d2tbut = new uiPushButton( this, "&Depth/Time Model", false );
	d2tbut->activated.notify( mCB(this,uiWellMan,edD2T) );
	d2tbut->attach( rightOf, markerbut );
    }

    infofld->attach( ensureBelow, markerbut );
    selChg( this );
}


uiWellMan::~uiWellMan()
{
    delete welldata;
    delete wellrdr;
}


void uiWellMan::ownSelChg()
{
    getCurrentWell();
    fillLogsFld();
}


void uiWellMan::getCurrentWell()
{
    fname = ""; 
    delete wellrdr; wellrdr = 0;
    delete welldata; welldata = 0;
    if ( !curioobj_ ) return;
    
    mDynamicCastGet(const IOStream*,iostrm,curioobj_)
    if ( !iostrm ) return;
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    fname = sp.fileName();
    welldata = new Well::Data;
    wellrdr = new Well::Reader( fname, *welldata );
    wellrdr->getInfo();
}


void uiWellMan::fillLogsFld()
{
    logsfld->empty();
    if ( !wellrdr ) return;

    BufferStringSet lognms;
    wellrdr->getLogInfo( lognms );
    for ( int idx=0; idx<lognms.size(); idx++)
	logsfld->addItem( lognms.get(idx) );
    logsfld->selectAll( false );
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }


void uiWellMan::edMarkers( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
    {
	if ( welldata->markers().isEmpty() )
	    wellrdr->getMarkers();
	wd = welldata;
    }


    uiMarkerDlg dlg( this, wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( fname, *wd );
    if ( !wtr.putMarkers() )
	uiMSG().error( "Cannot write new markers to disk" );

    wd->markerschanged.trigger();
}


void uiWellMan::edD2T( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
	wd = welldata;

    if ( SI().zIsTime() && !wd->d2TModel() )
	wd->setD2TModel( new Well::D2TModel );


    uiD2TModelDlg dlg( this, *wd );
    if ( !dlg.go() ) return;
    Well::Writer wtr( fname, *wd );
    if ( !wtr.putD2T() )
	uiMSG().error( "Cannot write new model to disk" );
}


#define mDeleteLogs() \
    while ( welldata->logs().size() ) \
        delete welldata->logs().remove(0);

void uiWellMan::addLogs( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;

    wellrdr->getLogs();
    uiLoadLogsDlg dlg( this, *welldata );
    if ( !dlg.go() ) { mDeleteLogs(); return; }

    Well::Writer wtr( fname, *welldata );
    wtr.putLogs();
    fillLogsFld();
    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::exportLogs( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) return;

    BoolTypeSet issel;
    for ( int idx=0; idx<logsfld->size(); idx++ )
	issel += logsfld->isSelected(idx);

    if ( !welldata->d2TModel() )
	wellrdr->getD2T();

    wellrdr->getLogs();
    uiExportLogs dlg( this, *welldata, issel );
    dlg.go();

    mDeleteLogs();
}


void uiWellMan::removeLogPush( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) return;

    BufferString msg;
    msg = logsfld->nrSelected() == 1 ? "This log " : "These logs ";
    msg += "will be removed from disk.\nDo you wish to continue?";
    if ( !uiMSG().askGoOn(msg) )
	return;

    wellrdr->getLogs();
    BufferStringSet logs2rem;
    for ( int idx=0; idx<logsfld->size(); idx++ )
    {
	if ( logsfld->isSelected(idx) )
	    logs2rem.add( welldata->logs().getLog(idx).name() );
    }

    for ( int idx=0; idx<logs2rem.size(); idx++ )
    {
	BufferString& logname = logs2rem.get( idx );
	for ( int logidx=0; logidx<welldata->logs().size(); logidx++ )
	{
	    if ( logname == welldata->logs().getLog(logidx).name() )
	    {
		Well::Log* log = welldata->logs().remove( logidx );
		delete log;
		break;
	    }
	}
    }

    logs2rem.deepErase();

    if ( wellrdr->removeAll(Well::IO::sExtLog) )
    {
	Well::Writer wtr( fname, *welldata );
	wtr.putLogs();
	fillLogsFld();
    }

    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::renameLogPush( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) mErrRet("No log selected")

    const int lognr = logsfld->currentItem() + 1;
    FilePath fp( fname ); fp.setExtension( 0 );
    BufferString logfnm = Well::IO::mkFileName( fp.fullPath(),
	    					Well::IO::sExtLog, lognr );
    StreamProvider sp( logfnm );
    StreamData sdi = sp.makeIStream();
    bool res = wellrdr->addLog( *sdi.istrm );
    sdi.close();
    if ( !res ) 
	mErrRet("Cannot read selected log")

    Well::Log* log = welldata->logs().remove( 0 );

    BufferString titl( "Rename '" );
    titl += log->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
    			new StringInpSpec(log->name()) );
    if ( !dlg.go() ) return;

    BufferString newnm = dlg.text();
    if ( logsfld->isPresent(newnm) )
	mErrRet("Name already in use")

    log->setName( newnm );
    Well::Writer wtr( fname, *welldata );
    StreamData sdo = sp.makeOStream();
    wtr.putLog( *sdo.ostrm, *log );
    sdo.close();
    fillLogsFld();
    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );
    delete log;
}


void uiWellMan::mkFileInfo()
{
    if ( !welldata || !wellrdr || !curioobj_ )
    {
	infofld->setText( "" );
	return;
    }

    BufferString txt;

#define mAddWellInfo(key,str) \
    if ( str.size() ) \
    { txt += key; txt += ": "; txt += str; txt += "\n"; }

    Well::Info& info = welldata->info();
    BufferString crdstr; info.surfacecoord.fill( crdstr.buf() );
    BufferString bidstr; SI().transform(info.surfacecoord).fill( bidstr.buf() );
    BufferString posstr( bidstr ); posstr += " "; posstr += crdstr;
    const BufferString elevstr( toString(info.surfaceelev) );
    mAddWellInfo(Well::Info::sKeycoord,posstr)
    mAddWellInfo(Well::Info::sKeyelev,elevstr)
    mAddWellInfo(Well::Info::sKeyuwid,info.uwid)
    mAddWellInfo(Well::Info::sKeyoper,info.oper)
    mAddWellInfo(Well::Info::sKeystate,info.state)
    mAddWellInfo(Well::Info::sKeycounty,info.county)

    txt += getFileInfo();
    infofld->setText( txt );
}


static bool addSize( const char* basefnm, const char* fnmend,
		     double& totalsz, int& nrfiles )
{
    BufferString fnm( basefnm ); fnm += fnmend;
    if ( !File_exists(fnm) ) return false;

    totalsz += (double)File_getKbSize( fnm );
    nrfiles++;
    return true;
}


double uiWellMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File_isEmpty(filenm) ) return -1;

    double totalsz = (double)File_getKbSize( filenm );
    nrfiles = 1;

    FilePath fp( filenm ); fp.setExtension( 0 );
    const BufferString basefnm( fp.fullPath() );

    addSize( basefnm, Well::IO::sExtMarkers, totalsz, nrfiles );
    addSize( basefnm, Well::IO::sExtD2T, totalsz, nrfiles );

    for ( int idx=1; ; idx++ )
    {
	BufferString fnmend( "^" ); fnmend += idx;
	fnmend += Well::IO::sExtLog;
	if ( !addSize(basefnm,fnmend,totalsz,nrfiles) )
	    break;
    }

    return totalsz;
}
