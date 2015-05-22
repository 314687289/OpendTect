/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";




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
#include "attribstorprovider.h"
#include "attribfactory.h"
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

    return attrd ? attrd->attribName().buf() : "";
}


uiAttrDescEd::uiAttrDescEd( uiParent* p, bool is2d, const HelpKey& helpkey )
    : uiGroup(p)
    , desc_(0)
    , ads_(0)
    , is2d_(is2d)
    , helpkey_(helpkey)
    , needinpupd_(false)
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


void uiAttrDescEd::setDataPackInp( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;

    if ( desc_ )
	setInput( *desc_ );
}


void uiAttrDescEd::fillInp( uiAttrSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() || !desc.descSet() )
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
    uiString errmsg;
    if ( !fld->areParsOK(errmsg) )
    {
	errmsg_ = errmsg.getFullString();
	return;
    }

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


void uiAttrDescEd::fillInp( uiSteerAttrSel* fld, Attrib::Desc& desc, int inp )
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


uiAttrSel* uiAttrDescEd::createInpFld( bool is2d, const char* txt )
{
    uiAttrSelData asd( is2d );
    return new uiAttrSel( this, asd.attrSet(), txt, asd.attribid_ );
}


uiAttrSel* uiAttrDescEd::createInpFld( const uiAttrSelData& asd,
				       const char* txt )
{
    return new uiAttrSel( this, txt, asd );
}


uiImagAttrSel* uiAttrDescEd::createImagInpFld( bool is2d )
{
    uiAttrSelData asd( is2d );
    return new uiImagAttrSel( this, 0, asd );
}


void uiAttrDescEd::putInp( uiAttrSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    if ( dpfids_.size() )
	inpfld->setPossibleDataPacks( dpfids_ );

    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
    {
	if ( needinpupd_ )
	{
	    Attrib::DescID defaultdid = ad.descSet()->ensureDefStoredPresent();
	    Attrib::Desc* defaultdesc = ad.descSet()->getDesc( defaultdid );
	    if ( !defaultdesc )
		inpfld->setDescSet( ad.descSet() );

	    inpfld->setDesc( defaultdesc );
	}
	else
	    inpfld->setDescSet( ad.descSet() );
    }
    else
    {
	inpfld->setDesc( inpdesc );
	inpfld->updateHistory( adsman_->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteerAttrSel* inpfld, const Attrib::Desc& ad,
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
    BufferString lbl, zstr( zIsTime() ? "Time" : "Depth" );
    if ( pre )
	{ lbl += pre; lbl += " "; zstr[0] = (char)tolower(zstr[0]); }

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


const char* uiAttrDescEd::errMsgStr( Attrib::Desc* desc )
{
    if ( !desc )
	return 0;

    if ( desc->isSatisfied() == Desc::Error )
    {
	const BufferString derrmsg( desc->errMsg() );
	if ( !desc->isStored() || derrmsg != DescSet::storedIDErrStr() )
	    errmsg_ = derrmsg;
	else
	{
	    errmsg_.set( "Cannot find stored data '" ).add( desc->userRef() );
	    errmsg_.add( ".\nData might have been deleted or corrupted"
			 ".\nPlease select valid stored data as input." );
	}
    }

    const bool isuiok = areUIParsOK();
    if ( !isuiok && errmsg_.isEmpty() )
	errmsg_= "Please review your parameters, some of them are not correct";

    return errmsg_.str();
}


const char* uiAttrDescEd::commit( Attrib::Desc* editdesc )
{
    if ( !editdesc ) editdesc = desc_;
    if ( !editdesc ) return 0;

    getParameters( *editdesc );
    errmsg_ = Provider::prepare( *editdesc ).getFullString();
    editdesc->updateParams();	//needed before getInput to set correct input nr
    if ( !getInput(*editdesc) || !getOutput(*editdesc) )
	return 0;

    editdesc->updateParams();	//needed after getInput to update inputs' params

    if ( !errmsg_.isEmpty() )
	return errmsg_.buf();

    return errMsgStr( editdesc );
}


bool uiAttrDescEd::getOutput( Attrib::Desc& desc )
{
    desc.selectOutput( 0 );
    return true;
}


bool uiAttrDescEd::getInputDPID( uiAttrSel* inpfld,
				 DataPack::FullID& inpdpfid ) const
{
    LineKey lk( inpfld->getInput() );
    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	DataPack::FullID dpfid = dpfids_[idx];
	BufferString dpnm = DataPackMgr::nameOf( dpfid );
	if ( lk.lineName() == dpnm )
	{
	    inpdpfid = dpfid;
	    return true;
	}
    }

    return false;
}


Desc* uiAttrDescEd::getInputDescFromDP( uiAttrSel* inpfld ) const
{
    if ( !dpfids_.size() )
    {
	pErrMsg( "No datapacks present to form Desc" );
	return 0;
    }

    DataPack::FullID inpdpfid;
    if ( !getInputDPID(inpfld,inpdpfid) )
	return 0;


    BufferString dpidstr( "#" );
    dpidstr.add( inpdpfid.buf() );
    Desc* inpdesc = Attrib::PF().createDescCopy( StorageProvider::attribName());
    Attrib::ValParam* param =
	inpdesc->getValParam( Attrib::StorageProvider::keyStr() );
    param->setValue( dpidstr.buf() );
    return inpdesc;
}

