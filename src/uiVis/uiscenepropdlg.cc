/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          February 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiscenepropdlg.cc,v 1.8 2009-01-09 09:17:26 cvssatyaki Exp $";

#include "uiscenepropdlg.h"

#include "vissurvscene.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uislider.h"
#include "uisoviewer.h"
#include "uivispartserv.h"

bool uiScenePropertyDlg::savestatus = true;


uiScenePropertyDlg::uiScenePropertyDlg( uiParent* p,
					visSurvey::Scene* scene,
					uiSoViewer* viewer,
					const ObjectSet<uiSoViewer>& viewers,
					uiVisPartServer* visserv )
    : uiDialog(p, uiDialog::Setup("Scene","Set scene options","50.0.5") )
    , scene_( scene )
    , hadsurveybox_( scene->isAnnotShown() )
    , hadannot_( scene->isAnnotTextShown() )
    , hadannotscale_( scene->isAnnotScaleShown() )
    , oldbgcolor_( viewer->getBackgroundColor() )
    , annotcolor_( scene->getAnnotColor() )
    , oldmarkersize_( scene->getMarkerSize() )
    , oldmarkercolor_( scene->getMarkerColor() )
    , markersizefld_( 0 )
    , annotfld_( 0 )
    , annotscalefld_( 0 )
    , survboxfld_( 0 )
    , bgcolfld_( 0 )
    , viewer_( viewer )
    , viewers_( viewers )
    , visserv_( visserv )
{
    enableSaveButton( "Apply to all scenes");
    setSaveButtonChecked( savestatus );

    survboxfld_ = new uiCheckBox( this, "Survey box" );
    survboxfld_->setChecked( scene->isAnnotShown() );
    survboxfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    annotfld_ = new uiCheckBox( this, "Annotation text" );
    annotfld_->setChecked( scene->isAnnotTextShown() );
    annotfld_->attach( alignedBelow, survboxfld_ );
    annotfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotfld_->setSensitive( survboxfld_->isChecked() );

    annotscalefld_ = new uiCheckBox( this, "Annotation scale" );
    annotscalefld_->setChecked( scene->isAnnotScaleShown() );
    annotscalefld_->attach( alignedBelow, annotfld_ );
    annotscalefld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );

    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup( oldbgcolor_)
					.lbltxt("Background color") );
    bgcolfld_->attach( alignedBelow, annotscalefld_ );
    bgcolfld_->colorchanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    markersizefld_ = new uiSliderExtra( this,
				uiSliderExtra::Setup("Mouse marker size"),
				"Marker size slider" );
    markersizefld_->sldr()->setMinValue( 1 );
    markersizefld_->sldr()->setMaxValue( 10 );
    markersizefld_->sldr()->setValue( scene->getMarkerSize() );
    markersizefld_->attach( alignedBelow, bgcolfld_ );
    markersizefld_->sldr()->valueChanged.notify(
	    				mCB(this,uiScenePropertyDlg,updateCB) );

    markercolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(oldmarkercolor_).lbltxt("Mouse marker color") );
    markercolfld_->attach( alignedBelow, markersizefld_ );
    markercolfld_->colorchanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    
    annotcolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(annotcolor_).lbltxt("Annotation color") );
    annotcolfld_->attach( alignedBelow, markercolfld_ );
    annotcolfld_->colorchanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );
}


void uiScenePropertyDlg::updateCB( CallBacker* cb )
{
    updateScene( scene_ );
    viewer_->setBackgroundColor( bgcolfld_->color() );
    annotfld_->setSensitive( survboxfld_->isChecked() );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );
}


void uiScenePropertyDlg::updateScene( visSurvey::Scene* scene )
{
    scene->showAnnot( survboxfld_->isChecked() );
    scene->showAnnotScale( annotscalefld_->isChecked() );
    scene->showAnnotText( annotfld_->isChecked() );
    scene->setMarkerSize( markersizefld_->sldr()->getValue() );
    scene->setMarkerColor( markercolfld_->color() );
    scene->setAnnotColor( annotcolfld_->color() );
}


bool uiScenePropertyDlg::rejectOK( CallBacker* )
{
    scene_->showAnnot( hadsurveybox_ );
    scene_->showAnnotScale( hadannot_ );
    scene_->showAnnotText( hadannotscale_ );
    scene_->setMarkerSize( oldmarkersize_ );
    scene_->setMarkerColor( oldmarkercolor_ );
    viewer_->setBackgroundColor( oldbgcolor_ );
    scene_->setAnnotColor( annotcolor_ );
    return true;
}


bool uiScenePropertyDlg::acceptOK( CallBacker* )
{
    savestatus = saveButtonChecked();
    if ( !savestatus )
	return true;

    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::Scene*, scene,
			 visserv_->getObject(sceneids[idx]) );
	updateScene( scene );
    }

    for ( int idx=0; idx<viewers_.size(); idx++ )
	const_cast<uiSoViewer*>(viewers_[idx])->setBackgroundColor(
					bgcolfld_->color() );

    return true;
}
