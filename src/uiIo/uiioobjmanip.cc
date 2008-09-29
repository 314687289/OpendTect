/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjmanip.cc,v 1.33 2008-09-29 13:23:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjmanip.h"
#include "iodirentry.h"
#include "uiioobj.h"
#include "uifiledlg.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"
#include "ptrman.h"
#include "ioman.h"
#include "iodir.h"
#include "iostrm.h"
#include "iopar.h"
#include "transl.h"
#include "ptrman.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "errh.h"

uiManipButGrp::ButData::ButData( uiToolButton* b, const ioPixmap& p,
				 const char* t )
	: but(b)
    	, pm(new ioPixmap(p))
    	, tt(t)
{
}


uiManipButGrp::ButData::~ButData()
{
    delete pm;
}


uiToolButton* uiManipButGrp::addButton( Type tp, const CallBack& cb,
					const char* tooltip )
{
    PtrMan<ioPixmap> pm = 0;
    switch ( tp )
    {
	case FileLocation:
	    pm = new ioPixmap( "filelocation.png" ); break;
	case Rename:
	    pm = new ioPixmap( "renameobj.png" ); break;
	case Remove:
	    pm = new ioPixmap( "trashcan.png" ); break;
	case ReadOnly:
	    pm = new ioPixmap( "readonly.png" ); break;
	default:
	    pErrMsg("Unknown toolbut typ");
	    pm = new ioPixmap( "home.png" ); break;
    }

    return addButton( *pm, cb, tooltip );
}


uiToolButton* uiManipButGrp::addButton( const char* pmnm, const CallBack& cb,
					const char* tooltip )
{
    return addButton( ioPixmap(pmnm), cb, tooltip );
}


uiToolButton* uiManipButGrp::addButton( const ioPixmap& pm, const CallBack& cb,
					const char* tooltip )
{
    uiToolButton* button = new uiToolButton( this, tooltip, pm, cb );
    button->setToolTip( tooltip );
    butdata += new ButData( button, pm, tooltip );
    altbutdata += 0;
    return button;
}


void uiManipButGrp::setAlternative( uiToolButton* button, const ioPixmap& pm,
				    const char* tt )
{
    for ( int idx=0; idx<butdata.size(); idx++ )
    {
	if ( butdata[idx]->but == button )
	{
	    uiManipButGrp::ButData* bd = altbutdata[idx];
	    if ( !bd )
		altbutdata.replace( idx,
				    new uiManipButGrp::ButData(button,pm,tt) );
	    else
	    {
		bd->but = button;
		delete bd->pm; bd->pm = new ioPixmap(pm);
		bd->tt = tt;
	    }
	}
    }
}
				    

void uiManipButGrp::useAlternative( uiToolButton* button, bool yn )
{
    for ( int idx=0; idx<butdata.size(); idx++ )
    {
	uiManipButGrp::ButData* normbd = butdata[idx];
	if ( normbd->but == button )
	{
	    uiManipButGrp::ButData* altbd = altbutdata[idx];
	    if ( yn && !altbd ) return;
	    uiManipButGrp::ButData& bd = yn ? *altbd : *normbd;
	    button->setPixmap( *bd.pm );
	    button->setToolTip( bd.tt );
	    break;
	}
    }
}


uiIOObjManipGroup::uiIOObjManipGroup( uiIOObjManipGroupSubj& s )
    	: uiManipButGrp(s.obj_->parent())
	, subj_(s)
{
    subj_.grp_ = this;

    const CallBack cb( mCB(this,uiIOObjManipGroup,tbPush) );
    locbut = addButton( FileLocation, cb, "Change location on disk" );
    renbut = addButton( Rename, cb, "Rename this object" );
    robut = addButton( ReadOnly, cb, "Toggle read-only" );
    rembut = addButton( Remove, cb, "Remove this object" );
    robut->setToggleButton( true );
    attach( rightOf, subj_.obj_ );
}


uiIOObjManipGroup::~uiIOObjManipGroup()
{
}


IOObj* uiIOObjManipGroup::gtIOObj() const
{
    const MultiID* curid = subj_.curID();
    return curid ? IOM().get( *curid ) : 0;
}


void uiIOObjManipGroup::commitChgs( IOObj* ioobj )
{
    if ( ioobj ) IOM().commitChanges( *ioobj );
}


void uiIOObjManipGroup::triggerButton( uiManipButGrp::Type tp )
{
    if ( tp == FileLocation )	locbut->click();
    else if ( tp == Rename )	renbut->click();
    else if ( tp == ReadOnly )	robut->click();
    else if ( tp == Remove )	rembut->click();
}


void uiIOObjManipGroup::selChg()
{
    PtrMan<IOObj> ioobj = gtIOObj();
    if ( !ioobj ) return;
    mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())

    const bool isexisting = ioobj && ioobj->implExists(true);
    const bool isreadonly = isexisting && ioobj->implReadOnly();
    locbut->setSensitive( iostrm && !isreadonly );
    robut->setOn( isreadonly );
    renbut->setSensitive( ioobj );
    rembut->setSensitive( ioobj && !isreadonly );
}


void uiIOObjManipGroup::tbPush( CallBacker* c )
{
    PtrMan<IOObj> ioobj = gtIOObj();
    if ( !ioobj ) return;
    mDynamicCastGet(uiToolButton*,tb,c)
    if ( !tb ) { pErrMsg("CallBacker is not uiToolButton!"); return; }

    MultiID prevkey( ioobj->key() );
    PtrMan<Translator> tr = ioobj->getTranslator();

    bool chgd = false;
    if ( tb == locbut )
	chgd = relocEntry( ioobj, tr );
    else if ( tb == robut )
	chgd = readonlyEntry( ioobj, tr );
    else if ( tb == renbut )
	chgd = renameEntry( ioobj, tr );
    else if ( tb == rembut )
    {
	bool exists = tr ? tr->implExists(ioobj,true) : ioobj->implExists(true);
	bool readonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
	if ( exists && readonly )
	{
	    uiMSG().error( "Entry is not writable.\nPlease change this first.");
	    return; 
	}
	chgd = rmEntry( ioobj, exists );
    }

    if ( chgd )
	subj_.chgsOccurred();
}


bool uiIOObjManipGroup::renameEntry( IOObj* ioobj, Translator* tr )
{
    BufferString titl( "Rename '" );
    titl += ioobj->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
	    		new StringInpSpec(ioobj->name()) );
    if ( !dlg.go() ) return false;

    BufferString newnm = dlg.text();
    if ( subj_.names().indexOf(newnm) >= 0 )
    {
	if ( newnm != ioobj->name() )
	    uiMSG().error( "Name already in use" );
	return false;
    }
    else
    {
	IOObj* lioobj = IOM().getLocal( newnm );
	if ( lioobj )
	{
	    BufferString msg( "This name is already used by a " );
	    msg += lioobj->translator();
	    msg += " object";
	    delete lioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    ioobj->setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
    {
	if ( !iostrm->implExists(true) )
	    iostrm->genDefaultImpl();
	else
	{
	    IOStream chiostrm;
	    chiostrm.copyFrom( iostrm );
	    FilePath fp( iostrm->fileName() );
	    if ( tr )
		chiostrm.setExt( tr->defExtension() );

	    BufferString cleannm( chiostrm.name() );
	    cleanupString( cleannm.buf(), mC_True, mC_False, mC_True );
	    chiostrm.setName( cleannm );
	    chiostrm.genDefaultImpl();
	    chiostrm.setName( newnm );

	    FilePath deffp( chiostrm.fileName() );
	    fp.setFileName( deffp.fileName() );
	    chiostrm.setFileName( fp.fullPath() );

	    const bool newfnm = strcmp(chiostrm.fileName(),iostrm->fileName());
	    if ( newfnm && !doReloc(tr,*iostrm,chiostrm) )
	    {
		if ( strchr(newnm.buf(),'/') || strchr(newnm.buf(),'\\') )
		{
		    cleanupString(newnm.buf(),mC_False,mC_False,mC_True);
		    chiostrm.setName( newnm );
		    chiostrm.genDefaultImpl();
		    deffp.set( chiostrm.fileName() );
		    fp.setFileName( deffp.fileName() );
		    chiostrm.setFileName( fp.fullPath() );
		    chiostrm.setName( iostrm->name() );
		    if ( !doReloc(tr,*iostrm,chiostrm) )
			return false;
		}
	    }

	    iostrm->copyFrom( &chiostrm );
	}
    }

    commitChgs( ioobj );
    return true;
}


bool uiIOObjManipGroup::rmEntry( IOObj* ioobj, bool rmabl )
{
    return rmabl ? uiIOObj(*ioobj).removeImpl( true )
		 : IOM().permRemove( ioobj->key() );
}


bool uiIOObjManipGroup::relocEntry( IOObj* ioobj, Translator* tr )
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    BufferString caption( "New file location for '" );
    caption += ioobj->name(); caption += "'";
    BufferString oldfnm( iostrm->getExpandedName(true) );
    BufferString filefilt;
    BufferString defext( subj_.defExt() );
    if ( !defext.isEmpty() )
    {
	filefilt += "OpendTect Files (*."; filefilt += defext;
	filefilt += ");;";
    }
    filefilt += "All Files(*)";

    uiFileDialog dlg( this, uiFileDialog::Directory, oldfnm, filefilt, caption);
    if ( !dlg.go() ) return false;

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    const char* newdir = dlg.fileName();
    if ( !File_isDirectory(newdir) )
    { uiMSG().error( "Selected path is not a directory" ); return false; }

    FilePath fp( oldfnm ); fp.setPath( newdir );
    chiostrm.setFileName( fp.fullPath() );
    if ( !doReloc(tr,*iostrm,chiostrm) )
	return false;

    commitChgs( ioobj );
    return true;
}


bool uiIOObjManipGroup::readonlyEntry( IOObj* ioobj, Translator* tr )
{
    if ( !ioobj ) { pErrMsg("Huh"); return false; }

    bool exists = tr ? tr->implExists(ioobj,true) : ioobj->implExists(true);
    if ( !exists ) return false;

    bool oldreadonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
    bool newreadonly = robut->isOn();
    if ( oldreadonly == newreadonly ) return false;

    bool res = tr ? tr->implSetReadOnly(ioobj,newreadonly)
		: ioobj->implSetReadOnly(newreadonly);

    newreadonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
    if ( oldreadonly == newreadonly )
	uiMSG().warning( "Could not change the read-only status" );

    selChg();
    return false;
}


bool uiIOObjManipGroup::doReloc( Translator* tr, IOStream& iostrm,
				 IOStream& chiostrm )
{
    const bool oldimplexist = tr ? tr->implExists(&iostrm,true)
				 : iostrm.implExists(true);
    BufferString newfname( chiostrm.getExpandedName(true) );

    bool succeeded = true;
    if ( oldimplexist )
    {
	const bool newimplexist = tr ? tr->implExists(&chiostrm,true)
				     : chiostrm.implExists(true);
	if ( newimplexist && !uiIOObj(chiostrm).removeImpl(false) )
	    return false;

	CallBack cb( mCB(this,uiIOObjManipGroup,relocCB) );
	succeeded = tr  ? tr->implRename( &iostrm, newfname, &cb )
			: iostrm.implRename( newfname, &cb );
    }

    if ( succeeded )
	iostrm.setFileName( newfname );
    else
	uiMSG().error( "Relocation failed" );
    return succeeded;
}


void uiIOObjManipGroup::relocCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    subj_.relocStart( msg );
}
