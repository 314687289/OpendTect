/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.cc,v 1.16 2008-09-30 12:34:18 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiscalingattrib.h"
#include "scalingattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uitable.h"

using namespace Attrib;

static const int initnrrows = 5;
static const int startcol = 0;
static const int stopcol = 1;
static const int factcol = 2;


static const char* statstypestr[] =
{
    "RMS",
    "Mean",
    "Max",
    "User-defined",
    0
};


static const char* scalingtypestr[] =
{
    "T^n",
    "Window",
    "AGC",
    0
};

mInitAttribUI(uiScalingAttrib,Scaling,"Scaling",sKeyBasicGrp)


uiScalingAttrib::uiScalingAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.12")

{
    inpfld = getInpFld();

    typefld = new uiGenInput( this, "Type", StringListInpSpec(scalingtypestr) );
    typefld->valuechanged.notify( mCB(this,uiScalingAttrib,typeSel) );
    typefld->attach( alignedBelow, inpfld );

    nfld = new uiGenInput( this, "n", FloatInpSpec() );
    nfld->attach( alignedBelow, typefld );

    statsfld = new uiGenInput( this, "Basis", StringListInpSpec(statstypestr) );
    statsfld->attach( alignedBelow, typefld );
    statsfld->valuechanged.notify( mCB(this,uiScalingAttrib,statsSel) );

    table = new uiTable( this, uiTable::Setup().rowdesc("Gate")
					       .rowgrow(true)
					       .defrowlbl("")
					       .fillcol(true)
					       .maxrowhgt(1)
	   				       ,"Define Gate limits" );

    BufferString lblstart = "Start "; lblstart += SI().getZUnit();
    BufferString lblstop = "Stop "; lblstop += SI().getZUnit();
    const char* collbls[] = { lblstart.buf(), lblstop.buf(), "Scale value", 0 };
    table->setColumnLabels( collbls );
    table->setNrRows( initnrrows );
    table->setColumnStretchable( startcol, true );
    table->setColumnStretchable( stopcol, true );
    table->attach( alignedBelow, statsfld );
    table->setStretch( 2, 0 );
    table->setToolTip( "Right-click to add, insert or remove a gate" );

    // for AGC
    BufferString label = "Window width ";
    label += SI().getZUnit( true );
    windowfld = new uiGenInput( this, label.buf(), FloatInpSpec() );
    windowfld->attach( alignedBelow, typefld );

    lowenergymute = new uiGenInput( this, "Low energy mute (%)",
	    			    FloatInpSpec() );
    lowenergymute->attach( alignedBelow, windowfld );
    lowenergymute->setValue( 0 );

    typeSel(0);
    statsSel(0);

    setHAlignObj( inpfld );
}


void uiScalingAttrib::typeSel( CallBacker* )
{
    const int typeval = typefld->getIntValue();
    nfld->display( !typeval );

    statsfld->display( typeval==1 );
    table->display( typeval==1 );

    windowfld->display( typeval==2);
    lowenergymute->display( typeval==2);
}


void uiScalingAttrib::statsSel( CallBacker* )
{
    const int statstype = statsfld->getIntValue();
    table->hideColumn( 2, statstype!=3 );
}


bool uiScalingAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Scaling::attribName()) )
	return false;

    mIfGetEnum( Scaling::scalingTypeStr(), scalingtype,
	        typefld->setValue(scalingtype) );
    mIfGetFloat( Scaling::powervalStr(), powerval, nfld->setValue(powerval) );
    mIfGetEnum( Scaling::statsTypeStr(), statstype, 
	    			statsfld->setValue(statstype) );
    mIfGetFloat( Scaling::widthStr(), wndwidthval, 
	    			windowfld->setValue(wndwidthval) );
    mIfGetFloat( Scaling::mutefractionStr(), mutefactor,
				lowenergymute->setValue(mutefactor) );    

    table->clearTable();

    int nrtgs = 0;
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
	nrtgs = gateset->size();
    }
    
    while ( nrtgs > table->nrRows() )
	table->insertRows( 0, 1 );
    
    while ( nrtgs < table->nrRows() && table->nrRows() > initnrrows )
	table->removeRow( 0 );

    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr());
	for ( int idx=0; idx<gateset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*gateset)[idx];
	    table->setValue( RowCol(idx,startcol), param.getfValue(0) );
	    table->setValue( RowCol(idx,stopcol), param.getfValue(1) );
	}
    }

    if ( desc.getParam(Scaling::factorStr()) )
    {
	mDescGetConstParamGroup(ValParam,factorset,desc,Scaling::factorStr());
	for ( int idx=0; idx< factorset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*factorset)[idx];
	    table->setValue( RowCol(idx,factcol), param.getfValue(0) );
	}
    }

    typeSel(0);
    statsSel(0);
    return true;
}


bool uiScalingAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiScalingAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Scaling::attribName()) )
	return false;

    mSetEnum( Scaling::scalingTypeStr(), typefld->getIntValue() );
    mSetFloat( Scaling::powervalStr(), nfld->getfValue() );
    mSetEnum( Scaling::statsTypeStr(), statsfld->getIntValue() );
    mSetFloat( Scaling::widthStr(), windowfld->getfValue() );
    mSetFloat( Scaling::mutefractionStr(), lowenergymute->getfValue() );

    TypeSet<ZGate> tgs;
    TypeSet<float> factors;
    for ( int idx=0; idx<table->nrRows(); idx++ )
    {
	int start = table->getIntValue( RowCol(idx,startcol) );
	int stop = table->getIntValue( RowCol(idx,stopcol) );
	if ( mIsUdf(start) && mIsUdf(stop) ) continue;
	
	tgs += ZGate( start, stop );

	if ( statsfld->getIntValue() == 3 )
	{
	    const char* fact = table->text( RowCol(idx,factcol) );
	    factors += fact && *fact ? atof(fact) : 1;
	}
    }

    mDescGetParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
    gateset->setSize( tgs.size() );
    for ( int idx=0; idx<tgs.size(); idx++ )
    {
	ZGateParam& zgparam = (ZGateParam&)(*gateset)[idx];
	zgparam.setValue( tgs[idx] );
    }

    mDescGetParamGroup(ValParam,factorset,desc,Scaling::factorStr())
    factorset->setSize( factors.size() );
    for ( int idx=0; idx<factors.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*factorset)[idx];
	valparam.setValue(factors[idx] );
    }
    
    return true;
}


bool uiScalingAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}
