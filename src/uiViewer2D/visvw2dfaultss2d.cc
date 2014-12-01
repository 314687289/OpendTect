/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visvw2dfaultss2d.h"

#include "attribdataholder.h"
#include "attribdatapack.h"
#include "flatauxdataeditor.h"
#include "faultstickseteditor.h"
#include "mpeengine.h"
#include "mpefssflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "hiddenparam.h"

mCreateVw2DFactoryEntry( VW2DFaultSS2D );

HiddenParam< VW2DFaultSS2D, bool* >		fss2dknotenabled_( 0 );

VW2DFaultSS2D::VW2DFaultSS2D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DEMDataObject(oid,win,auxdataeds)
    , deselted_( this )
    , fsseditor_(0)
{
    fss2dknotenabled_.setParam(this,new bool(false));
    fsseds_.allowNull();
    if ( oid >= 0 )
	setEditors();
}


void VW2DFaultSS2D::setEditors() 
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
	ConstDataPackRef<FlatDataPack>	fdp = vwr.obtainPack( true );
	if ( !fdp )
	{
	    fsseds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp.ptr());
	if ( !dp2ddh )
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


VW2DFaultSS2D::~VW2DFaultSS2D()
{
    deepErase(fsseds_);
    if ( fsseditor_ )
    {
	fsseditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }

    delete fss2dknotenabled_.getParam(this);
    fss2dknotenabled_.removeParam(this);
}


void VW2DFaultSS2D::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( true, true );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp.ptr());
	if ( !dp2ddh ) continue;

	if ( fsseds_[ivwr] )
	{
	    dp2ddh->getPosDataTable( fsseds_[ivwr]->getTrcNos(),
		    		     fsseds_[ivwr]->getDistances() );
	    dp2ddh->getCoordDataTable( fsseds_[ivwr]->getTrcNos(),
		    		       fsseds_[ivwr]->getCoords() );
	    fsseds_[ivwr]->setGeomID( geomid_ );
	    fsseds_[ivwr]->set2D( true );
	    fsseds_[ivwr]->drawFault();
	}
    }
}


void VW2DFaultSS2D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->enablePainting( yn );
	    fsseds_[ivwr]->enableKnots( *fss2dknotenabled_.getParam(this) );
	}
    }
}


void VW2DFaultSS2D::selected()
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
	    *fss2dknotenabled_.getParam(this) = iseditable;
	}
    }
}


void VW2DFaultSS2D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( false );
	}
    }

    *fss2dknotenabled_.getParam(this) = false;
    deselted_.trigger();
}
