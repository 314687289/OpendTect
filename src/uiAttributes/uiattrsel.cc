/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrsel.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribdescsetsholder.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "hilbertattrib.h"

#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ptrman.h"
#include "seisread.h"
#include "seistrctr.h"
#include "linekey.h"
#include "cubesampling.h"
#include "separstr.h"
#include "survinfo.h"
#include "zdomain.h"
#include "datapack.h"

#include "nlamodel.h"
#include "nladesign.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "od_helpids.h"

const Attrib::DescSet& emptyads2d()
{
    mDefineStaticLocalObject( Attrib::DescSet, res, (true) );
    return res;
}

const Attrib::DescSet& emptyads3d()
{
    mDefineStaticLocalObject( Attrib::DescSet, res, (false) );
    return res;
}

using namespace Attrib;

#define mImplConstr \
    , attribid_(-1,true) \
    , nlamodel_(0) \
    , outputnr_(-1) \
    , compnr_(-1) \
    , zdomaininfo_(0) \
{ \
    if ( fillwithdef ) \
	attribid_ = attrset_->ensureDefStoredPresent(); \
}

uiAttrSelData::uiAttrSelData( bool is2d, bool fillwithdef )
    : attrset_(is2d ? &emptyads2d() : &emptyads3d() )
    mImplConstr


uiAttrSelData::uiAttrSelData( const Attrib::DescSet& aset, bool fillwithdef )
    : attrset_(&aset)
    mImplConstr


bool uiAttrSelData::is2D() const
{
    return attrSet().is2D();
}

#define mImplInitVar \
	: uiDialog(p,uiDialog::Setup("","",mODHelpKey(mAttrSelDlgNo_NNHelpID)))\
	, attrdata_(atd) \
	, selgrp_(0) \
	, storoutfld_(0) \
	, steerfld_(0) \
	, steeroutfld_(0) \
	, attroutfld_(0) \
	, attr2dfld_(0) \
	, nlafld_(0) \
	, nlaoutfld_(0) \
	, zdomainfld_(0) \
	, zdomoutfld_(0) \
	, in_action_(false) \
	, showsteerdata_(stp.showsteeringdata_) \
	, usedasinput_(stp.isinp4otherattrib_)


uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const uiAttrSelData& atd,
                            const Setup& stp )
    mImplInitVar
{
    initAndBuild( stp.seltxt_, stp.ignoreid_, usedasinput_ );
}

uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const uiAttrSelData& atd,
			    const TypeSet<DataPack::FullID>& dpfids,
			    const Setup& stp )
    mImplInitVar
{
    dpfids_ = dpfids;
    initAndBuild( stp.seltxt_, stp.ignoreid_, usedasinput_ );
}


void uiAttrSelDlg::initAndBuild( const uiString& seltxt,
				 Attrib::DescID ignoreid,
				 bool isinp4otherattrib )
{
    //TODO: steering will never be displayed: on purpose?
    attrinf_ = new SelInfo( &attrdata_.attrSet(), attrdata_.nlamodel_,
			    is2D(), ignoreid );
    if ( dpfids_.size() )
	replaceStoredByInMem();

    setCaption( uiStrings::sSelect() );

    uiString title = uiStrings::sSelect(true).arg( seltxt );
    setTitleText( title );
    setName( title.getFullString() );

    createSelectionButtons();
    createSelectionFields();

    int seltyp = 0;
    int storcur = -1, attrcur = -1, nlacur = -1;
    if ( attrdata_.nlamodel_ && attrdata_.outputnr_ >= 0 )
    {
	seltyp = 3;
	nlacur = attrdata_.outputnr_;
    }
    else
    {
	const Desc* desc = attrdata_.attribid_.isValid()
			? attrdata_.attrSet().getDesc( attrdata_.attribid_ ) :0;
	if ( desc )
	{
	    seltyp = desc->isStored() ? 0 : 2;
	    if ( seltyp == 2 )
		attrcur = attrinf_->attrnms_.indexOf( desc->userRef() );
	    else if ( storoutfld_ )
	    {
		LineKey lk( desc->userRef() );
		storcur = attrinf_->ioobjnms_.indexOf( lk.lineName() );
		//2D attrib is set in cubeSel, called from doFinalize
	    }
	}
	else
	{
	    // Defaults are the last ones added to attrib set
	    for ( int idx=attrdata_.attrSet().size()-1; idx!=-1; idx-- )
	    {
		const DescID attrid = attrdata_.attrSet().getID( idx );
		const Desc& ad = *attrdata_.attrSet().getDesc( attrid );
		if ( ad.isStored() && storcur == -1 )
		    storcur = attrinf_->ioobjnms_.indexOf( ad.userRef() );
		else if ( !ad.isStored() && attrcur == -1 )
		{
		    attrcur = attrinf_->attrnms_.indexOf( ad.userRef() );
		    seltyp = 2;
		}
		if ( storcur != -1 && attrcur != -1 ) break;
	    }
	}
    }

    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    if ( storcur == -1 )		storcur = 0;
    if ( attrcur == -1 )		attrcur = attrinf_->attrnms_.size()-1;
    if ( nlacur == -1 && havenlaouts )	nlacur = 0;

    if ( storoutfld_  )			storoutfld_->setCurrentItem( storcur );
    if ( attroutfld_ && attrcur != -1 )	attroutfld_->setCurrentItem( attrcur );
    if ( nlaoutfld_ && nlacur != -1 )	nlaoutfld_->setCurrentItem( nlacur );

    if ( seltyp == 0 )
	storfld_->setChecked(true);
    else if ( steerfld_ && seltyp == 1 )
	steerfld_->setChecked( true );
    else if ( seltyp == 2 )
	attrfld_->setChecked(true);
    else if ( nlafld_ )
	nlafld_->setChecked(true);

    preFinalise().notify( mCB( this,uiAttrSelDlg,doFinalise) );
}


uiAttrSelDlg::~uiAttrSelDlg()
{
    delete selgrp_;
    delete attrinf_;
}


void uiAttrSelDlg::doFinalise( CallBacker* )
{
    selDone(0);
    in_action_ = true;
}


void uiAttrSelDlg::createSelectionButtons()
{
    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    const bool haveattribs = attrinf_->attrnms_.size();
    const bool havestored = attrinf_->ioobjnms_.size();
    const bool havesteered = attrinf_->steernms_.size();

    selgrp_ = new uiButtonGroup( this, "Input selection", OD::Vertical );
    storfld_ = new uiRadioButton( selgrp_, "Stored" );
    storfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    storfld_->setSensitive( havestored );

    if ( showsteerdata_ )
    {
	steerfld_ = new uiRadioButton( selgrp_, "Steering" );
	steerfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
	steerfld_->setSensitive( havesteered );
    }

    attrfld_ = new uiRadioButton( selgrp_, "Attributes" );
    attrfld_->setSensitive( haveattribs );
    attrfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );

    if ( havenlaouts )
    {
	nlafld_ = new uiRadioButton( selgrp_,
				     attrdata_.nlamodel_->nlaType(false) );
	nlafld_->setSensitive( havenlaouts );
	nlafld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    }

    if ( attrdata_.zdomaininfo_ )
    {
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, nms );
	zdomainfld_ = new uiRadioButton( selgrp_,
					 attrdata_.zdomaininfo_->key() );
	zdomainfld_->setSensitive( !nms.isEmpty() );
	zdomainfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    }
}


void uiAttrSelDlg::createSelectionFields()
{
    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    const bool haveattribs = attrinf_->attrnms_.size();

    storoutfld_ = new uiListBox( this, attrinf_->ioobjnms_, "Stored output" );
    storoutfld_->setHSzPol( uiObject::Wide );
    storoutfld_->selectionChanged.notify( mCB(this,uiAttrSelDlg,cubeSel) );
    storoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
    storoutfld_->attach( rightOf, selgrp_ );

    steeroutfld_ = new uiListBox( this, attrinf_->steernms_, "Steered output" );
    steeroutfld_->selectionChanged.notify( mCB(this,uiAttrSelDlg,cubeSel) );
    steeroutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
    steeroutfld_->attach( rightOf, selgrp_ );
    steeroutfld_->attach( heightSameAs, storoutfld_ );

    filtfld_ = new uiGenInput( this, "Filter", "*" );
    filtfld_->attach( alignedBelow, storoutfld_ );
    filtfld_->valuechanged.notify( mCB(this,uiAttrSelDlg,filtChg) );
    compfld_ = new uiLabeledComboBox( this, "Component", "Compfld" );
    compfld_->attach( rightTo, filtfld_ );

    if ( haveattribs )
    {
	attroutfld_ = new uiListBox( this, attrinf_->attrnms_,
					"Output attributes" );
	attroutfld_->setHSzPol( uiObject::Wide );
	attroutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	attroutfld_->attach( rightOf, selgrp_ );
    }

    if ( havenlaouts )
    {
	nlaoutfld_ = new uiListBox( this, attrinf_->nlaoutnms_, "Output NLAs" );
	nlaoutfld_->setHSzPol( uiObject::Wide );
	nlaoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	nlaoutfld_->attach( rightOf, selgrp_ );
    }

    if ( attrdata_.zdomaininfo_ )
    {
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, nms );
	zdomoutfld_ = new uiListBox( this, nms, "ZDomain output" );
	zdomoutfld_->setHSzPol( uiObject::Wide );
	zdomoutfld_->selectionChanged.notify( mCB(this,uiAttrSelDlg,cubeSel) );
	zdomoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	zdomoutfld_->attach( rightOf, selgrp_ );
	zdomoutfld_->attach( heightSameAs, storoutfld_ );
    }
}


int uiAttrSelDlg::selType() const
{
    if ( steerfld_ && steerfld_->isChecked() )
	return 1;
    if ( attrfld_->isChecked() )
	return 2;
    if ( nlafld_ && nlafld_->isChecked() )
	return 3;
    if ( zdomainfld_ && zdomainfld_->isChecked() )
	return 4;
    return 0;
}


void uiAttrSelDlg::selDone( CallBacker* c )
{
    if ( !selgrp_ ) return;

    const int seltyp = selType();
    if ( attroutfld_ ) attroutfld_->display( seltyp == 2 );
    if ( nlaoutfld_ ) nlaoutfld_->display( seltyp == 3 );
    if ( zdomoutfld_ ) zdomoutfld_->display( seltyp == 4 );
    if ( storoutfld_ || steeroutfld_ )
    {
	if ( storoutfld_ )
	    storoutfld_->display( seltyp==0 );
	if ( steeroutfld_ )
	    steeroutfld_->display( seltyp==1 );
    }

    const bool isstoreddata = seltyp==0 || seltyp==1;
    const bool issteerdata = seltyp==1;
    filtfld_->display( isstoreddata );
    compfld_->display( issteerdata );

    cubeSel(0);
}


void uiAttrSelDlg::filtChg( CallBacker* c )
{
    if ( !storoutfld_ || !filtfld_ ) return;

    const bool issteersel = selType() == 1;
    uiListBox* outfld = issteersel ? steeroutfld_ : storoutfld_;
    BufferStringSet& nms = issteersel ? attrinf_->steernms_
				      : attrinf_->ioobjnms_;
    attrinf_->fillStored( issteersel, filtfld_->text() );
    outfld->setEmpty();
    if ( nms.isEmpty() ) return;

    outfld->addItems( nms );
    outfld->setCurrentItem( 0 );
    cubeSel( c );
}


void uiAttrSelDlg::cubeSel( CallBacker* c )
{
    if ( !storoutfld_ ) return;

    const int seltyp = selType();
    if ( seltyp==2 || seltyp==3 )
	return;

    BufferString ioobjkey;
    if ( seltyp==0 )
    {
	if ( !attrinf_->ioobjids_.isEmpty() )
	    ioobjkey = attrinf_->ioobjids_.get( storoutfld_->currentItem() );
    }
    else if ( seltyp==1 )
    {
	if ( !attrinf_->steerids_.isEmpty() )
	    ioobjkey = attrinf_->steerids_.get( steeroutfld_->currentItem() );
    }
    else if ( seltyp==4 )
    {
	const int selidx = zdomoutfld_->currentItem();
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, nms );
	if ( nms.validIdx(selidx) )
	{
	    IOM().to(
		 MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
	    PtrMan<IOObj> ioobj = IOM().getLocal( nms.get(selidx), 0 );
	    if ( ioobj ) ioobjkey = ioobj->key();
	}
    }

    const bool is2d = ioobjkey.isEmpty()
	? false : SelInfo::is2D( ioobjkey.buf() );
    const bool isstoreddata = seltyp==0 || seltyp==1;
    filtfld_->display( !is2d && isstoreddata );

    compfld_->box()->setCurrentItem(0);
    const MultiID key( ioobjkey.buf() );
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.prepareWork(Seis::PreScan) ) return;

    SeisTrcTranslator* transl = rdr.seisTranslator();
    if ( !transl ) return;
    BufferStringSet compnms;
    transl->getComponentNames( compnms );
    compfld_->box()->setEmpty();
    compfld_->box()->addItem( "ALL" );

    compfld_->box()->addItems( compnms );
    compfld_->display( compnms.size()>2 );
}


bool uiAttrSelDlg::getAttrData( bool needattrmatch )
{
    DescSet* descset = 0;
    attrdata_.attribid_ = DescID::undef();
    attrdata_.outputnr_ = -1;
    if ( !selgrp_ || !in_action_ ) return true;

    int selidx = -1;
    const int seltyp = selType();
    if ( seltyp==1 )		selidx = steeroutfld_->currentItem();
    else if ( seltyp==2 )	selidx = attroutfld_->currentItem();
    else if ( seltyp==3 )	selidx = nlaoutfld_->currentItem();
    else if ( seltyp==4 )	selidx = zdomoutfld_->currentItem();
    else if ( storoutfld_ )	selidx = storoutfld_->currentItem();
    if ( selidx < 0 )
	return false;

    if ( seltyp == 2 )
	attrdata_.attribid_ = attrinf_->attrids_[selidx];
    else if ( seltyp == 3 )
	attrdata_.outputnr_ = selidx;
    else if ( seltyp == 4 )
    {
	if ( !attrdata_.zdomaininfo_ )
	    { pErrMsg( "Huh" ); return false; }

	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, nms );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( nms.get(selidx), 0 );
	if ( !ioobj ) return false;

	LineKey linekey( ioobj->key() );
	descset = usedasinput_
		? const_cast<DescSet*>( &attrdata_.attrSet() )
		: eDSHolder().getDescSet( is2D(), true );
	attrdata_.attribid_ = descset->getStoredID( linekey, 0, true );
    }
    else
    {
	const bool canuseallcomps = BufferString(compfld_->box()->textOfItem(0))
					== BufferString("ALL")
					&& compfld_->mainObject()->visible();
	const int curselitmidx = compfld_->box()->currentItem();
	attrdata_.compnr_ = canuseallcomps ? curselitmidx -1 : curselitmidx;
	if ( attrdata_.compnr_< 0 && !canuseallcomps )
	    attrdata_.compnr_ = 0;
	const char* ioobjkey = seltyp==0 ? attrinf_->ioobjids_.get( selidx )
					 : attrinf_->steerids_.get( selidx );
	descset = usedasinput_
		? const_cast<DescSet*>( &attrdata_.attrSet() )
		: eDSHolder().getDescSet( is2D(), true );
	attrdata_.attribid_ = canuseallcomps && attrdata_.compnr_==-1
	    ? descset->getStoredID(ioobjkey, attrdata_.compnr_, true,true,"ALL")
	    : descset->getStoredID( ioobjkey, attrdata_.compnr_, true );
	if ( needattrmatch && !attrdata_.attribid_.isValid() )
	{
	    BufferString msg( "Could not find the seismic data " );
	    msg += attrdata_.attribid_ == DescID::undef() ? "in object manager"
							  : "on disk";
	    uiMSG().error( msg );
	    return false;
	}
    }

    if ( !usedasinput_ && descset )
	attrdata_.setAttrSet( descset );

    return true;
}


const char* uiAttrSelDlg::zDomainKey() const
{ return attrdata_.zdomaininfo_ ? attrdata_.zdomaininfo_->key() : ""; }


bool uiAttrSelDlg::acceptOK( CallBacker* )
{
    return getAttrData(true);
}


void uiAttrSelDlg::replaceStoredByInMem()
{
    attrinf_->ioobjnms_.erase();
    attrinf_->ioobjids_.erase();

    BufferStringSet ioobjnmscopy;
    BufferStringSet ioobjidscopy;
    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	ioobjnmscopy.add( DataPackMgr::nameOf( dpfids_[idx] ) );
	BufferString tmpstr = "#"; tmpstr += dpfids_[idx];
	ioobjidscopy.add( tmpstr );
    }

    int* sortindexes = ioobjnmscopy.getSortIndexes();
    for ( int idx=0; idx<ioobjnmscopy.size(); idx++ )
    {
	attrinf_->ioobjnms_.add( ioobjnmscopy.get(sortindexes[idx]) );
	attrinf_->ioobjids_.add( ioobjidscopy.get(sortindexes[idx]) );
    }

    delete [] sortindexes;
}


uiString uiAttrSel::cDefLabel() { return tr("Input Data"); }

uiAttrSel::uiAttrSel( uiParent* p, const DescSet& ads, const char* txt,
		      DescID curid, bool isinp4otherattrib )
    : uiIOSelect(p,uiIOSelect::Setup(txt?txt:cDefLabel()),
		 mCB(this,uiAttrSel,doSel))
    , attrdata_(ads)
    , ignoreid_(DescID::undef())
    , usedasinput_(isinp4otherattrib)
    , seltype_(-1)
{
    attrdata_.attribid_ = curid;
    updateInput();
}


uiAttrSel::uiAttrSel( uiParent* p, const char* txt, const uiAttrSelData& ad,
		      bool isinp4otherattrib )
    : uiIOSelect(p,uiIOSelect::Setup(txt?txt:cDefLabel()),
		 mCB(this,uiAttrSel,doSel))
    , attrdata_(ad)
    , ignoreid_(DescID::undef())
    , usedasinput_(isinp4otherattrib)
    , seltype_(-1)
{
    updateInput();
}


void uiAttrSel::setDescSet( const DescSet* ads )
{
    attrdata_.setAttrSet( ads );
    updateInput();
}


void uiAttrSel::setNLAModel( const NLAModel* mdl )
{
    attrdata_.nlamodel_ = mdl;
}


void uiAttrSel::setDesc( const Desc* ad )
{
    if ( !ad || !ad->descSet() ) return;
    attrdata_.setAttrSet( ad->descSet() );

    const bool isstor = ad->isStored();
    const char* inp = ad->userRef();
    if ( inp[0] == '_' || (isstor && ad->dataType() == Seis::Dip) )
	return;

    attrdata_.attribid_ = ad->id();
    updateInput();
    seltype_ = ad->isStored() ? 0 : 1;
}


void uiAttrSel::setSelSpec( const Attrib::SelSpec* selspec )
{
    if ( !selspec ) return;

    attrdata_.attribid_ = selspec->id();
    updateInput();
}


void uiAttrSel::setIgnoreDesc( const Desc* ad )
{
    ignoreid_ = ad ? ad->id() : DescID::undef();
}


void uiAttrSel::updateInput()
{
    SeparString bs( 0, ':' );
    bs += attrdata_.attribid_.asInt();
    bs += getYesNoString( attrdata_.attribid_.isStored() );
    bs += attrdata_.outputnr_;
    if ( attrdata_.compnr_ > -1 )
	bs += attrdata_.compnr_;

    setInput( bs );
}


const char* uiAttrSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    if ( *txt == '#' )
	return DataPackMgr::nameOf( DataPack::FullID(txt+1) );

    SeparString bs( txt, ':' );
    if ( bs.size() < 3 ) return "";

    const int id = toInt( bs[0] );
    const bool isstored = toBool( bs[1], true );
    const DescID attrid( id, isstored );
    const int outnr = toInt( bs[2] );

    if ( !attrid.isValid() )
    {
	if ( !attrdata_.nlamodel_ || outnr < 0 )
	    return "";
	if ( outnr >= attrdata_.nlamodel_->design().outputs.size() )
	    return "<error>";

	const char* nm = attrdata_.nlamodel_->design().outputs[outnr]->buf();
	return IOObj::isKey(nm) ? IOM().nameOf(nm) : nm;
    }

    const DescSet& descset = attrid.isStored() ?
	*eDSHolder().getDescSet( is2D(), true ) : attrdata_.attrSet();
    const Desc* ad = descset.getDesc( attrid );
    LineKey lk( ad ? ad->userRef() : "" );
    if ( is2D() && ad && ad->isStored() )
    {
	const Attrib::ValParam* param = ad
	    ? ad->getValParam( Attrib::StorageProvider::keyStr() )
	    : 0;

	BufferStringSet nms;
	int attrnr = 0;
	MultiID mid( MultiID::udf() );
	if ( param && param->getStringValue( 0 ) )
	{
	    const LineKey realkey = param->getStringValue(0);
	    mid = realkey.lineName().buf();
	    SelInfo::getAttrNames( mid, nms );
	    BufferString attrnm = realkey.attrName();
	    if ( attrnm.isEmpty() )
		attrnm = LineKey::sKeyDefAttrib();
	    attrnr = nms.indexOf( attrnm );
	}

	if ( attrnr >= 0 && attrnr < nms.size() )
	    lk.setAttrName( nms.get(attrnr) );

	usrnm_ = lk;

	//check for multi-components or prestack data
	//tricky test: look for "=" or "|ALL"
	BufferString descattrnm( ad->userRef() );
	BufferString copyofdanm( descattrnm );
	const_cast<BufferString*>(&copyofdanm)->remove( '=' );
	if ( descattrnm != copyofdanm && !mid.isUdf() )
	{
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( ioobj )
	    {
		if ( BufferString(ioobj->name()).isStartOf( descattrnm.buf()) )
		    usrnm_ = descattrnm;
		else
		{
		    usrnm_ = ioobj->name();
		    usrnm_ += "|";
		    usrnm_ += descattrnm;
		}
	    }
	}
    }
    else
	usrnm_ = lk;

    if ( ad && usrnm_ != ad->userRef() )
	const_cast<Desc*>( ad )->setUserRef( usrnm_.buf() );

    return usrnm_.buf();
}


void uiAttrSel::getHistory( const IOPar& iopar )
{
    uiIOSelect::getHistory( iopar );
    updateInput();
}


bool uiAttrSel::getRanges( CubeSampling& cs ) const
{
    if ( !attrdata_.attribid_.isValid() )
	return false;

    const Desc* desc = attrdata_.attrSet().getDesc( attrdata_.attribid_ );
    if ( !desc->isStored() ) return false;

    const ValParam* keypar =
		(ValParam*)desc->getParam( StorageProvider::keyStr() );
    const MultiID mid( keypar->getStringValue() );
    return SeisTrcTranslator::getRanges( mid, cs,
					 desc->is2D() ? getInput() : 0 );
}


void uiAttrSel::doSel( CallBacker* )
{
    uiAttrSelDlg::Setup setup( lbl_ ? lbl_->text() : cDefLabel() );
    setup.ignoreid(ignoreid_).isinp4otherattrib(usedasinput_);
    uiAttrSelDlg dlg( this, attrdata_, dpfids_, setup );
    if ( dlg.go() )
    {
	attrdata_.attribid_ = dlg.attribID();
	attrdata_.outputnr_ = dlg.outputNr();
	if ( !usedasinput_ )
	    attrdata_.setAttrSet( &dlg.getAttrSet() );
	updateInput();
	selok_ = true;
	seltype_ = dlg.selType();
    }
}


void uiAttrSel::processInput()
{
    BufferString inp = getInput();
    const DescSet& descset = usedasinput_ ? attrdata_.attrSet()
				: *eDSHolder().getDescSet( is2D(), true );
    bool useseltyp = seltype_ >= 0;
    if ( !useseltyp )
    {
	const Desc* adesc = descset.getDesc( attrdata_.attribid_ );
	if ( adesc )
	{
	    useseltyp = true;
	    seltype_ = adesc->isStored() ? 0 : 1;
	}
    }

    attrdata_.attribid_ = descset.getID( inp, true, !seltype_, useseltyp );
    if ( !attrdata_.attribid_.isValid() && !usedasinput_ )
	attrdata_.attribid_ = attrdata_.attrSet().getID( inp, true );
    attrdata_.outputnr_ = -1;
    if ( attrdata_.attribid_.isValid() && is2D() )
    {
	const char* attr2d = firstOcc( inp.buf(), '|' );
	if ( !attr2d )
	    attr2d = LineKey::sKeyDefAttrib();

	const Desc* ad = descset.getDesc( attrdata_.attribid_ );
	const Attrib::ValParam* param = ad
		    ? ad->getValParam( Attrib::StorageProvider::keyStr() )
		    : 0;

	BufferStringSet nms;
	if ( param && param->getStringValue( 0 ) )
	{
	    const MultiID mid =
		    LineKey(param->getStringValue(0)).lineName().buf();
	    SelInfo::getAttrNames( mid, nms );
	}
    }
    else if ( !attrdata_.attribid_.isValid() && attrdata_.nlamodel_ )
    {
	const BufferStringSet& outnms( attrdata_.nlamodel_->design().outputs );
	const BufferString nodenm = IOObj::isKey(inp) ? IOM().nameOf(inp)
							: inp.buf();
	for ( int idx=0; idx<outnms.size(); idx++ )
	{
	    const BufferString& outstr = *outnms[idx];
	    const char* desnm = IOObj::isKey(outstr) ? IOM().nameOf(outstr)
						     : outstr.buf();
	    if ( nodenm == desnm )
		{ attrdata_.outputnr_ = idx; break; }
	}
    }

    updateInput();
}


void uiAttrSel::fillSelSpec( SelSpec& as ) const
{
    const bool isnla =
	!attrdata_.attribid_.isValid() && attrdata_.outputnr_ >= 0;
    if ( isnla )
	as.set( 0, DescID(attrdata_.outputnr_,true), true, "" );
    else
	as.set( 0, attrdata_.attribid_, false, "" );

    if ( isnla && attrdata_.nlamodel_ )
	as.setRefFromID( *attrdata_.nlamodel_ );
    else
    {
	const DescSet& descset = as.id().isStored() ?
		*eDSHolder().getDescSet( is2D(), true ) : attrdata_.attrSet();
	as.setRefFromID( descset );
    }

    if ( is2D() )
	as.set2DFlag();
}


const char* uiAttrSel::getAttrName() const
{
    mDeclStaticString( ret );

    ret = getInput();
    if ( is2D() )
    {
	const Desc* ad = attrdata_.attrSet().getDesc( attrdata_.attribid_ );
	if ( (ad && ad->isStored()) || ret.contains('|') )
	    ret = LineKey( ret ).attrName();
    }

    return ret.buf();
}


bool uiAttrSel::checkOutput( const IOObj& ioobj ) const
{
    if ( !attrdata_.attribid_.isValid() && attrdata_.outputnr_ < 0 )
    {
	uiMSG().error( tr("Please select the input") );
	return false;
    }

    if ( is2D() && !SeisTrcTranslator::is2D(ioobj) )
    {
	uiMSG().error( tr("Can only store this in a 2D line set") );
	return false;
    }

    //TODO check cyclic dependencies and bad stored IDs
    return true;
}


void uiAttrSel::setObjectName( const char* nm )
{
    inp_->setName( nm );
    const char* butnm = selbut_->name();
    selbut_->setName( BufferString(butnm," ",nm) );
}


void uiAttrSel::setPossibleDataPacks( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;

    //make sure the default stored data is not used
    BufferString str( toString(attribID().asInt()) );
    if ( *str == '#') return; //TODO what on earth is this???

    const Attrib::Desc* inpdesc = getAttrSet().getDesc( attribID() );
    if ( !inpdesc || inpdesc->isStored() )
    {
	Attrib::SelSpec* tmpss = new Attrib::SelSpec(0,Attrib::DescID::undef());
        setSelSpec( tmpss );	//only to reset attrdata_.attribid_=-1
	delete tmpss;
    }

    //use the first fid as default data
    BufferString fidstr = "#"; fidstr += ids[0];
    attrdata_.attribid_ = const_cast<Attrib::DescSet*>(&getAttrSet())
					->getStoredID( fidstr.buf(), 0, true );
    updateInput();
}


// **** uiImagAttrSel ****

DescID uiImagAttrSel::imagID() const
{
    const DescID selattrid = attribID();
    TypeSet<DescID> attribids;
    attrdata_.attrSet().getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* desc = attrdata_.attrSet().getDesc( attribids[idx] );

	if ( desc->attribName() != Hilbert::attribName() )
	    continue;

	const Desc* inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id() != selattrid )
	    continue;

	return attribids[idx];
    }

    DescSet& descset = const_cast<DescSet&>(attrdata_.attrSet());
    Desc* inpdesc = descset.getDesc( selattrid );
    Desc* newdesc = PF().createDescCopy( Hilbert::attribName() );
    if ( !newdesc || !inpdesc ) return DescID::undef();

    newdesc->selectOutput( 0 );
    newdesc->setInput( 0, inpdesc );
    newdesc->setHidden( true );

    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += "_imag";
    newdesc->setUserRef( usrref );

    return descset.addDesc( newdesc );
}
