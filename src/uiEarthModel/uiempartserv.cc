/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/

#include "uiempartserv.h"

#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
#include "datacoldef.h"
#include "datainpspec.h"
#include "datapointset.h"
#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizonpreload.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emobjectio.h"
#include "empolygonbody.h"
#include "emposid.h"
#include "emrandomposbody.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceio.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "dbdir.h"
#include "dbman.h"
#include "ioobj.h"
#include "parametricsurface.h"
#include "pickset.h"
#include "posinfo2d.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "undo.h"
#include "variogramcomputers.h"
#include "varlenarray.h"

#include "uiarray2dchg.h"
#include "uiarray2dinterpol.h"
#include "uibulkfaultimp.h"
#include "uibulkhorizonimp.h"
#include "uichangesurfacedlg.h"
#include "uicreatehorizon.h"
#include "uidlggroup.h"
#include "uiempreloaddlg.h"
#include "uiexpfault.h"
#include "uiexphorizon.h"
#include "uiexport2dhorizon.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uihorgeom2attr.h"
#include "uihorinterpol.h"
#include "uihorsavefieldgrp.h"
#include "uihor3dfrom2ddlg.h"
#include "uiimpfault.h"
#include "uiimphorizon.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultisurfaceread.h"
#include "uirandlinegen.h"
#include "uiselsimple.h"
#include "uisurfaceman.h"
#include "uitaskrunner.h"
#include "uivariogram.h"

#include <math.h>

static const char* sKeyPreLoad()		{ return "PreLoad"; }

int uiEMPartServer::evDisplayHorizon()		{ return 0; }
int uiEMPartServer::evRemoveTreeObject()	{ return 1; }


uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , em_(EM::EMM())
    , disponcreation_(false)
    , selectedrg_(false)
    , imphorattrdlg_(0)
    , imphorgeomdlg_(0)
    , impbulkhordlg_(0)
    , impfltdlg_(0)
    , impbulkfltdlg_(0)
    , impfss2ddlg_(0)
    , exphordlg_(0)
    , expfltdlg_(0)
    , expfltstickdlg_(0)
    , impfltstickdlg_(0)
    , crhordlg_(0)
    , man3dhordlg_(0)
    , man2dhordlg_(0)
    , ma3dfaultdlg_(0)
    , manfssdlg_(0)
    , manbodydlg_(0)
{
    DBM().surveyChanged.notify( mCB(this,uiEMPartServer,survChangedCB) );
}


uiEMPartServer::~uiEMPartServer()
{
    em_.setEmpty();
    deepErase( variodlgs_ );
    delete man3dhordlg_;
    delete man2dhordlg_;
    delete ma3dfaultdlg_;
    delete manfssdlg_;
    delete manbodydlg_;
    delete crhordlg_;
}


void uiEMPartServer::survChangedCB( CallBacker* )
{
    delete imphorattrdlg_; imphorattrdlg_ = 0;
    delete imphorgeomdlg_; imphorgeomdlg_ = 0;
    delete impfltdlg_; impfltdlg_ = 0;
    delete impbulkfltdlg_; impbulkfltdlg_ = 0;
    delete impbulkhordlg_; impbulkhordlg_ = 0;
    delete exphordlg_; exphordlg_ = 0;
    delete expfltdlg_; expfltdlg_ = 0;
    delete man3dhordlg_; man3dhordlg_ = 0;
    delete man2dhordlg_; man2dhordlg_ = 0;
    delete ma3dfaultdlg_; ma3dfaultdlg_ = 0;
    delete manfssdlg_; manfssdlg_ = 0;
    delete manbodydlg_; manbodydlg_ = 0;
    delete crhordlg_; crhordlg_ = 0;
    deepErase( variodlgs_ );
}


void uiEMPartServer::manageSurfaces( const char* typstr )
{
    uiSurfaceMan dlg( parent(), uiSurfaceMan::TypeDef().parse(typstr) );
    dlg.go();
}


#define mManSurfaceDlg( dlgobj, surfacetype ) \
    delete dlgobj; \
    dlgobj = new uiSurfaceMan( parent(), uiSurfaceMan::surfacetype ); \
    dlgobj->go();

void uiEMPartServer::manage3DHorizons()
{
    mManSurfaceDlg(man3dhordlg_,Hor3D);
}


void uiEMPartServer::manage2DHorizons()
{
    mManSurfaceDlg(man2dhordlg_,Hor2D);
}


void uiEMPartServer::manage3DFaults()
{
    mManSurfaceDlg(ma3dfaultdlg_,Flt3D);
}


void uiEMPartServer::manageFaultStickSets()
{
    mManSurfaceDlg(manfssdlg_,StickSet);
}


void uiEMPartServer::manageBodies()
{
    mManSurfaceDlg(manbodydlg_,Body);
}


bool uiEMPartServer::import3DHorAttr()
{
    if ( imphorattrdlg_ )
	imphorattrdlg_->raise();
    else
    {
	imphorattrdlg_ = new uiImportHorizon( parent(), false );
	imphorattrdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB));
    }

    return imphorattrdlg_->go();
}


bool uiEMPartServer::import3DHorGeom( bool bulk )
{
    if ( bulk )
    {
	if ( !impbulkhordlg_ )
	    impbulkhordlg_ = new uiBulkHorizonImport( parent() );

	return impbulkhordlg_->go();
    }

    if ( imphorgeomdlg_ )
	imphorgeomdlg_->raise();
    else
    {
	imphorgeomdlg_ = new uiImportHorizon( parent(), true );
	imphorgeomdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB));
    }

    return imphorgeomdlg_->go();
}


void uiEMPartServer::importReadyCB( CallBacker* cb )
{
    mDynamicCastGet(uiImportHorizon*,dlg,cb)
    if ( dlg && dlg->doDisplay() )
	selemid_ = dlg->getSelID();

    mDynamicCastGet(uiImportFault*,fltdlg,cb)
    if ( fltdlg && fltdlg->saveButtonChecked() )
	selemid_ = fltdlg->getSelID();

    mDynamicCastGet(uiCreateHorizon*,crdlg,cb)
    if ( crdlg && crdlg->saveButtonChecked() )
	selemid_ = crdlg->getSelID();

    if ( selemid_.isInvalid() )
	return;

    EM::EMObject* emobj = em_.getObject( selemid_ );
    if ( emobj ) emobj->setFullyLoaded( true );

    sendEvent( evDisplayHorizon() );
}


bool uiEMPartServer::export2DHorizon()
{
    ObjectSet<SurfaceInfo> hinfos;
    getAllSurfaceInfo( hinfos, true );
    uiExport2DHorizon dlg( parent(), hinfos );
    return dlg.go();
}


bool uiEMPartServer::export3DHorizon()
{
    if ( exphordlg_ )
	exphordlg_->raise();
    else
	exphordlg_ = new uiExportHorizon( parent() );

    return exphordlg_->go();
}


bool uiEMPartServer::importFault( bool bulk )
{
    if ( bulk )
    {
	if ( !impbulkfltdlg_ )
	    impbulkfltdlg_ = new uiBulkFaultImport( parent() );

	return impbulkfltdlg_->go();
    }

    if ( impfltdlg_ )
	impfltdlg_->raise();
    else
    {
	impfltdlg_ =
	    new uiImportFault3D( parent(),
				 EMFault3DTranslatorGroup::sGroupName());
	impfltdlg_->importReady.notify( mCB(this,uiEMPartServer,importReadyCB));
    }

    return impfltdlg_->go();
}


bool uiEMPartServer::importFaultStickSet()
{
    if ( impfltstickdlg_ )
	impfltstickdlg_->raise();
    else
    {
	impfltstickdlg_ =
	    new uiImportFault3D( parent(),
				 EMFaultStickSetTranslatorGroup::sGroupName() );
	impfltstickdlg_->importReady.notify(
				mCB(this,uiEMPartServer,importReadyCB));
    }

    return impfltstickdlg_->go();
}


void uiEMPartServer::import2DFaultStickset()
{
    if ( impfss2ddlg_ )
    {
	impfss2ddlg_->show();
	impfss2ddlg_->raise();
	return;
    }

    impfss2ddlg_ = new uiImportFaultStickSet2D( parent(),
				EMFaultStickSetTranslatorGroup::sGroupName() );
    impfss2ddlg_->importReady.notify( mCB(this,uiEMPartServer,importReadyCB) );
    impfss2ddlg_->show();
}


bool uiEMPartServer::exportFault()
{
    if ( expfltdlg_ )
	expfltdlg_->raise();
    else
	expfltdlg_ = new uiExportFault( parent(),
					EMFault3DTranslatorGroup::sGroupName());
    return expfltdlg_->go();
}


bool uiEMPartServer::exportFaultStickSet()
{
    if ( expfltstickdlg_ )
	expfltstickdlg_->raise();
    else
	expfltstickdlg_ =
	    new uiExportFault( parent(),
			       EMFaultStickSetTranslatorGroup::sGroupName() );
    return expfltstickdlg_->go();
}


void uiEMPartServer::createHorWithConstZ( bool is2d )
{
    if ( !crhordlg_ )
    {
	crhordlg_ = new uiCreateHorizon( parent(), is2d );
	crhordlg_->ready.notify( mCB(this,uiEMPartServer,importReadyCB) );
    }

    crhordlg_->show();
}


bool uiEMPartServer::isChanged( const DBKey& emid ) const
{
    const EM::EMObject* emobj = em_.getObject( emid );
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isGeometryChanged( const DBKey& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,em_.getObject(emid));
    return emsurf && emsurf->geometry().isChanged(0);
}


int uiEMPartServer::nrAttributes( const DBKey& emid ) const
{
    EM::EMObject* emobj = em_.getObject( emid );
    if ( !emobj ) return 0;

    EM::IOObjInfo eminfo( emobj->dbKey() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    return eminfo.isOK() ? attrnms.size() : 0;
}


bool uiEMPartServer::isEmpty( const DBKey& emid ) const
{
    EM::EMObject* emobj = em_.getObject( emid );
    return emobj && emobj->isEmpty();
}


bool uiEMPartServer::isFullResolution( const DBKey& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,em_.getObject(emid));
    return emsurf && emsurf->geometry().isFullResolution();
}


bool uiEMPartServer::isFullyLoaded( const DBKey& emid ) const
{
    const EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isFullyLoaded();
}


void uiEMPartServer::displayEMObject( const DBKey& mid )
{
    selemid_ = mid;

    if ( selemid_.isInvalid() )
    {
	loadSurface( mid );
	selemid_ = mid;
    }

    if ( !selemid_.isInvalid() )
    {
	mDynamicCastGet( EM::Horizon3D*,hor3d,em_.getObject( selemid_ ) )
        if ( hor3d )
        {
	    TrcKeySampling selrg;
	    selrg.setInlRange( hor3d->geometry().rowRange() );
	    selrg.setCrlRange( hor3d->geometry().colRange( 0 ) );
	    setHorizon3DDisplayRange( selrg );
        }
	sendEvent( evDisplayHorizon() );
    }
}


void uiEMPartServer::displayOnCreateCB( CallBacker* cb )
{
    mDynamicCastGet(uiHorizonInterpolDlg*,interpoldlg,cb);
    mDynamicCastGet(uiFilterHorizonDlg*,filterdlg,cb);
    if ( !interpoldlg && !filterdlg )
	return;

    uiHorSaveFieldGrp* horfldsavegrp = interpoldlg ? interpoldlg->saveFldGrp()
						   : filterdlg->saveFldGrp();
    if ( horfldsavegrp->displayNewHorizon() )
	displayEMObject( horfldsavegrp->getNewHorizon()->dbKey() );

    horfldsavegrp->overwriteHorizon();
}


bool uiEMPartServer::fillHoles( const DBKey& emid, bool is2d )
{
    mDynamicCastGet(EM::Horizon*,hor,em_.getObject(emid));
    uiHorizonInterpolDlg dlg( parent(), hor, is2d );
    dlg.saveFldGrp()->allowOverWrite( false );
    dlg.horReadyForDisplay.notify( mCB(this,uiEMPartServer,displayOnCreateCB) );
    return dlg.go();
}


bool uiEMPartServer::filterSurface( const DBKey& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid))
    uiFilterHorizonDlg dlg( parent(), hor3d );
    dlg.saveFldGrp()->allowOverWrite( false );
    dlg.horReadyForDisplay.notify( mCB(this,uiEMPartServer,displayOnCreateCB) );
    return dlg.go();
}


void uiEMPartServer::removeTreeObject( const DBKey& emid )
{
    selemid_ = emid;
    sendEvent( evRemoveTreeObject() );
}


EM::EMObject* uiEMPartServer::selEMObject()
{ return em_.getObject( selemid_ ); }


void uiEMPartServer::deriveHor3DFrom2D( const DBKey& emid )
{
    mDynamicCastGet(EM::Horizon2D*,h2d,em_.getObject(emid))
    uiHor3DFrom2DDlg dlg( parent(), *h2d, this );

    if ( dlg.go() && dlg.doDisplay() )
    {
	RefMan<EM::Horizon3D> hor = dlg.getHor3D();
	selemid_ = hor->id();
	sendEvent( evDisplayHorizon() );
    }
}


bool uiEMPartServer::askUserToSave( const DBKey& emid,
				    bool withcancel ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    if ( !emobj || !emobj->isChanged() || !EM::canOverwrite(emobj->dbKey()) )
	return true;

    PtrMan<IOObj> ioobj = DBM().get( emid );
    if ( !ioobj && emobj->isEmpty() )
	return true;

    uiString msg = tr( "%1 '%2' has changed.\n\nDo you want to save it?" )
		   .arg( emobj->getTypeStr() ).arg( emobj->name() );

    const int ret = uiMSG().askSave( msg, withcancel );
    if ( ret == 1 )
    {
	const bool saveas = !ioobj || !isFullyLoaded(emid);
	return storeObject( emid, saveas );
    }

    return ret == 0;
}


void uiEMPartServer::selectHorizons( ObjectSet<EM::EMObject>& objs, bool is2d )
{
    selectSurfaces( objs, is2d ? EMHorizon2DTranslatorGroup::sGroupName()
			      : EMHorizon3DTranslatorGroup::sGroupName() );
}


void uiEMPartServer::selectFaults( ObjectSet<EM::EMObject>& objs, bool is2d )
{
    if ( !is2d )
	selectSurfaces( objs, EMFault3DTranslatorGroup::sGroupName() );
}


void uiEMPartServer::selectFaultStickSets( ObjectSet<EM::EMObject>& objs )
{  selectSurfaces( objs, EMFaultStickSetTranslatorGroup::sGroupName() ); }


void uiEMPartServer::selectBodies( ObjectSet<EM::EMObject>& objs )
{
    CtxtIOObj ctio( EMBodyTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;

    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg dlg( parent(), sdsu, ctio );
    if ( !dlg.go() )
	return;

    DBKeySet mids;
    dlg.getChosen( mids );
    if ( mids.isEmpty() )
	return;

    ExecutorGroup loaders( "Loading Bodies" );
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( mids[idx] );
	if ( !ioobj )
	    continue;

	const BufferString& translator = ioobj->translator();
	if ( translator!=EMBodyTranslatorGroup::sKeyUserWord() )
	    continue;

	BufferString typestr;
	ioobj->pars().get( sKey::Type(), typestr );
	EM::EMObject* object = EM::BodyMan().createTempObject( typestr );
	if ( !object ) continue;

	object->ref();
	object->setDBKey( mids[idx] );
	objs += object;
	loaders.add( object->loader() );
    }

    uiTaskRunner execdlg( parent() );
    if ( !TaskRunner::execute( &execdlg, loaders ) )
    {
	deepUnRef( objs );
	return;
    }
}


void uiEMPartServer::selectSurfaces( ObjectSet<EM::EMObject>& objs,
				     const char* typ )
{
    uiMultiSurfaceReadDlg dlg( parent(), typ );
    if ( !dlg.go() ) return;

    DBKeySet surfaceids;
    dlg.iogrp()->getSurfaceIds( surfaceids );

    PtrMan<EM::ObjectLoader> emloader =
			  EM::ObjectLoader::factory().create( typ, surfaceids );
    if ( !emloader )
	return;

    uiTaskRunner uitr( parent() );
    if ( emloader->load(&uitr) )
	objs.append( emloader->getLoadedEMObjects() );
}


void uiEMPartServer::setHorizon3DDisplayRange( const TrcKeySampling& hs )
{ selectedrg_ = hs; }


bool uiEMPartServer::loadAuxData( const DBKey& id,
			    const TypeSet<int>& selattribs, bool removeold )
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    if ( removeold )
	hor3d->auxdata.removeAll();

    bool retval = false;
    for ( int idx=0; idx<selattribs.size(); idx++ )
    {
	Executor* executor = hor3d->auxdata.auxDataLoader( selattribs[idx] );
	uiTaskRunner runer( parent() );
	if ( runer.execute( *executor) )
	    retval = true;
    }

    return retval;

}



int uiEMPartServer::loadAuxData( const DBKey& id, const char* attrnm,
				 bool removeold )
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return -1;

    EM::IOObjInfo eminfo( id );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const int nritems = attrnms.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = attrnms.get(idx);
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx<0 ) return -1;
    TypeSet<int> selattribs( 1, selidx );
    return loadAuxData( id, selattribs, removeold )
	? hor3d->auxdata.auxDataIndex(attrnm) : -1;
}


bool uiEMPartServer::loadAuxData( const DBKey& id,
			const BufferStringSet& selattrnms, bool removeold )
{
    EM::IOObjInfo eminfo( id );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    TypeSet<int> idxs;
    for ( int idx=0; idx<selattrnms.size(); idx++ )
    {
	const int selidx = attrnms.indexOf( selattrnms.get(idx) );
	if ( selidx < 0 ) continue;

	idxs += idx;
    }

    return loadAuxData( id, idxs, removeold );
}


bool uiEMPartServer::storeFaultAuxData( const DBKey& id,
	BufferString& auxdatanm, const Array2D<float>& data )
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Fault3D*, flt3d, object );
    if ( !flt3d )
	return false;

    EM::FaultAuxData* auxdata = flt3d->auxData();
    if ( !auxdata )
	return false;

    BufferStringSet atrrnms;
    auxdata->getAuxDataList( atrrnms );
    uiGetObjectName::Setup setup( toUiString("%1 %2").arg(uiStrings::sFault())
					    .arg(uiStrings::sData()), atrrnms );
    setup.inptxt_ = uiStrings::sAttribute(mPlural);
    uiGetObjectName dlg( parent(), setup );
    if ( !dlg.go() )
	return false;

    auxdatanm = dlg.text();
    if ( atrrnms.indexOf(auxdatanm.buf())>=0 )
    {
	uiString msg = tr("The name '%1' already exists, "
	                  "do you really want to overwrite it?")
                     .arg(auxdatanm);
	if ( !uiMSG().askGoOn( msg ) )
	    return false;
    }

    const int sdidx = auxdata->setData( auxdatanm.buf(), &data, OD::UsePtr );
    if ( !auxdata->storeData(sdidx,false) )
    {
	uiMSG().error( auxdata->errMsg() );
	return false;
    }

    return true;
}


bool uiEMPartServer::showLoadFaultAuxDataDlg( const DBKey& id )
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Fault3D*, flt3d, object );
    if ( !flt3d )
	return false;

    EM::FaultAuxData* auxdata = flt3d->auxData();
    if ( !auxdata )
	return false;

    BufferStringSet atrrnms;
    auxdata->getAuxDataList( atrrnms );
    uiSelectFromList::Setup setup( tr("Fault Data"), atrrnms );
    setup.dlgtitle( tr("Select one attribute to be displayed") );
    uiSelectFromList dlg( parent(), setup );
    if ( !dlg.go() || !dlg.selFld() )
	return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getChosen( selattribs );
    auxdata->setSelected( selattribs );

    return true;
}


bool uiEMPartServer::showLoadAuxDataDlg( const DBKey& id )
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    EM::IOObjInfo eminfo( id );
    BufferStringSet atrrnms;
    eminfo.getAttribNames( atrrnms );
    uiSelectFromList::Setup setup( tr("Horizon Data"), atrrnms );
    setup.dlgtitle( tr("Select one or more attributes to be displayed\n"
		       "on the horizon. After loading, use 'Page Up'\n"
		       "and 'Page Down' buttons to scroll.\n"
		       "Make sure the attribute treeitem is selected\n"
		       "and that the mouse pointer is in the scene.") );
    uiSelectFromList dlg( parent(), setup );
    if ( dlg.selFld() )
	dlg.selFld()->setMultiChoice( true );
    if ( !dlg.go() || !dlg.selFld() ) return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getChosen( selattribs );
    if ( selattribs.isEmpty() ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Loading Horizon Data" );
    exgrp.setNrDoneText( tr("Positions read") );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiTaskRunner exdlg( parent() );
    return TaskRunner::execute( &exdlg, exgrp );
}


bool uiEMPartServer::storeObject( const DBKey& id, bool storeas ) const
{
    DBKey dummykey;
    return storeObject( id, storeas, dummykey );
}


bool uiEMPartServer::storeObject( const DBKey& id, bool storeas,
				  DBKey& storagekey,
				  float shift ) const
{
    EM::EMObject* object = em_.getObject( id );
    if ( !object ) return false;

    mDynamicCastGet(EM::Surface*,surface,object);
    mDynamicCastGet(EM::Body*,body,object);

    PtrMan<Executor> exec = 0;
    DBKey key = object->dbKey();

    if ( storeas )
    {
	if ( surface )
	{
	    uiWriteSurfaceDlg dlg( parent(), *surface, shift );
	    dlg.showAlwaysOnTop(); // hack to show on top of tracking window
	    if ( !dlg.go() ) return false;

	    EM::SurfaceIOData sd;
	    EM::SurfaceIODataSelection sel( sd );
	    dlg.getSelection( sel );

	    key = dlg.ioObj() ? dlg.ioObj()->key() : DBKey::getInvalid();
	    exec = surface->geometry().saver( &sel, &key );
	    if ( exec && dlg.replaceInTree() )
		    surface->setDBKey( key );

	    mDynamicCastGet( EM::dgbSurfaceWriter*, writer, exec.ptr() );
	    if ( writer ) writer->setShift( shift );
	}
	else
	{
	    CtxtIOObj ctio( body ? EMBodyTranslatorGroup::ioContext()
				 : object->getIOObjContext(),
				 DBM().get(object->dbKey()) );

	    ctio.ctxt_.forread_ = false;

	    uiIOObjSelDlg dlg( parent(), ctio );
	    if ( !ctio.ioobj_ )
		dlg.selGrp()->getNameField()->setText( object->name() );

	    if ( !dlg.go() )
		return false;

	    if ( dlg.ioObj() )
	    {
		key = dlg.ioObj()->key();
		object->setDBKey( key );
	    }

	    exec = object->saver();
	}
    }
    else
	exec = object->saver();

    if ( !exec )
	return false;

    PtrMan<IOObj> ioobj = DBM().get( key );
    if ( !ioobj->pars().find( sKey::Type() ) )
    {
	ioobj->pars().set( sKey::Type(), object->getTypeStr() );
	if ( !DBM().setEntry( *ioobj ) )
	{
	    uiMSG().error( uiStrings::phrCannotWriteDBEntry(ioobj->uiName()) );
	    return false;
	}
    }

    object->resetChangedFlag();
    storagekey = key;
    uiTaskRunner exdlg( parent() );
    const bool ret = TaskRunner::execute( &exdlg, *exec );
    if( ret && surface )
	surface->saveDisplayPars();

    return ret;

}


bool uiEMPartServer::storeAuxData( const DBKey& id,
				   BufferString& auxdatanm, bool storeas ) const
{
    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet(EM::Horizon3D*,hor3d,object);
    if ( !hor3d )
	return false;

    const ObjectSet<BinIDValueSet>& datastor = hor3d->auxdata.getData();
    if ( datastor.isEmpty() )
	return false;

    bool hasdata = false;
    for ( int idx=0; idx<datastor.size(); idx++ )
    {
	const BinIDValueSet* bvs = datastor[idx];
	if ( bvs && !bvs->isEmpty() )
	{
	    hasdata = true;
	    break;
	}
    }

    if ( !hasdata )
	return false;

    uiTaskRunner exdlg( parent() );
    int dataidx = -1;
    bool overwrite = false;
    if ( storeas )
    {
	if ( hor3d )
	{
	    uiStoreAuxData dlg( parent(), *hor3d );
	    if ( !dlg.go() ) return false;

	    dataidx = 0;
	    overwrite = dlg.doOverWrite();
	    auxdatanm = dlg.auxdataName();
	}
    }

    PtrMan<Executor> saver = hor3d->auxdata.auxDataSaver(dataidx,overwrite);
    if ( !saver )
    {
	uiMSG().error( tr("Cannot save attribute") );
	return false;
    }

    return TaskRunner::execute( &exdlg, *saver );
}


int uiEMPartServer::setAuxData( const DBKey& id, DataPointSet& data,
				const char* attribnm, int idx, float shift )
{
    if ( !data.size() )
    { uiMSG().error(tr("No data calculated")); return -1; }

    EM::EMObject* object = em_.getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
    { uiMSG().error(tr("No horizon loaded")); return -1; }

    BufferString auxnm = attribnm;
    if ( auxnm.isEmpty() )
    {
	auxnm = "AuxData";
	auxnm += idx;
    }

    const BinIDValueSet& bivs = data.bivSet();
    const int nrdatavals = bivs.nrVals();
    if ( idx>=nrdatavals ) return -1;

    hor3d->auxdata.removeAll();
    const int auxdataidx = hor3d->auxdata.addAuxData( auxnm );
    hor3d->auxdata.setAuxDataShift( auxdataidx, shift );

    BinID bid;
    BinIDValueSet::SPos pos;
    while ( bivs.next(pos) )
    {
	bid = bivs.getBinID( pos );
	const float* vals = bivs.getVals( pos );

	RowCol rc( bid.inl(), bid.crl() );
	EM::PosID posid = EM::PosID::getFromRowCol( rc );
	hor3d->auxdata.setAuxDataVal( auxdataidx, posid, vals[idx] );
    }

    return auxdataidx;
}


bool uiEMPartServer::getAuxData( const DBKey& oid, int auxdataidx,
				 DataPointSet& data, float& shift ) const
{
    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    const char* nm = hor3d->auxdata.auxDataName( auxdataidx );
    if ( !hor3d || !nm )
	return false;

    shift = hor3d->auxdata.auxDataShift( auxdataidx );
    data.dataSet().add( new DataColDef(sKeySectionID()) );
    data.dataSet().add( new DataColDef(nm) );

    float auxvals[3];
    if ( !hor3d->geometry().geometryElement() )
	return false;

    auxvals[1] = 0;
    PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator();
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	auxvals[0] = (float) hor3d->getPos( pid ).z_;
	auxvals[2] = hor3d->auxdata.getAuxDataVal( auxdataidx, pid );
	data.bivSet().add( pid.getBinID(), auxvals );
    }

    data.dataChanged();
    return true;
}


bool uiEMPartServer::getAllAuxData( const DBKey& oid,
	DataPointSet& data, TypeSet<float>* shifts,
	const TrcKeyZSampling* cs ) const
{
    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    data.dataSet().add( new DataColDef(sKeySectionID()) );

    BufferStringSet nms;
    for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
    {
	if ( hor3d->auxdata.auxDataName(idx) )
	{
	    const char* nm = hor3d->auxdata.auxDataName( idx );
	    *shifts += hor3d->auxdata.auxDataShift( idx );
	    nms.add( nm );
	    data.dataSet().add( new DataColDef(nm) );
	}
    }

    data.bivSet().allowDuplicateBinIDs(false);
    mAllocVarLenArr( float, auxvals, nms.size()+2 );
    if ( !hor3d->geometry().geometryElement() )
	return false;

    auxvals[0] = 0;
    auxvals[1] = 0;
    PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator(cs);
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.isInvalid() )
	    break;

	BinID bid = pid.getBinID();
	if ( cs )
	{
	    if ( !cs->hsamp_.includes(bid) )
		continue;

	    BinID diff = bid - cs->hsamp_.start_;
	    if ( diff.inl() % cs->hsamp_.step_.inl() ||
		 diff.crl() % cs->hsamp_.step_.crl() )
		continue;
	}

	auxvals[0] = (float) hor3d->getPos( pid ).z_;
	for ( int idx=0; idx<nms.size(); idx++ )
	{
	    const int auxidx = hor3d->auxdata.auxDataIndex( nms.get(idx) );
	    auxvals[idx+2] = hor3d->auxdata.getAuxDataVal( auxidx, pid );
	}
	data.bivSet().add( bid, mVarLenArr(auxvals) );
    }

    data.dataChanged();
    return true;
}


bool uiEMPartServer::interpolateAuxData( const DBKey& oid,
	const char* nm, DataPointSet& dpset )
{ return changeAuxData( oid, nm, true, dpset ); }


bool uiEMPartServer::filterAuxData( const DBKey& oid,
	const char* nm, DataPointSet& dpset )
{ return changeAuxData( oid, nm, false, dpset ); }


static int getColID( const DataPointSet& dps, const char* nm )
{
    const BinIDValueSet& bivs = dps.bivSet();
    int cid = -1;
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
    {
	BufferString colnm = dps.dataSet().colDef(idx).name_;
	if ( colnm.isEqual(nm) )
	    { cid = idx; break; }
    }
    return cid;
}


bool uiEMPartServer::computeVariogramAuxData( const DBKey& oid,
					      const char* nm,
					      DataPointSet& dpset )
{
    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid < 0 || mIsUdf(cid) )
	return false;

    const StepInterval<int> rowrg = hor3d->geometry().rowRange();
    const StepInterval<int> colrg = hor3d->geometry().colRange( -1 );
    BinID step( rowrg.step, colrg.step );
//    BIDValSetArrAdapter adapter( bivs, cid, step );

    uiVariogramDlg varsettings( parent(), false );
    if ( !varsettings.go() )
	return false;

    int size = varsettings.getMaxRg();
    size /= SI().inlDistance() <= SI().crlDistance() ?
	int(SI().inlDistance()) : int(SI().crlDistance());
    size++;

    uiString errmsg;
    bool msgiserror = true;

    HorVariogramComputer hvc( dpset, size, cid, varsettings.getMaxRg(),
			      varsettings.getFold(), errmsg, msgiserror );
    if ( !hvc.isOK() )
    {
	msgiserror ? uiMSG().error( errmsg )
		   : uiMSG().warning( errmsg );
	return false;
    }
    uiVariogramDisplay* uivv = new uiVariogramDisplay( parent(), hvc.getData(),
						       hvc.getXaxes(),
						       hvc.getLabels(),
						       varsettings.getMaxRg(),
						       true );
    variodlgs_ += uivv;
    uivv->draw();
    uivv->go();
    return true;
}


bool uiEMPartServer::changeAuxData( const DBKey& oid,
	const char* nm, bool interpolate, DataPointSet& dpset )
{
    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    const StepInterval<int> rowrg = hor3d->geometry().rowRange();
    const StepInterval<int> colrg = hor3d->geometry().colRange( -1 );
    BinID step( rowrg.step, colrg.step );
    BIDValSetArrAdapter adapter( bivs, cid, step );

    PtrMan<Task> changer;
    uiTaskRunner execdlg( parent() );
    if ( interpolate )
    {
        uiSingleGroupDlg<uiArray2DInterpolSel> dlg( parent(),
                new uiArray2DInterpolSel( 0, false, false, true, 0 ) );

        dlg.setCaption( uiStrings::sInterpolation() );
        dlg.setTitleText( uiStrings::sSettings() );

	if ( !dlg.go() ) return false;

	Array2DInterpol* interp = dlg.getDlgGroup()->getResult();
	if ( !interp )
	    return false;

	changer = interp;
	interp->setFillType( Array2DInterpol::Full );
	const float inldist = SI().inlDistance();
	const float crldist = SI().crlDistance();
	interp->setRowStep( rowrg.step*inldist );
	interp->setColStep( colrg.step*crldist );

	PtrMan< Array2D<float> > arr = hor3d->createArray2D();
	const float* arrptr = arr ? arr->getData() : 0;
	if ( arrptr )
	{
	    Array2D<bool>* mask = new Array2DImpl<bool>( arr->info() );
	    bool* maskptr = mask->getData();
	    if ( maskptr )
	    {
		for ( int idx=mCast(int,mask->info().getTotalSz()-1); idx>=0;
									idx-- )
		{
		    *maskptr = !mIsUdf(*arrptr);
		    maskptr++;
		    arrptr++;
		}
	    }

	    interp->setMask( mask, OD::TakeOverPtr );
	}

	if ( !interp->setArray( adapter, &execdlg ) )
	    return false;
    }
    else
    {
	uiArr2DFilterParsDlg dlg( parent() );
	if ( !dlg.go() ) return false;

	Array2DFilterer<float>* filter =
	    new Array2DFilterer<float>( adapter, dlg.getInput() );
	changer = filter;
    }

    if ( !TaskRunner::execute( &execdlg, *changer ) )
	return false;

    mDynamicCastGet(const Array2DInterpol*,interp,changer.ptr())
    const uiString infomsg =
			interp ? interp->infoMsg() : uiStrings::sEmptyString();
    if ( !infomsg.isEmpty() )
	uiMSG().message( infomsg );

    return true;
}


bool uiEMPartServer::attr2Geom( const DBKey& oid,
	const char* nm, const DataPointSet& dpset )
{
    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorAttr2Geom dlg( parent(), *hor3d, dpset, cid );
    if ( !dlg.go() )
	return false;

    if ( dlg.saveFldGrp()->displayNewHorizon() &&
	 dlg.saveFldGrp()->getNewHorizon( ) )
	displayEMObject( dlg.saveFldGrp()->getNewHorizon()->dbKey() );

    return true;
}


bool uiEMPartServer::geom2Attr( const DBKey& oid )
{
    EM::EMObject* object = em_.getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorGeom2Attr dlg( parent(), *hor3d );
    return dlg.go();
}


void uiEMPartServer::removeUndo()
{
    em_.undo().removeAll();
}


bool uiEMPartServer::loadSurface( const DBKey& mid,
				  const EM::SurfaceIODataSelection* newsel )
{
    if ( em_.getObject(mid) )
	return true;

    Executor* exec = em_.objectLoader( mid, newsel );
    if ( !exec )
    {
	PtrMan<IOObj> ioobj = DBM().get(mid);
	BufferString nm = ioobj ? (const char*)ioobj->name()
				: (const char*)mid.toString().str();
	uiString msg = tr( "Cannot load '%1'" ).arg( nm );
	uiMSG().error( msg );
	return false;
    }

    EM::EMObject* obj = em_.getObject( mid );
    obj->ref();
    uiTaskRunner exdlg( parent() );
    if ( !TaskRunner::execute( &exdlg, *exec ) )
    {
	obj->unRef();
	return false;
    }

    delete exec;
    obj->unRefNoDelete();
    return true;
}


DBKey uiEMPartServer::genRandLine( int opt )
{
    DBKey res;
    if ( opt == 0 )
    {
	uiGenRanLinesByShift dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else if ( opt == 1 )
    {
	uiGenRanLinesByContour dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else
    {
	uiGenRanLineFromPolygon dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }

    return res;
}


ZAxisTransform* uiEMPartServer::getHorizonZAxisTransform( bool is2d )
{
    uiDialog dlg( parent(),
		 uiDialog::Setup( uiStrings::phrSelect( uiStrings::sHorizon() ),
                 mNoDlgTitle, mODHelpKey(mgetHorizonZAxisTransformHelpID)));
    const IOObjContext ctxt = is2d
	? EMHorizon2DTranslatorGroup::ioContext()
	: EMHorizon3DTranslatorGroup::ioContext();
    uiIOObjSel* horfld = new uiIOObjSel( &dlg, ctxt );
    if ( !dlg.go() || !horfld->ioobj() ) return 0;

    const DBKey emid = horfld->key();
    if ( emid.isInvalid() ) return 0;

    EM::EMObject* obj = em_.getObject( emid );
    if ( !obj || !obj->isFullyLoaded() )
    {
	if ( !loadSurface( emid ) )
	    return 0;
    }

    obj = em_.getObject( emid );
    mDynamicCastGet(EM::Horizon*,hor,obj)
    if ( !hor ) return 0;

    EM::HorizonZTransform* transform = new EM::HorizonZTransform();
    transform->setHorizon( *hor );
    return transform;
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    for ( int idx=0; idx<em_.nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(em_.objID(idx)));
	if ( hor3d )
	    hinfos += new SurfaceInfo( hor3d->name(), hor3d->dbKey() );
    }
}


void uiEMPartServer::getAllSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos,
					bool is2d )
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Surf );
    if ( !dbdir )
	return;

    FixedString groupstr = is2d
	? EMHorizon2DTranslatorGroup::sGroupName()
	: EMHorizon3DTranslatorGroup::sGroupName();
    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ioobj.group() == groupstr )
	    hinfos += new SurfaceInfo( ioobj.name(), ioobj.key() );
    }
}


void uiEMPartServer::getSurfaceDef3D( const DBKeySet& selhorids,
				    BinIDValueSet& bivs,
				    const TrcKeySampling& hs ) const
{
    bivs.setEmpty(); bivs.setNrVals( 2 );

    const DBKey& id = selhorids[0];
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(id))
    if ( !hor3d ) return;
    hor3d->ref();

    EM::Horizon3D* hor3d2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor3d2 = (EM::Horizon3D*)(em_.getObject(selhorids[1]));
	hor3d2->ref();
    }

    BinID bid;
    for ( bid.inl()=hs.start_.inl(); bid.inl()<=hs.stop_.inl();
	  bid.inl()+=hs.step_.inl() )
    {
	for ( bid.crl()=hs.start_.crl(); bid.crl()<=hs.stop_.crl();
	      bid.crl()+=hs.step_.crl() )
	{
	    const EM::PosID posid = EM::PosID::getFromRowCol( bid );
	    float z1pos = mUdf(float), z2pos = mUdf(float);
	    if ( !hor3d->isDefined(posid) )
		continue;

	    z1pos = (float) hor3d->getPos( posid ).z_;
	    if ( !hor3d2 )
		z2pos = z1pos;
	    else
	    {
		if ( !hor3d2->isDefined(posid) )
		    continue;

		z2pos = (float) hor3d2->getPos( posid ).z_;
	    }

	    Interval<float> zintv( z1pos, z2pos );
	    zintv.sort();
	    bivs.add( bid, zintv.start, zintv.stop );
	}
    }

    hor3d->unRef();
    if ( hor3d2 ) hor3d2->unRef();
}


#define mGetEMObj( num, obj ) \
{ \
    const DBKey horid = selhorids[num]; \
    if ( horid.isInvalid() || !isFullyLoaded(horid) ) \
	loadSurface( horid ); \
    obj = em_.getObject( horid ); \
}
void uiEMPartServer::getSurfaceDef2D( const DBKeySet& selhorids,
				  const BufferStringSet& selectlines,
				  TypeSet<Coord>& coords,
				  TypeSet<Pos::GeomID>& geomids,
				  TypeSet< Interval<float> >& zrgs )
{
    if ( selhorids.isEmpty() )
	return;

    DBKey id;
    const bool issecondhor = selhorids.size()>1;
    EM::EMObject* obj = 0;
    mGetEMObj(0,obj);
    mDynamicCastGet(EM::Horizon2D*,hor2d1,obj);

    EM::Horizon2D* hor2d2 = 0;
    if ( issecondhor )
    {
	mGetEMObj(1,obj);
	mDynamicCastGet(EM::Horizon2D*,hor,obj);
	hor2d2 = hor;
    }

    if ( !hor2d1 || ( issecondhor && !hor2d2 ) ) return;

    for ( int lidx=0; lidx<selectlines.size(); lidx++ )
    {
	int lineidx = hor2d1->geometry().lineIndex( selectlines.get(lidx) );
	if ( lineidx<0 ) continue;

	const Pos::GeomID geomid = hor2d1->geometry().geomID( lineidx );
	const StepInterval<int> trcrg = hor2d1->geometry().colRange( geomid );
	if ( trcrg.isUdf() ) continue;

	for ( int trcidx=0; trcidx<=trcrg.nrSteps(); trcidx++ )
	{
	    const int trcnr = trcrg.atIndex( trcidx );
	    const Coord3 pos1 = hor2d1->getPos( geomid, trcnr );
	    const float z1 = mCast( float, pos1.z_ );
	    float z2 = mUdf(float);

	    if ( issecondhor )
		z2 = mCast( float, hor2d2->getPos(geomid,trcnr).z_ );

	    if ( !mIsUdf(z1) && ( !issecondhor || !mIsUdf(z2) ) )
	    {
		Interval<float> zrg( z1, issecondhor ? z2 : z1 );
		zrgs += zrg;
		coords += pos1.getXY();
		geomids += geomid;
	    }
	}
    }
}


void uiEMPartServer::fillPickSet( Pick::Set& ps, DBKey horid )
{
    if ( horid.isInvalid() || !isFullyLoaded(horid) )
	loadSurface( horid );

    EM::EMObject* obj = em_.getObject( horid );
    mDynamicCastGet(EM::Horizon3D*,hor,obj)
    if ( !hor )
	return;

    hor->ref();
    Pick::SetIter4Edit psiter( ps );
    while ( psiter.next() )
    {
	const BinID bid = psiter.get().binID();
	const EM::PosID posid = EM::PosID::getFromRowCol( bid );
	double zval = hor->getPos( posid ).z_;

	if ( mIsUdf(zval) )
	{
	    const Geometry::BinIDSurface* geom =
		hor->geometry().geometryElement();
	    if ( geom )
		zval = geom->computePosition( Coord(bid.inl(),bid.crl()) ).z_;
	}
	if ( mIsUdf(zval) )
	    psiter.removeCurrent();
	else
	    psiter.get().setZ( zval );
    }

    hor->unRef();
}


void uiEMPartServer::managePreLoad()
{
    uiHorizonPreLoadDlg dlg( appserv().parent() );
    dlg.go();
}


void uiEMPartServer::fillPar( IOPar& par ) const
{
    const DBKeySet& mids = EM::HPreL().getPreloadedIDs();
    for ( int idx=0; idx<mids.size(); idx++ )
	par.set( IOPar::compKey(sKeyPreLoad(),idx), mids[idx] );
}


bool uiEMPartServer::usePar( const IOPar& par )
{
    const int maxnr2pl = 1000;
    DBKeySet mids;
    for ( int idx=0; idx<maxnr2pl; idx++ )
    {
	DBKey mid;
	par.get( IOPar::compKey(sKeyPreLoad(),idx), mid );
	if ( mid.isInvalid() )
	    break;

	mids += mid;
    }

    uiTaskRunner uitr( parent() );
    EM::HPreL().load( mids, &uitr );
    return true;
}
