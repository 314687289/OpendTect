/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifreqfilterattrib.cc,v 1.8 2006-03-10 13:34:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifreqfilterattrib.h"
#include "freqfilterattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

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


uiFreqFilterAttrib::uiFreqFilterAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    isfftfld = new uiGenInput( this, "Filtering method", 
			       BoolInpSpec("FFT","ButterWorth") );
    isfftfld->attach( alignedBelow, inpfld );
    isfftfld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,isfftSel) );
		    
    typefld = new uiGenInput( this, "Filter type", 
	    		      StringListInpSpec(typestrs) );
    typefld->attach( alignedBelow, isfftfld );
    typefld->valuechanged.notify( mCB(this,uiFreqFilterAttrib,typeSel) );

    FloatInpSpec fis;
    freqfld = new uiGenInput( this, "Min/max frequency(Hz)", fis, fis );
    freqfld->setElemSzPol( uiObject::Small );
    freqfld->setValues( 15, 50 );
    freqfld->attach( alignedBelow, typefld );

    polesfld = new uiLabeledSpinBox( this, "Nr of poles" );
    polesfld->box()->setMinValue( 2 );
    polesfld->attach( alignedBelow, freqfld );

    winfld = new uiGenInput( this, "Window/Taper", StringListInpSpec(winstrs) );
    winfld->setElemSzPol( uiObject::MedVar );
    winfld->attach( alignedBelow, freqfld );
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
    if ( !hasmin ) freqfld->setText( "", 0 );
    if ( !hasmax ) freqfld->setText( "", 1 );
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
    mIfGetEnum( FreqFilter::windowStr(), window, winfld->setValue(window) );
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
    mSetEnum( FreqFilter::windowStr(), winfld->getIntValue() );
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
