/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uivolstatsattrib.cc,v 1.12 2006-11-23 12:55:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uivolstatsattrib.h"
#include "volstatsattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* outpstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	"Sum",
	"NormVariance",
	"Most Frequent",
	"RMS",
	0
};

mInitAttribUI(uiVolumeStatisticsAttrib,VolStats,"Volume Statistics",
	      sKeyStatsGrp)


uiVolumeStatisticsAttrib::uiVolumeStatisticsAttrib( uiParent* p )
    : uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, inpfld );

    shapefld = new uiGenInput( this, "Shape", BoolInpSpec("Cylinder","Cube") );
    shapefld->attach( alignedBelow, gatefld );
    
    stepoutfld = new uiStepOutSel( this, uiStepOutSel::Setup() );
    stepoutfld->valueChanged.notify(
			mCB(this,uiVolumeStatisticsAttrib,stepoutChg) );
    stepoutfld->attach( alignedBelow, shapefld );

    nrtrcsfld = new uiLabeledSpinBox( this, "Min nr of valid traces" );
    nrtrcsfld->box()->setMinValue( 1 );
    nrtrcsfld->attach( alignedBelow, stepoutfld );

    outpfld = new uiGenInput( this, "Output statistic", 
			      StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, nrtrcsfld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, outpfld );

    setHAlignObj( inpfld );
}


void uiVolumeStatisticsAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );
    shapefld->display( !yn );
}


void uiVolumeStatisticsAttrib::stepoutChg( CallBacker* )
{
    const BinID so = stepoutfld->getBinID();
    int nrtrcs = 1;
    if ( !mIsUdf(so.inl) && !mIsUdf(so.crl) )
	nrtrcs = (so.inl*2+1) * (so.crl*2+1);
    nrtrcsfld->box()->setInterval( 1, nrtrcs );
}


bool uiVolumeStatisticsAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mIfGetFloatInterval( VolStats::gateStr(), gate,
	    		 gatefld->setValue(gate) );
    mIfGetBinID( VolStats::stepoutStr(), stepout,
	         stepoutfld->setBinID(stepout) );
    mIfGetEnum( VolStats::shapeStr(), shape,
	        shapefld->setValue(shape) );
    mIfGetInt( VolStats::nrtrcsStr(), nrtrcs,
	       nrtrcsfld->box()->setValue(nrtrcs) );
    stepoutChg(0);
    return true;
}


bool uiVolumeStatisticsAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( steerfld, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiVolumeStatisticsAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mSetFloatInterval( VolStats::gateStr(), gatefld->getFInterval() );
    mSetBinID( VolStats::stepoutStr(), stepoutfld->getBinID() );
    mSetEnum( VolStats::shapeStr(), shapefld->getBoolValue() );
    mSetBool( VolStats::steeringStr(), steerfld->willSteer() );
    mSetInt( VolStats::nrtrcsStr(), nrtrcsfld->box()->getValue() );

    return true;
}


bool uiVolumeStatisticsAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    fillInp( steerfld, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}


void uiVolumeStatisticsAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, VolStats::gateStr() );
    params += EvalParam( stepoutstr, VolStats::stepoutStr() );
}
