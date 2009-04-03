/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivolstatsattrib.cc,v 1.22 2009-04-03 14:59:02 cvshelene Exp $";

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

static const char* shapestrs[] =
{
	"Cube",
	"Cylinder",
	"Optical stack",
	0
};

mInitAttribUI(uiVolumeStatisticsAttrib,VolStats,"Volume Statistics",
	      sKeyStatsGrp)


uiVolumeStatisticsAttrib::uiVolumeStatisticsAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.16")
 
{
    inpfld_ = getInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(), 
	    		       FloatInpIntervalSpec().setName("Z start",0)
						     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    shapefld_ = new uiGenInput( this, "Shape", StringListInpSpec(shapestrs) );
    shapefld_->valuechanged.notify(mCB(this,uiVolumeStatisticsAttrib,shapeChg));
    shapefld_->attach( alignedBelow, gatefld_ );
    shapefld_->display( !is2d_ );
    
    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->setFieldNames( "Inl Stepout", "Crl Stepout" );
    stepoutfld_->valueChanged.notify(
			mCB(this,uiVolumeStatisticsAttrib,stepoutChg) );
    stepoutfld_->attach( alignedBelow, shapefld_ );

    optstackstepfld_ = new uiLabeledSpinBox( this, "Stack stepout" );
    optstackstepfld_->box()->setMinValue( 1 );
    optstackstepfld_->box()->valueChanged.notify(
			mCB(this,uiVolumeStatisticsAttrib,stackstepChg) );
    optstackstepfld_->attach( alignedBelow, shapefld_ );

    stackdirfld_ = new uiGenInput( this, "Direction",
	    			   BoolInpSpec( true, "Perpendicular", "Line"));
    stackdirfld_->attach( rightTo,optstackstepfld_ );

    nrtrcsfld_ = new uiLabeledSpinBox( this, "Min nr of valid traces" );
    nrtrcsfld_->box()->setMinValue( 1 );
    nrtrcsfld_->attach( alignedBelow, stepoutfld_ );

    outpfld_ = new uiGenInput( this, "Output statistic", 
			       StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, nrtrcsfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d );
    steerfld_->attach( alignedBelow, outpfld_ );

    setHAlignObj( inpfld_ );
    shapeChg(0);
}


void uiVolumeStatisticsAttrib::stepoutChg( CallBacker* )
{
    const BinID so = stepoutfld_->getBinID();
    int nrtrcs = 1;
    if ( !mIsUdf(so.inl) && !mIsUdf(so.crl) )
	nrtrcs = (so.inl*2+1) * (so.crl*2+1);
    nrtrcsfld_->box()->setInterval( 1, nrtrcs );
}


void uiVolumeStatisticsAttrib::stackstepChg( CallBacker* )
{
    nrtrcsfld_->box()->setInterval( 1, optstackstepfld_->box()->getValue() );
}


bool uiVolumeStatisticsAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mIfGetFloatInterval( VolStats::gateStr(), gate,
	    		 gatefld_->setValue(gate) );
    mIfGetBinID( VolStats::stepoutStr(), stepout,
	         stepoutfld_->setBinID(stepout) );
    mIfGetEnum( VolStats::shapeStr(), shape,
	        shapefld_->setValue(shape) );
    mIfGetInt( VolStats::nrtrcsStr(), nrtrcs,
	       nrtrcsfld_->box()->setValue(nrtrcs) );
    mIfGetEnum( VolStats::optstackdirStr(), dir,
	        stackdirfld_->setValue(dir) );
    mIfGetInt( VolStats::optstackstepStr(), stackstep,
	       optstackstepfld_->box()->setValue(stackstep) );
    stepoutChg(0);
    shapeChg(0);
    return true;
}


bool uiVolumeStatisticsAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::setOutput( const Desc& desc )
{
    outpfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiVolumeStatisticsAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),VolStats::attribName()) )
	return false;

    mSetFloatInterval( VolStats::gateStr(), gatefld_->getFInterval() );
    mSetBinID( VolStats::stepoutStr(), stepoutfld_->getBinID() );
    mSetEnum( VolStats::shapeStr(), shapefld_->getIntValue() );
    mSetBool( VolStats::steeringStr(), steerfld_->willSteer() );
    mSetInt( VolStats::nrtrcsStr(), nrtrcsfld_->box()->getValue() );
    mSetInt( VolStats::optstackstepStr(), optstackstepfld_->box()->getValue() );
    mSetEnum( VolStats::optstackdirStr(), stackdirfld_->getBoolValue() );

    return true;
}


bool uiVolumeStatisticsAttrib::getInput( Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}


bool uiVolumeStatisticsAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld_->getIntValue() );
    return true;
}


void uiVolumeStatisticsAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, VolStats::gateStr() );
    params += EvalParam( stepoutstr, VolStats::stepoutStr() );
}


void uiVolumeStatisticsAttrib::shapeChg( CallBacker* )
{
    const int shapeidx = shapefld_->getIntValue();
    stepoutfld_->display( shapeidx<2 );
    optstackstepfld_->display( shapeidx>1 );
    stackdirfld_->display( shapeidx>1 );
}


