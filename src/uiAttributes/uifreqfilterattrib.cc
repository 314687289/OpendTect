/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifreqfilterattrib.cc,v 1.16 2008-05-05 05:42:18 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uifreqfilterattrib.h"
#include "freqfilterattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiwindowfunctionsel.h"

using namespace Attrib;


static const char* winstrs[] =
{
    	"None",
	"Hamming",
	"Hanning",
	"Blackman",
	"Bartlett",
	"CosTaper5",
	"CosTaper10",
	"CosTaper20",
	0
};


static const char* typestrs[] =
{
    "LowPass",
    "HighPass",
    "BandPass",
    0
};


mInitAttribUI(uiFreqFilterAttrib,FreqFilter,"Frequency Filter",sKeyFilterGrp)


uiFreqFilterAttrib::uiFreqFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.7")

{
    inpfld = getImagInpFld();

    isfftfld = new uiGenInput( this, "Filtering method", 
			       BoolInpSpec(true,"FFT","ButterWorth") );
    isfftfld->attach( alignedBelow, inpfld );
    isfftfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,isfftSel) );
		    
    typefld = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld->attach( alignedBelow, isfftfld );
    typefld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,typeSel) );

    freqfld = new uiGenInput( this, "Min/max frequency(Hz)", 
	    FloatInpSpec().setName("Min frequency"),
	    FloatInpSpec().setName("Max frequency") );
    freqfld->setElemSzPol( uiObject::Small );
    freqfld->attach( alignedBelow, typefld );

    polesfld = new uiLabeledSpinBox( this, "Nr of poles" );
    polesfld->box()->setMinValue( 2 );
    polesfld->attach( alignedBelow, freqfld );

    winfld = new uiWindowFunctionSel( this, "Window/Taper" );
    winfld->attach( alignedBelow, polesfld );
    winfld->display (false);

    mainObject()->finaliseDone.notify( mCB(this,uiFreqFilterAttrib,finaliseCB));
    setHAlignObj( inpfld );
}


void uiFreqFilterAttrib::finaliseCB( CallBacker* )
{
    typeSel(0);
    isfftSel(0);
}


void uiFreqFilterAttrib::typeSel( CallBacker* )
{
    const int type = typefld->getIntValue();
    const bool hasmin = type==1 || type==2;
    const bool hasmax = !type || type==2;
    freqfld->setSensitive( hasmin, 0, 0 );
    freqfld->setSensitive( hasmax, 0, 1 );
}


void uiFreqFilterAttrib::isfftSel( CallBacker* )
{
    const bool isfft = isfftfld->getBoolValue();
    winfld->display( isfft );
    polesfld->display( !isfft );
}


bool uiFreqFilterAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FreqFilter::attribName()) )
	return false;

    mIfGetEnum( FreqFilter::filtertypeStr(), filtertype, 
	    	typefld->setValue(filtertype) );
    mIfGetFloat( FreqFilter::minfreqStr(), minfreq,
	    	 freqfld->setValue(minfreq,0) );
    mIfGetFloat( FreqFilter::maxfreqStr(), maxfreq,
	    	 freqfld->setValue(maxfreq,1) );
    mIfGetInt( FreqFilter::nrpolesStr(), nrpoles,
	       polesfld->box()->setValue(nrpoles) )
    mIfGetString( FreqFilter::windowStr(), window,
			    winfld->setWindowName(window) );
    mIfGetFloat( FreqFilter::paramvalStr(), variable,
			     winfld->setWindowParamValue(variable) );
    mIfGetBool( FreqFilter::isfftfilterStr(), isfftfilter, 
	    	isfftfld->setValue(isfftfilter) );

    typeSel(0);
    isfftSel(0);
    return true;
}


bool uiFreqFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiFreqFilterAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),FreqFilter::attribName()) )
	return false;

    mSetEnum( FreqFilter::filtertypeStr(), typefld->getIntValue() );
    mSetFloat( FreqFilter::minfreqStr(), freqfld->getfValue(0) );
    mSetFloat( FreqFilter::maxfreqStr(), freqfld->getfValue(1) );
    mSetInt( FreqFilter::nrpolesStr(), polesfld->box()->getValue() );
    mSetString( FreqFilter::windowStr(), winfld->windowName() );
    mSetFloat( FreqFilter::paramvalStr(), winfld->windowParamValue() );
    mSetBool( FreqFilter::isfftfilterStr(), isfftfld->getBoolValue() );

    return true;
}


bool uiFreqFilterAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    const bool needimag = isfftfld->getBoolValue();
    fillInp( needimag ? inpfld : (uiAttrSel*)inpfld, desc, 0 );
    return true;
}


void uiFreqFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const int passtype = typefld->getIntValue();
    if ( passtype != 0 )
	params += EvalParam( "Min frequency", FreqFilter::minfreqStr() );
    if ( passtype != 1 )
	params += EvalParam( "Max frequency", FreqFilter::maxfreqStr() );
    if ( !isfftfld->getBoolValue() )
	params += EvalParam( "Nr poles", FreqFilter::nrpolesStr() );
}
