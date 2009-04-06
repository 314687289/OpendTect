/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattrdesced.cc,v 1.31 2009-04-06 09:32:24 cvsranojay Exp $";




#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uilabel.h"
#include "uisteeringsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribparam.h"
#include "attribprovider.h"
#include "iopar.h"
#include "survinfo.h"

using namespace Attrib;


const char* uiAttrDescEd::timegatestr()	    { return "Time gate"; }
const char* uiAttrDescEd::stepoutstr()	    { return "Stepout"; }
const char* uiAttrDescEd::frequencystr()    { return "Frequency"; }
const char* uiAttrDescEd::filterszstr()	    { return "Filter size"; }

const char* uiAttrDescEd::sKeyOtherGrp()	{ return "Other"; }
const char* uiAttrDescEd::sKeyBasicGrp()	{ return "Basic"; }
const char* uiAttrDescEd::sKeyFilterGrp()	{ return "Filters"; }
const char* uiAttrDescEd::sKeyFreqGrp()		{ return "Frequency"; }
const char* uiAttrDescEd::sKeyPatternGrp()	{ return "Patterns"; }
const char* uiAttrDescEd::sKeyStatsGrp()	{ return "Statistics"; }
const char* uiAttrDescEd::sKeyPositionGrp()	{ return "Positions"; }
const char* uiAttrDescEd::sKeyDipGrp()		{ return "Dip"; }



const char* uiAttrDescEd::getInputAttribName( uiAttrSel* inpfld,
					      const Desc& desc )
{
    Attrib::DescID did = inpfld->attribID();
    Attrib::Desc* attrd = desc.descSet()->getDesc(did);

    return attrd ? attrd->attribName() : "";
}


uiAttrDescEd::uiAttrDescEd( uiParent* p, bool is2d, const char* helpid )
    : uiGroup(p,"")
    , desc_(0)
    , ads_(0)
    , is2d_(is2d)
    , helpid_(helpid)
{
}


uiAttrDescEd::~uiAttrDescEd()
{
}


void uiAttrDescEd::setDesc( Attrib::Desc* desc, Attrib::DescSetMan* adsm )
{
    desc_ = desc;
    adsman_ = adsm;
    if ( desc_ )
    {
	chtr_.setVar( adsman_->unSaved() );
	setParameters( *desc );
	setInput( *desc );
	setOutput( *desc );
    }
}


void uiAttrDescEd::fillInp( uiAttrSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() )
	return;

    fld->processInput();
    const DescID attribid = fld->attribID();

    const Attrib::Desc* inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), attribid );
    else
	chtr_.setChanged( true );

    if ( !desc.setInput(inp,desc.descSet()->getDesc(attribid)) )
    {
	errmsg_ += "The suggested attribute for input"; errmsg_ += inp;
	errmsg_ += " is incompatible with the input (wrong datatype)";
    }

    mDynamicCastGet(const uiImagAttrSel*,imagfld,fld)
    if ( imagfld )
	desc.setInput( inp+1, desc.descSet()->getDesc(imagfld->imagID()) );
}


void uiAttrDescEd::fillInp( uiSteeringSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() )
	return;

    const DescID descid = fld->descID();
    const Attrib::Desc* inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), descid );
    else if ( fld->willSteer() )
	chtr_.setChanged( true );

    if ( !desc.setInput( inp, desc.descSet()->getDesc(descid) ) )
    {
	errmsg_ += "The suggested attribute for input"; errmsg_ += inp;
	errmsg_ += " is incompatible with the input (wrong datatype)";
    }
}


void uiAttrDescEd::fillInp( uiSteerCubeSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() )
	return;

    fld->processInput();
    const DescID inlid = fld->inlDipID();
    const Attrib::Desc* inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), inlid );
    else
	chtr_.setChanged( true );

    if ( !desc.setInput( inp, desc.descSet()->getDesc(inlid) ) )
    {
	errmsg_ += "The suggested attribute for input"; errmsg_ += inp;
	errmsg_ += " is incompatible with the input (wrong datatype)";
    }

    const DescID crlid = fld->crlDipID();
    inpdesc = desc.getInput( inp+1 );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), crlid );
    else
	chtr_.setChanged( true );

    desc.setInput( inp+1, desc.descSet()->getDesc(crlid) );
}


void uiAttrDescEd::fillOutput( Attrib::Desc& desc, int selout )
{
    if ( chtr_.set(desc.selectedOutput(),selout) )
	desc.selectOutput( selout );
}


uiAttrSel* uiAttrDescEd::getInpFld( bool is2d, const char* txt )
{
    uiAttrSelData asd( is2d );
    return new uiAttrSel( this, asd.attrSet(), txt, asd.attribid );
}


uiAttrSel* uiAttrDescEd::getInpFld( const uiAttrSelData& asd, const char* txt )
{
    return new uiAttrSel( this, txt, asd );
}


uiImagAttrSel* uiAttrDescEd::getImagInpFld( bool is2d )
{
    uiAttrSelData asd( is2d );
    return new uiImagAttrSel( this, 0, asd );
}


void uiAttrDescEd::putInp( uiAttrSel* inpfld, const Attrib::Desc& ad, 
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
	inpfld->setDescSet( ad.descSet() );
    else
    {
	inpfld->setDesc( inpdesc );
	inpfld->updateHistory( adsman_->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteerCubeSel* inpfld, const Attrib::Desc& ad, 
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
        inpfld->setDescSet( ad.descSet() );
    else
    {
	inpfld->setDesc( inpdesc );
	inpfld->updateHistory( adsman_->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteeringSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
	inpfld->setDescSet( ad.descSet() );
    else
	inpfld->setDesc( inpdesc );
}


BufferString uiAttrDescEd::zDepLabel( const char* pre, const char* post ) const
{
    BufferString lbl;
    char zstr[6]; strcpy( zstr, zIsTime() ? "Time" : "Depth" );
    if ( pre )
	{ lbl += pre; lbl += " "; zstr[0] = tolower( zstr[0] ); }

    lbl += zstr;
    if ( post )
	{ lbl += " "; lbl += post; }
    lbl += " "; lbl += SI().getZUnitString();
    return lbl;
}


bool uiAttrDescEd::zIsTime() const
{
    return SI().zIsTime();
}
    

const char* uiAttrDescEd::commit( Attrib::Desc* editdesc )
{
    if ( !editdesc ) editdesc = desc_;
    if ( !editdesc ) return 0;

    getParameters( *editdesc );
    errmsg_ = Provider::prepare( *editdesc );
    editdesc->updateParams();	//needed before getInput to set correct input nr
    getInput( *editdesc );
    getOutput( *editdesc );
    editdesc->updateParams();	//needed after getInput to update inputs' params
    if ( editdesc->isSatisfied() == Desc::Error )
	errmsg_ = editdesc->errMsg();

    areUIParsOK();
    return errmsg_.size() ? errmsg_.buf() : 0;
}


bool uiAttrDescEd::getOutput( Attrib::Desc& desc )
{
    desc.selectOutput( 0 );
    return true;
}



