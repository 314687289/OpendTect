/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2004
 RCS:           $Id: uispecdecompattrib.cc,v 1.10 2006-04-03 13:30:32 cvshelene Exp $
________________________________________________________________________

-*/

#include "uispecdecompattrib.h"
#include "specdecompattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "wavelettrans.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uilabel.h"

using namespace Attrib;

static float sDefaultFreqStep = 5;

uiSpecDecompAttrib::uiSpecDecompAttrib( uiParent* p )
    : uiAttrDescEd(p)
    , nyqfreq(0)
    , nrsamples(0)
    , ds(0)
{
    inpfld = getImagInpFld();
    inpfld->selectiondone.notify( mCB(this,uiSpecDecompAttrib,inputSel) );

    typefld = new uiGenInput( this, "Transform type", BoolInpSpec("FFT","CWT"));
    typefld->attach( alignedBelow, inpfld );
    typefld->valuechanged.notify( mCB(this,uiSpecDecompAttrib,typeSel) );

    gatefld = new uiGenInput( this, gateLabel(), DoubleInpIntervalSpec() );
    gatefld->attach( alignedBelow, typefld );

    BufferString lbl( "Output frequency (" );
    lbl += zIsTime() ? "Hz" : "cycles/mm"; lbl += ")";
    outpfld = new uiLabeledSpinBox( this, lbl, 1 );
    outpfld->attach( alignedBelow, gatefld );
    outpfld->box()->doSnap( true );

    stepfld = new uiLabeledSpinBox( this, "step", 1 );
    stepfld->attach( rightTo, outpfld );
    stepfld->box()->valueChanged.notify( 
	    			mCB(this,uiSpecDecompAttrib,stepChg) );

    waveletfld = new uiGenInput( this, "Wavelet", 
	    			 StringListInpSpec(CWT::WaveletTypeNames) );
    waveletfld->attach( alignedBelow, typefld );

    stepChg(0);
    typeSel(0);
    setHAlignObj( inpfld );
}


void uiSpecDecompAttrib::inputSel( CallBacker* )
{
    if ( !*inpfld->getInput() ) return;

    CubeSampling cs;
    if ( !inpfld->getRanges(cs) )
	cs.init(true);

    ds = cs.zrg.step;
    int ns = (int)((cs.zrg.stop-cs.zrg.start)/ds + .5) + 1;
    int temp = 2;
    while ( temp  < ns ) temp *= 2;
    nrsamples = temp;
    nyqfreq = 0.5 / ds;

    const float freqscale = zIsTime() ? 1 : 1000;
    outpfld->box()->setMaxValue( nyqfreq*freqscale );
    stepfld->box()->setInterval( (float)1, nyqfreq*freqscale );
    stepfld->box()->setStep( (float)0.5, true );
}


void uiSpecDecompAttrib::typeSel( CallBacker* )
{
    bool usefft = typefld->getBoolValue();
    gatefld->display( usefft );
    waveletfld->display( !usefft );
}


void uiSpecDecompAttrib::stepChg( CallBacker* )
{
    outpfld->box()->setStep( stepfld->box()->getFValue(), true );
}


int uiSpecDecompAttrib::getOutputIdx( float outval ) const
{
    const float step = stepfld->box()->getFValue();
    return mNINT(outval/step);
}


float uiSpecDecompAttrib::getOutputValue( int idx ) const
{
    const float step = stepfld->box()->getFValue();
    return float(idx*step);
}


bool uiSpecDecompAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),SpecDecomp::attribName()) )
	return false;

    mIfGetFloatInterval( SpecDecomp::gateStr(), gate, gatefld->setValue(gate) );
    mIfGetEnum( SpecDecomp::transformTypeStr(), transformtype,
		typefld->setValue(transformtype==0) );
    mIfGetEnum( SpecDecomp::cwtwaveletStr(), cwtwavelet,
	        waveletfld->setValue(cwtwavelet) );

    const float freqscale = zIsTime() ? 1 : 1000;
    mIfGetFloat( SpecDecomp::deltafreqStr(), deltafreq,
		 stepfld->box()->setValue(deltafreq*freqscale) );

    stepChg(0);
    typeSel(0);
    return true;
}


bool uiSpecDecompAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    inputSel(0);
    return true;
}


bool uiSpecDecompAttrib::setOutput( const Desc& desc )
{
    const float freq = getOutputValue( desc.selectedOutput() );
    outpfld->box()->setValue( freq );
    return true;
}


bool uiSpecDecompAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),SpecDecomp::attribName()) )
	return false;

    mSetEnum( SpecDecomp::transformTypeStr(), typefld->getBoolValue() ? 0 : 2 );
    mSetFloatInterval( SpecDecomp::gateStr(), gatefld->getFInterval() );
    mSetEnum( SpecDecomp::cwtwaveletStr(), waveletfld->getIntValue() );

    const float freqscale = zIsTime() ? 1 : 1000;
    mSetFloat( SpecDecomp::deltafreqStr(), 
	       stepfld->box()->getFValue()/freqscale );

    return true;
}


bool uiSpecDecompAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiSpecDecompAttrib::getOutput( Desc& desc )
{
    const int freqidx = getOutputIdx( outpfld->box()->getFValue() );
    fillOutput( desc, freqidx );
    return true;
}


void uiSpecDecompAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    EvalParam ep( "Frequency" ); ep.evaloutput_ = true;
    params += ep;
    if ( typefld->getBoolValue() )
	params += EvalParam( timegatestr, SpecDecomp::gateStr() );
}
