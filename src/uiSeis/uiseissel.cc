/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseissel.cc,v 1.67 2009-02-20 10:34:06 cvshelene Exp $";

#include "uiseissel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiselsimple.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
#include "seistype.h"
#include "separstr.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seispsioprov.h"


static const char* gtSelTxt( const uiSeisSel::Setup& setup, bool forread )
{
    if ( setup.seltxt_ )
	return setup.seltxt_;

    switch ( setup.geom_ )
    {
    case Seis::Vol:
	return forread ? "Input Cube" : "Output Cube";
    case Seis::Line:
	return forread ? "Input Lines" : "Output Lines";
    default:
	return forread ? "Input Data Store" : "Output Data Store";
    }
}


static void adaptCtxt( const IOObjContext& c, const uiSeisSel::Setup& s,
			bool chgtol )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( c );

    ctxt.trglobexpr = uiSeisSelDlg::standardTranslSel( s.geom_,
	    					       ctxt.forread );

    if ( ctxt.deftransl.isEmpty() )
	ctxt.deftransl = s.geom_ == Seis::Line ? "2D" : "CBVS";
    else if ( !c.forread )
	ctxt.trglobexpr = ctxt.deftransl;
    else
    {
	FileMultiString fms( ctxt.trglobexpr );
	if ( fms.indexOf(ctxt.deftransl.buf()) < 0 )
	{
	    fms += ctxt.deftransl;
	    ctxt.trglobexpr = fms;
	}
    }

    if ( s.geom_ == Seis::Line && !ctxt.allowcnstrsabsent && chgtol )
	ctxt.allowcnstrsabsent = true;	//change required to get any 2D LineSet
}


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const uiSeisSel::Setup& s )
{
    adaptCtxt( c.ctxt, s, true );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& setup )
    : uiIOObjSelDlg(p,getDlgCtio(c,setup),"",false)
    , attrfld_(0)
    , attrlistfld_(0)
    , datatype_(0)
{
    const bool is2d = Seis::is2D( setup.geom_ );
    const bool isps = Seis::isPS( setup.geom_ );

    if ( setup.datatype_ )     datatype_ = setup.datatype_;
    allowcnstrsabsent_ = setup.allowcnstrsabsent_;
    include_ = setup.include_;

    BufferString titletxt( "Setup " );
    if ( setup.seltxt_ )
	titletxt += setup.seltxt_;
    else
	titletxt += isps ? "Data Store" : (is2d ? "Line Set" : "Cube");
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();

    if ( setup.selattr_ && is2d && !isps )
    {
	if ( selgrp_->getCtxtIOObj().ctxt.forread )
	{
	    attrfld_ = new uiGenInput(selgrp_,"Attribute",StringListInpSpec());

	    if ( selgrp_->getNameField() )
		 attrfld_->attach( alignedBelow, selgrp_->getNameField() );
	    else
		attrfld_->attach( ensureBelow, topgrp );
	}
	else
	{
	    attrfld_ = new uiGenInput( selgrp_, "Attribute" );
	    attrlistfld_ = new uiListBox( selgrp_, "Existing List" );

	    if ( selgrp_->getNameField() )
		attrlistfld_->attach( alignedBelow, selgrp_->getNameField() );
	    else
		attrlistfld_->attach( ensureBelow, topgrp );

	    attrlistfld_->selectionChanged.notify(
		    		mCB(this,uiSeisSelDlg,attrNmSel) );
	    attrfld_->attach( alignedBelow, attrlistfld_ );
	}
    }

    selgrp_->getListField()->selectionChanged.notify(
	    			mCB(this,uiSeisSelDlg,entrySel) );
    if ( !selgrp_->getCtxtIOObj().ctxt.forread && Seis::is2D(setup.geom_) )
	selgrp_->setConfirmOverwrite( false );
    entrySel(0);
    if ( attrlistfld_ )
	attrNmSel(0);
}


// Tied to order in Seis::GeomType: Vol, VolPS, Line, LinePS
static const char* rdtrglobexprs[] =
{ "CBVS`PS Cube", "CBVS`MultiCube`SEGYDirect", "2D", "CBVS`SEGYDirect" };
static const char* wrtrglobexprs[] =
{ "CBVS", "CBVS", "2D", "CBVS" };

const char* uiSeisSelDlg::standardTranslSel( Seis::GeomType geom, bool forread )
{
    const char** ges = forread ? rdtrglobexprs : wrtrglobexprs;
    return ges[ (int)geom ];
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    const IOObj* ioobj = ioObj();
    if ( !ioobj || !attrfld_ )
	return;

    SeisIOObjInfo oinf( *ioobj );
    const bool is2d = oinf.is2D();
    const bool isps = oinf.isPS();
    attrfld_->display( is2d && !isps );

    BufferStringSet nms;
    oinf.getAttribNames( nms, true, 0, getDataType(), 
	    		 allowcnstrsabsent_, include_ );

    if ( selgrp_->getCtxtIOObj().ctxt.forread )
	attrfld_->newSpec( StringListInpSpec(nms), 0 );
    else
    {
	const BufferString attrnm( attrfld_->text() );
        attrlistfld_->empty();
	attrlistfld_->addItems( nms ); 
	attrfld_->setText( attrnm );
    }
}


void uiSeisSelDlg::attrNmSel( CallBacker* )
{
    attrfld_->setText( attrlistfld_->getText() );
}


const char* uiSeisSelDlg::getDataType()
{
    if ( !datatype_ )
	return 0;

    static BufferString typekey;
    typekey.setEmpty();
    switch ( Seis::dataTypeOf(datatype_) )
    {
	case Seis::Dip:
	    typekey += sKey::Steering;
	    break;

	// TODO: support other datatypes

	default:
	    return 0;
    }

    return typekey.buf();
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( attrfld_ ) iopar.set( sKey::Attribute, attrfld_->text() );
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );
    if ( attrfld_ )
    {
	entrySel(0);
	const char* selattrnm = iopar.find( sKey::Attribute );
	if ( selattrnm ) attrfld_->setText( selattrnm );
    }
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const uiSeisSel::Setup& setup )
	: uiIOObjSel(p,c,gtSelTxt(setup,c.ctxt.forread),setup.withclear_)
	, dlgiopar(*new IOPar)
    	, setup_(setup)
{
    adaptCtxt( c.ctxt, setup_, false );
}


uiSeisSel::~uiSeisSel()
{
    delete &dlgiopar;
}


CtxtIOObj* uiSeisSel::mkCtxtIOObj( Seis::GeomType gt )
{
    return !Seis::isPS(gt) ?	mMkCtxtIOObj(SeisTrc)
	:  (Seis::is2D(gt) ?	mGetCtxtIOObj(SeisPS2D,Seis)
			   :	mGetCtxtIOObj(SeisPS3D,Seis));
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( dlgiopar );
    setAttrNm( dlgiopar.find( sKey::Attribute ) );
}


void uiSeisSel::setAttrNm( const char* nm )
{
    attrnm = nm;
    if ( attrnm.isEmpty() )
	dlgiopar.removeWithKey( sKey::Attribute );
    else
	dlgiopar.set( sKey::Attribute, nm );
    updateInput();
}


const char* uiSeisSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    LineKey lk( txt );
    curusrnm = uiIOObjSel::userNameFromKey( lk.lineName() );
    curusrnm = LineKey( curusrnm, lk.attrName() );
    return curusrnm.buf();
}


bool uiSeisSel::existingTyped() const
{
    return !is2D() || isPS() ? uiIOObjSel::existingTyped()
	 : existingUsrName( LineKey(getInput()).lineName() );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    iop.merge( dlgiopar );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    dlgiopar.merge( iop );
    attrnm = iop.find( sKey::Attribute );
}


void uiSeisSel::updateInput()
{
    BufferString ioobjkey;
    if ( ctio_.ioobj ) ioobjkey = ctio_.ioobj->key();
    setInput( LineKey(ioobjkey,attrnm) );
}


void uiSeisSel::processInput()
{
    obtainIOObj();
    attrnm = LineKey( getInput() ).attrName();
    if ( ctio_.ioobj || ctio_.ctxt.forread )
	updateInput();
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio_, setup_ );
    dlg->usePar( dlgiopar );
    return dlg;
}


uiSeisLineSel::uiSeisLineSel( uiParent* p, const char* lsnm )
    : uiCompoundParSel(p,"Line name")
    , fixedlsname_(lsnm && *lsnm)
    , lsnm_(lsnm)
{
    butPush.notify( mCB(this,uiSeisLineSel,selPush) );
}


BufferString uiSeisLineSel::getSummary() const
{
    BufferString ret( lnm_ );
    if ( !lnm_.isEmpty() )
	{ ret += " ["; ret += lsnm_; ret += "]"; }
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisLineSel::selPush( CallBacker* )
{
    BufferString newlsnm( lsnm_ );
    if ( !fixedlsname_ )
    {
	BufferStringSet lsnms;
	SeisIOObjInfo::get2DLineInfo( lsnms );
	if ( lsnms.isEmpty() )
	    mErrRet("No line sets available.\nPlease import 2D data first")

	if ( lsnms.size() == 1 )
	    newlsnm = lsnm_ = lsnms.get( 0 );
	else
	{
	    uiSelectFromList::Setup su( "Select Line Set", lsnms );
	    su.current_ = lsnm_.isEmpty() ? 0 : lsnms.indexOf( lsnm_ );
	    if ( su.current_ < 0 ) su.current_ = 0;
	    uiSelectFromList dlg( this, su );
	    if ( !dlg.go() || dlg.selection() < 0 )
		return;
	    newlsnm = lsnms.get( dlg.selection() );
	}
    }

    BufferStringSet lnms;
    SeisIOObjInfo ioinf( newlsnm );
    if ( !ioinf.isOK() )
	mErrRet("Invalid line set selected")
    if ( ioinf.isPS() || !ioinf.is2D() )
	mErrRet("Selected Line Set duplicates name with other object")

    ioinf.getLineNames( lnms );
    uiSelectFromList::Setup su( "Select 2D line", lnms );
    su.current_ = lnm_.isEmpty() ? 0 : lnms.indexOf( lnm_ );
    if ( su.current_ < 0 ) su.current_ = 0;
    uiSelectFromList dlg( this, su );
    if ( !dlg.go() || dlg.selection() < 0 )
	return;

    lsnm_ = newlsnm;
    lnm_ = lnms.get( dlg.selection() );
}
