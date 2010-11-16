/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjfileman.cc,v 1.35 2010-11-16 09:49:10 cvsbert Exp $";


#include "uiobjfileman.h"

#include "uitoolbutton.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uisplitter.h"
#include "uitextedit.h"

#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "streamconn.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "transl.h"


static const int cPrefHeight = 10;
static const int cPrefWidth = 75;


uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    const IOObjContext& ctxt )
    : uiDialog(p,s)
    , curioobj_(0)
    , ctxt_(*new IOObjContext(ctxt))
    , lastexternal_(0)
{
    setCtrlStyle( LeaveOnly );
}


uiObjFileMan::~uiObjFileMan()
{
    delete curioobj_;
    delete &ctxt_;
}


void uiObjFileMan::createDefaultUI( bool needreloc )
{
    listgrp_ = new uiGroup( this, "List Group" );
    IOM().to( 0 ); IOM().to( ctxt_.getSelKey() );
    selgrp_ = new uiIOObjSelGrp( listgrp_, CtxtIOObj(ctxt_), 0, false,
				 needreloc);
    selgrp_->selectionChg.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    mkdefbut_ = selgrp_->getManipGroup()->addButton( "makedefault.png",
	    mCB(this,uiObjFileMan,makeDefault), "Set as default" );

    infogrp_ = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    setPrefWidth( cPrefWidth );

    uiSplitter* sep = new uiSplitter( this, "List-Info splitter", false );
    sep->addGroup( listgrp_ );
    sep->addGroup( infogrp_ );
}


void uiObjFileMan::addTool( uiButton* but )
{
    if ( lastexternal_ )
	but->attach( rightOf, lastexternal_ );
    else
    {
	but->attach( ensureBelow, selgrp_ );
	infofld_->attach( ensureBelow, but );
    }

    lastexternal_ = but;
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    delete curioobj_;
    curioobj_ = selgrp_->nrSel() > 0 ? IOM().get(selgrp_->selected(0)) : 0;
    curimplexists_ = curioobj_ && curioobj_->implExists(true);
    mkdefbut_->setSensitive( curimplexists_ );

    ownSelChg();
    if ( curioobj_ )
	mkFileInfo();
    else
	setInfo( "" );

    BufferString msg;
    if ( curioobj_ )
	System::getFreeMBOnDiskMsg( System::getFreeMBOnDisk(*curioobj_), msg );
    toStatusBar( msg );
}


void uiObjFileMan::makeDefault( CallBacker* )
{
    if ( !curioobj_ ) return;
    SI().getPars().set( getDefKey(), curioobj_->key() );
    SI().savePars();
    selgrp_->fullUpdate( curioobj_->key() );
}


const char* uiObjFileMan::getDefKey() const
{
    static BufferString ret;
    ctxt_.fillTrGroup();
    ret = IOPar::compKey(sKey::Default,ctxt_.trgroup->userName());
    return ret.buf();
}


double uiObjFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    nrfiles = File::exists(filenm) ? 1 : 0;
    if ( nrfiles == 0 )
	return 0;

    double ret = (double)File::getKbSize( filenm );
    if ( ret < 0 ) ret = -ret;

    FilePath fp( filenm );
    fp.setExtension( "" );
    const BufferString dirnm = fp.fullPath();
    if ( !File::isDirectory(dirnm) )
	return ret;

    DirList dl( dirnm );
    nrfiles = 0;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	int extranrfiles = 0;
	const BufferString subfnm( dl.fullPath(idx) );
	ret += getFileSize( subfnm, extranrfiles );
	nrfiles += extranrfiles;
    }

    return ret;
}


BufferString uiObjFileMan::getFileSizeString( double filesz )
{
    BufferString szstr;
    if ( filesz > 1024 )
    {
        const bool doGb = filesz > 1048576;
	const int nr = doGb ? mNINT(filesz/10485.76) : mNINT(filesz/10.24);
	szstr = nr/100; 
	const int rest = nr%100; 
	szstr += rest < 10 ? ".0" : "."; szstr += rest;
	szstr += doGb ? " GB" : " MB";
    }
    else if ( filesz == 0 )
	szstr = "< 1 kB";
    else
    {
	szstr = filesz;
	szstr += " kB";
    }

    return szstr;
}


BufferString uiObjFileMan::getFileInfo()
{
    BufferString txt;
    if ( !curioobj_ )
	return txt;

    mDynamicCastGet(StreamConn*,conn,curioobj_->getConn(Conn::Read))
    if ( !conn )
	{ txt = "-"; setInfo( txt ); return txt; }

    BufferString fname( conn->fileName() );
    FilePath fp( fname );
    const bool isdir = File::isDirectory( fname );
    int nrfiles = 1;
    const double totsz = getFileSize( fname, nrfiles );

    txt += "Location: "; txt += fp.pathOnly();
    txt += isdir ? "\nDirectory name: " : "\nFile name: ";
    txt += fp.fileName();
    txt += "\nSize on disk: "; txt += getFileSizeString( totsz );
    if ( nrfiles > 1 )
	{ txt += "\nNumber of files: "; txt += nrfiles; }
    const char* timestr = File::timeLastModified( fname );
    if ( timestr ) { txt += "\nLast modified: "; txt += timestr; }
    int txtsz = txt.size()-1;
    if ( txt[ txtsz ] != '\n' ) txt += "\n";
    txt += "Object ID: "; txt += curioobj_->key(); txt += "\n";
    conn->close();
    delete conn;
    return txt;
}


void uiObjFileMan::setInfo( const char* txt )
{ infofld_->setText( txt ); }


void uiObjFileMan::setPrefWidth( int width )
{
    selgrp_->setPrefWidthInChar( width );
    infofld_->setPrefWidthInChar( width );
}
