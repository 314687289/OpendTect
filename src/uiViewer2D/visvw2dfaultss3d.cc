/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visvw2dfaultss3d.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "zaxistransformutils.h"
#include "emeditor.h"
#include "flatauxdataeditor.h"
#include "faultstickseteditor.h"
#include "mpeengine.h"
#include "mpefssflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

mCreateVw2DFactoryEntry( VW2DFaultSS3D );

VW2DFaultSS3D::VW2DFaultSS3D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DEMDataObject(oid,win,auxdataeds)
    , deselted_( this )
    , fsseditor_(0)
{
    fsseds_.allowNull();
    if ( oid >= 0)
	setEditors();
}


void VW2DFaultSS3D::setEditors() 
{
    deepErase( fsseds_ ); 
    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid_, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    fsseditor_ = fsseditor;
    if ( fsseditor_ )
	fsseditor_->ref();

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( true );
	if ( !fdp )
	{
	    fsseds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp.ptr());
	mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp.ptr());
	mDynamicCastGet(const ZAxisTransformDataPack*,zatdp3d,fdp.ptr());
	if ( !dp3d && !dprdm && !zatdp3d )
	{
	    fsseds_ += 0;
	    continue;
	}

	MPE::FaultStickSetFlatViewEditor* fssed = 
	    new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	fsseds_ += fssed;
    }
}


VW2DFaultSS3D::~VW2DFaultSS3D()
{
    deepErase(fsseds_);
    if ( fsseditor_ )
    {
	fsseditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }
}


void VW2DFaultSS3D::setCubeSampling( const CubeSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFaultSS3D::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( true, true );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp.ptr());
	mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp.ptr());
	mDynamicCastGet(const ZAxisTransformDataPack*,zatdp3d,fdp.ptr());
	if ( !dp3d && !dprdm && !zatdp3d ) continue;

	if ( fsseds_[ivwr] )
	{
	    if ( dp3d )
		fsseds_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );

	    if ( dprdm )
	    {
		fsseds_[ivwr]->setPath( dprdm->pathBIDs() );
		fsseds_[ivwr]->setFlatPosData( &dprdm->posData() );
	    }

	    if ( zatdp3d )
		fsseds_[ivwr]->setCubeSampling( zatdp3d->inputCS() );

	    fsseds_[ivwr]->drawFault();
	}
    }
}


void VW2DFaultSS3D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	    fsseds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFaultSS3D::selected()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    const bool iseditable = vwr.appearance().annot_.editable_;
	    if ( iseditable )
		fsseds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( iseditable );
	}
    }
}


void VW2DFaultSS3D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( false );
	}
    }
    deselted_.trigger();
}
