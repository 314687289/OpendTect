/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uihorizonshiftdlg.h"
#include "uihorsavefieldgrp.h"
#include "uiimphorizon2d.h"
#include "uiseiseventsnapper.h"

#include "datapointset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "posvecdataset.h"
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
    , uiimphor2ddlg_(0)
    , uiseisevsnapdlg_(0)
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


void uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid, bool is2d )
{
    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    if ( !ioobj ) return;

    if ( uiseisevsnapdlg_ ) delete uiseisevsnapdlg_;
    uiseisevsnapdlg_ = new uiSeisEventSnapper( parent(), ioobj, is2d );
    uiseisevsnapdlg_->readyForDisplay.notify(
		mCB(this,uiEMAttribPartServer,readyForDisplayCB) );
    uiseisevsnapdlg_->show();
}


void uiEMAttribPartServer::import2DHorizon()
{
    if ( uiimphor2ddlg_ )
    {
	uiimphor2ddlg_->show();
	uiimphor2ddlg_->raise();
	return;
    }

    uiimphor2ddlg_ = new uiImportHorizon2D( parent() );
    uiimphor2ddlg_->readyForDisplay.notify(
		mCB(this,uiEMAttribPartServer,readyForDisplayCB) );
    uiimphor2ddlg_->show();
}


void uiEMAttribPartServer::readyForDisplayCB( CallBacker* cb )
{
    emobjids_.erase();
    if ( uiseisevsnapdlg_ && cb==uiseisevsnapdlg_ )
    {
	uiHorSaveFieldGrp* grp = uiseisevsnapdlg_->saveFldGrp();
	if ( !grp->getNewHorizon() )
	    return;

	emobjids_ += grp->getNewHorizon()->id();
    }
    else if ( uiimphor2ddlg_ && cb==uiimphor2ddlg_ )
	uiimphor2ddlg_->getEMObjIDs( emobjids_ );

    if ( !emobjids_.isEmpty() )
	sendEvent( evDisplayEMObject() );
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
					    const int& visid,
					    const BoolTypeSet& attrenabled,
					    float initialshift,
					    bool canaddattrib)
{
    sendEvent( uiEMAttribPartServer::evShiftDlgOpened() );

    initialshift_ = initialshift;
    initialattribstatus_ = attrenabled;
    setAttribIdx( mUdf(int) );
    horshiftdlg_ = new uiHorizonShiftDialog( appserv().parent(), id, visid,
					     *descset_, initialshift,
					     canaddattrib );
    horshiftdlg_->calcAttribPushed.notify(
	    mCB(this,uiEMAttribPartServer,calcDPS) );
    horshiftdlg_->horShifted.notify(
	    mCB(this,uiEMAttribPartServer,horShifted) );
    horshiftdlg_->windowClosed.notify(
	    mCB(this,uiEMAttribPartServer,shiftDlgClosed));

    horshiftdlg_->go();
    horshiftdlg_->setDeleteOnClose( true );
}


void uiEMAttribPartServer::shiftDlgClosed( CallBacker* )
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


void uiEMAttribPartServer::calcDPS( CallBacker* )
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
	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );

	dps->dataSet().add( new DataColDef( siddef_ ) );
	dps->bivSet().setNrVals( dps->nrFixedCols() + 2 );
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
	    const float realz = (float) (
		hor3dgeom.sectionGeometry(sid)->getKnot( bid, false ).z );
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

int uiEMAttribPartServer::getShiftedObjectVisID() const
{
    return horshiftdlg_ ? horshiftdlg_->visID() : -1;
}
