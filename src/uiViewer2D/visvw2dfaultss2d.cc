/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dfaultss2d.cc,v 1.2 2010-07-29 12:03:17 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2dfaultss2d.h"

#include "attribdataholder.h"
#include "attribdatapack.h"
#include "flatauxdataeditor.h"
#include "mpefssflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


VW2DFautSS2D::VW2DFautSS2D( const EM::ObjectID& oid, uiFlatViewWin* mainwin,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DDataObject()
    , emid_(oid)
    , viewerwin_(mainwin)
    , auxdataeditors_(auxdataeds)
    , deselted_( this )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const FlatDataPack* fdp = viewerwin_->viewer( ivwr ).pack( true );
	if ( !fdp )
	    fdp = viewerwin_->viewer( ivwr ).pack( false );
	if ( !fdp )
	{
	    fsseds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp);
	if ( !dp2ddh )
	{
	    fsseds_ += 0;
	    continue;
	}

	MPE::FaultStickSetFlatViewEditor* fssed =
	    new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]), oid );
	fsseds_ += fssed;
    }
}


VW2DFautSS2D::~VW2DFautSS2D()
{
    deepErase(fsseds_);
}


void VW2DFautSS2D::setLineName( const char* ln )
{
    linenm_ = ln;
}


void VW2DFautSS2D::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	const FlatDataPack* fdp = vwr.pack( true );
	if ( !fdp )
	    fdp = vwr.pack( false );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp);
	if ( !dp2ddh ) continue;

	if ( fsseds_[ivwr] )
	{
	    dp2ddh->getPosDataTable( fsseds_[ivwr]->getTrcNos(),
		    		     fsseds_[ivwr]->getDistances() );
	    dp2ddh->getCoordDataTable( fsseds_[ivwr]->getTrcNos(),
		    		       fsseds_[ivwr]->getCoords() );
	    fsseds_[ivwr]->setLineName( linenm_ );
	    fsseds_[ivwr]->setLineID( lsetid_ );
	    fsseds_[ivwr]->set2D( true );
	    fsseds_[ivwr]->drawFault();
	}
    }
}


void VW2DFautSS2D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	    fsseds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFautSS2D::selected( bool enabled )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    if ( enabled )
		fsseds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( true && enabled );
	}
    }
}


void VW2DFautSS2D::triggerDeSel()
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
