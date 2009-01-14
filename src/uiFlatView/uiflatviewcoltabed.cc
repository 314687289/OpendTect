/*
___________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          May 2008
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewcoltabed.cc,v 1.7 2009-01-14 11:15:50 cvsnanne Exp $";

#include "uiflatviewcoltabed.h"

#include "callback.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "flatview.h"

#include "uicolortable.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiParent* p, FlatView::Viewer& vwr )
    : ddpars_(vwr.appearance().ddpars_)
    , vwr_( &vwr )
    , colseq_(*new ColTab::Sequence())
    , colTabChgd(this)
{
    ColTab::SM().get( ddpars_.vd_.ctab_.buf(), colseq_ );
    uicoltab_ = new uiColorTable( 0, colseq_, false );
    uicoltab_->setEnabManage( false );
    uicoltab_->seqChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    uicoltab_->scaleChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged));
    setColTab( vwr );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    delete &colseq_;
}


void uiFlatViewColTabEd::setColTab( const FlatView::Viewer& vwr)
{
    uicoltab_->setSequence( vwr.appearance().ddpars_.vd_.ctab_ );
    uicoltab_->setDispPars( vwr.appearance().ddpars_.vd_ );
    uicoltab_->setInterval( vwr.getDataRange(false) );
}


void uiFlatViewColTabEd::colTabChanged( CallBacker* )
{
    ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
    uicoltab_->getDispPars( ddpars_.vd_ );
    colTabChgd.trigger();
}
