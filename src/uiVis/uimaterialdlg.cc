/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.16 2008-01-25 04:49:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "uivisplanedatadisplaydragprop.h"
#include "vismaterial.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "visplanedatadisplay.h"

uiLineStyleGrp::uiLineStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,"Line style")
    , survobj_(so)
    , backup_(*so->lineStyle())
{
    field_ = new uiSelLineStyle( this, backup_, "Line style", true,
				 so->hasSpecificLineColor(), true );
    field_->changed.notify( mCB(this,uiLineStyleGrp,changedCB) );
}


void uiLineStyleGrp::changedCB( CallBacker* )
{
    survobj_->setLineStyle( field_->getStyle() );
}


bool uiLineStyleGrp::rejectOK( CallBacker* )
{
    survobj_->setLineStyle( backup_ );
    return true;
}


uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiTabStackDlg(p,uiDialog::Setup("Display properties",0,"50.0.4") )
    , survobj_(so)
    , visobj_(dynamic_cast<visBase::VisualObject*>(so))
{
    if ( survobj_->allowMaterialEdit() && visobj_->getMaterial() )
    {
	addGroup(  new uiMaterialGrp( tabstack_->tabGroup(),
			survobj_, true, true, false, false, false, true,
			survobj_->hasColor() )) ;
    }

    if ( survobj_->lineStyle() )
	addGroup( new uiLineStyleGrp( tabstack_->tabGroup(), survobj_ )  );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,so);
    if ( pdd )
	addGroup( new uiVisPlaneDataDisplayDragProp(tabstack_->tabGroup(),pdd));

    setCancelText( "" );
}


#define mFinalise( sldr, fn ) \
if ( sldr ) \
{ \
    sldr->setInterval( StepInterval<float>( 0, 100, 10 ) ); \
    sldr->setValue( material_->fn()*100 ); \
}

uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiDlgGroup(p,"Material")
    , material_(dynamic_cast<visBase::VisualObject*>(so)->getMaterial())
    , survobj_(so)
    , ambslider_(0)
    , diffslider_(0)
    , specslider_(0)
    , emisslider_(0)
    , shineslider_(0)
    , transslider_(0)
    , colinp_(0)
    , prevobj_(0)
{
    if ( so->hasColor() )
    {
	colinp_ = new uiColorInput(this,so->getColor(),"Base color");
	colinp_->colorchanged.notify( mCB(this,uiMaterialGrp,colorChangeCB) );
	colinp_->setSensitive( color );
	prevobj_ = colinp_;
    }

    createSlider( ambience, ambslider_, "Ambient reflectivity" );
    createSlider( diffusecolor, diffslider_, "Diffuse reflectivity" );
    createSlider( specularcolor, specslider_, "Specular reflectivity" );
    createSlider( emmissivecolor, emisslider_, "Emissive intensity" );
    createSlider( shininess, shineslider_, "Shininess" );
    createSlider( transparency, transslider_, "Transparency" );

    mFinalise( ambslider_, getAmbience );
    mFinalise( diffslider_, getDiffIntensity );
    mFinalise( specslider_, getSpecIntensity );
    mFinalise( emisslider_, getEmmIntensity );
    mFinalise( shineslider_, getShininess );
    mFinalise( transslider_, getTransparency );
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const char* lbltxt )
{
    if ( !domk ) return;

    uiSliderExtra* se = new uiSliderExtra( this, lbltxt );
    slider = se->sldr();
    slider->valueChanged.notify( mCB(this,uiMaterialGrp,sliderMove) );
    if ( prevobj_ ) se->attach( alignedBelow, prevobj_ );
    prevobj_ = se;
}



void uiMaterialGrp::sliderMove( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,sldr,cb)
    if ( !sldr ) return;

    if ( sldr == ambslider_ )
	material_->setAmbience( ambslider_->getValue()/100 );
    else if ( sldr == diffslider_ )
	material_->setDiffIntensity( diffslider_->getValue()/100 );
    else if ( sldr == specslider_ )
	material_->setSpecIntensity( specslider_->getValue()/100 );
    else if ( sldr == emisslider_ )
	material_->setEmmIntensity( emisslider_->getValue()/100 );
    else if ( sldr == shineslider_ )
	material_->setShininess( shineslider_->getValue()/100 );
    else if ( sldr == transslider_ )
	material_->setTransparency( transslider_->getValue()/100 );
}

void uiMaterialGrp::colorChangeCB(CallBacker*)
{ if ( colinp_ ) survobj_->setColor( colinp_->color() ); }
