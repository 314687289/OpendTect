/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.cc,v 1.8 2007-12-06 08:52:58 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigridlinesdlg.h"

#include "draw.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uisellinest.h"
#include "visgridlines.h"
#include "visplanedatadisplay.h"


#define mCreateGridFld( name, lbl ) \
    label = "Show"; label += " "; label += lbl; label += " "; \
    label += "grid"; \
    name##fld_ = new uiCheckBox( this, label ); \
    name##fld_->activated.notify( mCB(this,uiGridLinesDlg,showGridLineCB) ); \
    name##spacingfld_ = new uiGenInput( this, "Spacing (Start/Stop)", \
	    				IntInpIntervalSpec(true) ); \
    name##spacingfld_->attach( leftAlignedBelow, name##fld_ );

    
uiGridLinesDlg::uiGridLinesDlg( uiParent* p, visSurvey::PlaneDataDisplay* pdd )
    : uiDialog(p,uiDialog::Setup("GridLines","Set gridlines options","50.0.3"))
    , pdd_( pdd )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , zfld_( 0 )
    , inlspacingfld_( 0 )
    , crlspacingfld_( 0 )
    , zspacingfld_( 0 )
{
    BufferString label;
    CubeSampling cs( pdd->getCubeSampling(true,true) );
    if ( cs.nrInl()>1 ) 
	{ mCreateGridFld( inl, "inline" ) }
    if ( cs.nrCrl()>1 )
    	{ mCreateGridFld( crl, "crossline" ) }
    if ( cs.nrZ()>1 )
    	{ mCreateGridFld( z, "z" ) }

    if ( inlfld_ && crlfld_ )
	crlfld_->attach( leftAlignedBelow, inlspacingfld_ );

    LineStyle lst;
    pdd->gridlines()->getLineStyle( lst );
    
    lsfld_ = new uiSelLineStyle( this, lst, "Line style", true, true );
    if ( zfld_ )
    {
	zfld_->attach( leftAlignedBelow, inlfld_ ? inlspacingfld_ 
						 : crlspacingfld_ );
	lsfld_->attach( alignedBelow, zspacingfld_ );
    }
    else
	lsfld_->attach( alignedBelow, crlspacingfld_ );
    
    setParameters();
}


void uiGridLinesDlg::showGridLineCB( CallBacker* cb )
{
    if ( inlspacingfld_ )
	inlspacingfld_->display( inlfld_->isChecked() );
    
    if ( crlspacingfld_ )
	crlspacingfld_->display( crlfld_->isChecked() );
    
    if ( zspacingfld_ )
	zspacingfld_->display( zfld_->isChecked() );
    
    lsfld_->display( (inlfld_ && inlfld_->isChecked()) ||
	    	     (crlfld_ && crlfld_->isChecked()) ||
		     (zfld_ && zfld_->isChecked()) );
}


static float getDefaultStep( float width )
{
    float reqstep = width / 5;
    float step = 10000;
    while ( true )
    {
	if ( step <= reqstep )
	    return step;
	step /= 10;
    }
}


static void getDefaultHorSampling( int& start, int& stop, int& step )
{
    const float width = stop - start;
    step = mNINT( getDefaultStep(width) );

    start = step * (int)( ceil( (float)start/(float)step ) );
    stop = step * (int)( floor( (float)stop/(float)step ) );
}


static void getDefaultZSampling( StepInterval<float>& zrg )
{
    const float width = (zrg.stop-zrg.start) * SI().zFactor();
    zrg.step = getDefaultStep( width );
    zrg.start = zrg.step * 
	ceil( (float)zrg.start * SI().zFactor() / (float)zrg.step );
    zrg.stop = zrg.step * 
	floor( (float)zrg.stop * SI().zFactor() / (float)zrg.step );
}


void uiGridLinesDlg::setParameters()
{
    const bool hasgl = !pdd_->gridlines()->getGridCubeSampling().isEmpty();
    CubeSampling cs = pdd_->getCubeSampling( true, true );
    if ( hasgl )
    {
	cs = pdd_->gridlines()->getGridCubeSampling();
	cs.zrg.scale( SI().zFactor() );
    }
    else
    {
	getDefaultHorSampling( cs.hrg.start.inl, cs.hrg.stop.inl, 
			       cs.hrg.step.inl );
	getDefaultHorSampling( cs.hrg.start.crl, cs.hrg.stop.crl, 
			       cs.hrg.step.crl );
	getDefaultZSampling( cs.zrg );
    }
    
    if ( inlfld_ )
    {
	inlfld_->setChecked( pdd_->gridlines()->areInlinesShown() );
	inlspacingfld_->setValue( cs.hrg.inlRange() );
    }

    if ( crlfld_ )
    {
	crlfld_->setChecked( pdd_->gridlines()->areCrosslinesShown() );
	crlspacingfld_->setValue( cs.hrg.crlRange() );
    }

    if ( zfld_ )
    {
	zfld_->setChecked( pdd_->gridlines()->areZlinesShown() );
	zspacingfld_->setValue(
		StepInterval<int>(mNINT(cs.zrg.start),mNINT(cs.zrg.stop),
		   		  mNINT(cs.zrg.step)) );
    }

    showGridLineCB(0);
}


#define mGetHrgSampling(dir)\
    StepInterval<int> dir##intv = dir##spacingfld_->getIStepInterval();\
    cs.hrg.start.dir = dir##intv.start;\
    cs.hrg.stop.dir = dir##intv.stop;\
    cs.hrg.step.dir = dir##intv.step;\


bool uiGridLinesDlg::acceptOK( CallBacker* )
{
    visBase::GridLines& gl = *pdd_->gridlines();
    CubeSampling cs;
    if ( inlfld_ ) { mGetHrgSampling(inl) };
    if ( crlfld_ ) { mGetHrgSampling(crl) };
    if ( zfld_ )
    {
	cs.zrg.setFrom( zspacingfld_->getFStepInterval() );
	cs.zrg.scale( 1/SI().zFactor() );
    }

    gl.setGridCubeSampling( cs );
    gl.setPlaneCubeSampling( pdd_->getCubeSampling(true,true) );
    if ( inlfld_ )
	gl.showInlines( inlfld_->isChecked() );
    if ( crlfld_ )
	gl.showCrosslines( crlfld_->isChecked() );
    if ( zfld_ )
	gl.showZlines( zfld_->isChecked() );

    gl.setLineStyle( lsfld_->getStyle() );

    return true;
}


