/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiemattribpartserv.cc,v 1.13 2009-07-20 11:18:09 cvsnageswara Exp $";


#include "uiemattribpartserv.h"

#include "posvecdataset.h"
#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uihorizonshiftdlg.h"
#include "uiimphorizon2d.h"
#include "uiimpfaultstickset2d.h"
#include "uiseiseventsnapper.h"

#include "datapointset.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "ioman.h"
#include "ioobj.h"
#include "typeset.h"

static const DataColDef	siddef_( "Section ID" );

const DataColDef& uiEMAttribPartServer::sidDef() const
{ return siddef_; }

uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , nlamodel_(0)
    , descset_(0)
    , horshiftdlg_(0)
    , shiftidx_(10)
    , attribidx_(0)
{}


void uiEMAttribPartServer::createHorizonOutput( HorOutType type )
{
    if ( !descset_ ) return;

    if ( type==OnHor )
    {
	uiAttrSurfaceOut dlg( parent(), *descset_, nlamodel_, nlaid_ );
	dlg.go();
    }
    else
    {
	uiAttrTrcSelOut dlg( parent(), *descset_, nlamodel_, nlaid_,
			     type==AroundHor );
	dlg.go();
    }
}


void uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid )
{
    IOObj* ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    uiSeisEventSnapper dlg( parent(), ioobj );
    dlg.go();
    delete ioobj;
}


void uiEMAttribPartServer::import2DHorizon() const
{
    uiImportHorizon2D dlg( parent() );
    dlg.go();
}


float uiEMAttribPartServer::getShift() const
{
    return horshiftdlg_ ? horshiftdlg_->getShift() : mUdf(float);
}


StepInterval<float> uiEMAttribPartServer::shiftRange() const
{
    StepInterval<float> res;
    if ( horshiftdlg_ )
	res = horshiftdlg_->shiftRg();

    return res;
}


void uiEMAttribPartServer::showHorShiftDlg( const EM::ObjectID& id,
					    const BoolTypeSet& attrenabled,
					    float initialshift,
					    bool canaddattrib)
{
    sendEvent( uiEMAttribPartServer::evShiftDlgOpened() );

    initialshift_ = initialshift;
    initialattribstatus_ = attrenabled;
    setAttribIdx( mUdf(int) );
    horshiftdlg_ = new uiHorizonShiftDialog( appserv().parent(),
	    id, *descset_, initialshift, canaddattrib );

    horshiftdlg_->calcAttribPushed.notify(
	    mCB(this,uiEMAttribPartServer,calcDPS) );
    horshiftdlg_->horShifted.notify(
	    mCB(this,uiEMAttribPartServer,horShifted) );
    horshiftdlg_->windowClosed.notify(
	    mCB(this,uiEMAttribPartServer,shiftDlgClosed));

    horshiftdlg_->go();
    horshiftdlg_->setDeleteOnClose( true );
}


void uiEMAttribPartServer::shiftDlgClosed( CallBacker* cb )
{
    if ( horshiftdlg_->uiResult()==1 )
    {
	if ( horshiftdlg_->doStore() )
	    sendEvent( uiEMAttribPartServer::evStoreShiftHorizons() );

	sendEvent( uiEMAttribPartServer::evShiftDlgClosedOK() );
    }
    else
	sendEvent( uiEMAttribPartServer::evShiftDlgClosedCancel() );
}


void uiEMAttribPartServer::calcDPS( CallBacker* cb )
{
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evCalcShiftAttribute() );
}


const char* uiEMAttribPartServer::getAttribBaseNm() const
{ return horshiftdlg_ ? horshiftdlg_->getAttribBaseName() : 0; }


void uiEMAttribPartServer::fillHorShiftDPS( ObjectSet<DataPointSet>& dpsset,
       					    TaskRunner* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    StepInterval<float> intv = horshiftdlg_->shiftRg();

    const int nrshifts = horshiftdlg_->nrSteps();
    const EM::Horizon3DGeometry& hor3dgeom =
	horshiftdlg_->horizon3D().geometry();
    for ( int idx=0; idx<nrshifts; idx++ )
    {
	const float shift = intv.atIndex(idx);
	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );

	dps->dataSet().add( new DataColDef( siddef_ ) );
	dps->bivSet().setNrVals( 3 );
	dpsset += dps;
    }

    DataPointSet::DataRow datarow;
    datarow.data_.setSize( 1, mUdf(float) );
    for ( int sididx=0; sididx<hor3dgeom.nrSections(); sididx++ )
    {
	const EM::SectionID sid = hor3dgeom.sectionID(sididx);
	datarow.data_[0] = sid;
	const int nrknots = hor3dgeom.sectionGeometry(sid)->nrKnots();

	for ( int idx=0; idx<nrknots; idx++ )
	{
	    const BinID bid =
		    hor3dgeom.sectionGeometry(sid)->getKnotRowCol(idx);
	    const float realz = 
		hor3dgeom.sectionGeometry(sid)->getKnot( bid, false ).z;
	    if ( mIsUdf(realz) )
		continue;

	    datarow.pos_.binid_ = bid;

	    for ( int shiftidx=0; shiftidx<nrshifts; shiftidx++ )
	    {
		const float shift = intv.atIndex(shiftidx);
		datarow.pos_.z_ = realz+shift;
		dpsset[shiftidx]->addRow( datarow );
	    }

	}
    }

    for ( int shiftidx=0; shiftidx<nrshifts; shiftidx++ )
	dpsset[shiftidx]->dataChanged();
}


int uiEMAttribPartServer::textureIdx() const
{ return horshiftdlg_->curShiftIdx(); }


void uiEMAttribPartServer::setAttribIdx( int idx )
{
    attribidx_ = idx;
}


void uiEMAttribPartServer::horShifted( CallBacker* cb )
{
    shiftidx_ = horshiftdlg_->curShiftIdx();
    sendEvent( uiEMAttribPartServer::evHorizonShift() );
}


void uiEMAttribPartServer::import2DFaultStickset( const char* type )
{
    uiImportFaultStickSet2D fssdlg( parent(), type );
    fssdlg.go();
}
