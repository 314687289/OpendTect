/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.cc,v 1.5 2005-08-15 16:17:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiscalingattrib.h"
#include "scalingattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "uigeninput.h"
#include "uitable.h"
#include "uiattrsel.h"
#include "survinfo.h"

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
    0
};


uiScalingAttrib::uiScalingAttrib( uiParent* p )
	: uiAttrDescEd(p)
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
					       .maxrowhgt(1) );

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

    table->clearTable();

    int nrtgs = 0;
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
//	const Param* param = desc.getParam( Scaling::gateStr() );
//	mDynamicCastGet(const ParamGroup<PT>*,gateset,param)
	nrtgs = gateset->size();
    }
    
    while ( nrtgs > table->nrRows() )
	table->insertRows(0);
    
    while ( nrtgs < table->nrRows() && table->nrRows() > initnrrows )
	table->removeRow(0);

    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr());
	for ( int idx=0; idx<gateset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*gateset)[idx];
	    table->setValue( uiTable::RowCol(idx,startcol), 
		    	     param.getfValue(0) );
	    table->setValue( uiTable::RowCol(idx,stopcol), 
		    	     param.getfValue(1) );
	}
    }

    if ( desc.getParam(Scaling::factorStr()) )
    {
	mDescGetConstParamGroup(ValParam,factorset,desc,Scaling::factorStr());
	for ( int idx=0; idx< factorset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*factorset)[idx];
	    table->setValue( uiTable::RowCol(idx,factcol),
		    	     param.getfValue(0) );
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

    TypeSet<TimeGate> tgs;
    TypeSet<float> factors;
    for ( int idx=0; idx<table->nrRows(); idx++ )
    {
	int start = table->getIntValue( uiTable::RowCol(idx,startcol) );
	int stop = table->getIntValue( uiTable::RowCol(idx,stopcol) );
	if ( mIsUndefInt(start) && mIsUndefInt(stop) ) continue;
	
	tgs += TimeGate(start,stop );

	if ( statsfld->getIntValue() == 3 )
	{
	    const char* fact = table->text( uiTable::RowCol(idx,factcol) );
	    factors += fact && *fact ? atof(fact) : 1;
	}
    }

    mDescGetParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
    gateset->setSize( tgs.size() );
    for ( int idx=0; idx<tgs.size(); idx++ )
    {
	ZGateParam& zgparam = (ZGateParam&)(*gateset)[idx];
	zgparam.setValue( Interval<float>(tgs[idx].start,tgs[idx].stop) );
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
