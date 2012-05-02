/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: uigraphicsview.cc,v 1.8 2012-05-02 11:53:58 cvskris Exp $";


#include "uigraphicsview.h"

#include "uitoolbutton.h"
#include "uigraphicssaveimagedlg.h"
#include "uigraphicsscene.h"


uiGraphicsView::uiGraphicsView( uiParent* p, const char* nm )
    : uiGraphicsViewBase(p,nm)
    , enableimagesave_(true)
{
    scene_->ctrlPPressed.notify( mCB(this,uiGraphicsView,saveImageCB) );
}


uiToolButton* uiGraphicsView::getSaveImageButton( uiParent* p )
{
    if ( !enableimagesave_ ) return 0;

    return new uiToolButton( p, "snapshot.png", "Save image",
			mCB(this,uiGraphicsView,saveImageCB) );
}


void uiGraphicsView::enableImageSave()	{ enableimagesave_ = true; }
void uiGraphicsView::disableImageSave()	{ enableimagesave_ = false; }

void uiGraphicsView::saveImageCB( CallBacker* )
{
    if ( !enableimagesave_ ) return;

    uiGraphicsSaveImageDlg dlg( parent(), scene_ );
    dlg.go();
}
