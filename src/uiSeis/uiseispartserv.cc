/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.95 2008-09-19 14:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "posinfo.h"
#include "survinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"

#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uisegysip.h"
#include "uiseiscbvsimp.h"
#include "uiseisiosimple.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseisrandto2dline.h"
#include "uiseissegyimpexp.h"
#include "uisegyio.h"
#include "uiseissel.h"
#include "uiseiswvltimp.h"
#include "uiseiswvltman.h"
#include "uiselsimple.h"
#include "uisurvey.h"
#include "uitaskrunner.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider( segyid_ );
    uiSurveyInfoEditor::addInfoProvider( sip );
    SeisIOObjInfo::initDefault( sKey::Steering );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    PtrMan<uiDialog> dlg = 0;
    if ( opt == 4 )
	dlg = new uiSeisImpCBVS( appserv().parent() );
    else if ( opt < 4 )
    {
	if ( GetEnvVarYN("OD_NEW_SEGY_HANDLING") )
	{
	    uiSEGYIO::Setup su( uiSEGYIO::Read, uiSEGYIO::ImpExp );
	    uiSEGYIO uisio( appserv().parent(), su );
	    return uisio.go();
	}
	else
	{
	    Seis::GeomType gt = opt == 0 ? Seis::Vol
			     : (opt == 1 ? Seis::Line
			     : (opt == 2 ? Seis::VolPS : Seis::LinePS));
	    if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	    dlg = new uiSeisSegYImpExp( appserv().parent(), forread, segyid_,
		    			gt );
	}
    }
    else
    {
	Seis::GeomType gt = opt == 5 ? Seis::Vol
	    		 : (opt == 6 ? Seis::Line
			 : (opt == 7 ? Seis::VolPS : Seis::LinePS));
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisIOSimple( appserv().parent(), gt, forread );
    }
    return dlg->go();
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


void uiSeisPartServer::manageSeismics()
{
    uiSeisFileMan dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    uiSeisWvltImp dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::manageWavelets()
{
    uiSeisWvltMan dlg( appserv().parent() );
    dlg.go();
}


bool uiSeisPartServer::select2DSeis( MultiID& mid, bool with_attr )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    uiSeisSel::Setup setup(Seis::Line); setup.selattr( with_attr );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DLineSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiSeisPartServer::get2DLineSetName( const MultiID& mid, 
					 BufferString& setname )
{
    mGet2DLineSet()
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getLineNames( linenames );

    uiSelectFromList::Setup setup( "Lines", linenames );
    uiSelectFromList dlg( appserv().parent(), setup );
    dlg.selFld()->setMultiSelect();
    dlg.selFld()->selectAll( true );
    if ( !dlg.go() )
	return false;

    dlg.selFld()->getSelectedItems( res );
    return res.size();
}


void uiSeisPartServer::get2DLineInfo( BufferStringSet& linesets,
				      TypeSet<MultiID>& setids,
				      TypeSet<BufferStringSet>& linenames )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    ObjectSet<IOObj> ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( !SeisTrcTranslator::is2D(ioobj,true)
	  || SeisTrcTranslator::isPS(ioobj) ) continue;

	uiSeisIOObjInfo oinf(ioobj,false);
	BufferStringSet lnms;
	oinf.getLineNames( lnms );
	linesets.add( ioobj.name() );
	setids += ioobj.key();
	linenames += lnms;
    }
}


bool uiSeisPartServer::get2DLineGeometry( const MultiID& mid,
					  const char* linenm,
					  PosInfo::Line2DData& geom )
{
    mGet2DLineSet(false)
    int lineidx = lineset.indexOf( linenm );
    if ( lineidx < 0 )
    {
	BufferStringSet attribs;
	get2DStoredAttribs( mid, linenm, attribs );
	if ( attribs.isEmpty() ) return false;

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	int maxnrtrcs = 0;
	for ( int idx=0; idx<attribs.size(); idx++ )
	{
	    int indx = lineset.indexOf( LineKey(linenm,attribs.get(idx)) );
	    if ( indx<0 || !lineset.getRanges(indx,trcrg,zrg) || !trcrg.step )
		continue;

	    const int nrtrcs = trcrg.nrSteps() + 1;
	    if ( nrtrcs > maxnrtrcs )
	    {
		maxnrtrcs = nrtrcs;
		lineidx = indx;
	    }
	}

	if ( lineidx < 0 ) return false;
    }

    return lineset.getGeometry( lineidx, geom );
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs )
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs );
}


void uiSeisPartServer::get2DStoredAttribsPartingDataType( const MultiID& mid,
						          const char* linenm,
					             BufferStringSet& attribs,
						     const char* datatypenm )
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs, true, datatypenm,
	   			   true, false );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)

    const int lidx = lineset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    lineset.getCubeSampling( cs, lidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lidx, buf );
    uiTaskRunner dlg( appserv().parent() );
    return dlg.execute( *exec );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj)
	  && SeisTrcTranslator::is2D(ioobj) != for3d )
	    nms.add( (const char*)ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( appserv().parent(), rln );
    dlg.go();
}
