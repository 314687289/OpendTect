/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimanprops.h"

#include "mathexpression.h"
#include "mathproperty.h"
#include "separstr.h"
#include "uibuildlistfromlist.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "unitofmeasure.h"


class uiBuildPROPS : public uiBuildListFromList
{
public:
			uiBuildPROPS(uiParent*,PropertyRefSet&,bool);

    PropertyRefSet& 	props_;
    bool		allowmath_;

    virtual const char*	avFromDef(const char*) const;
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual void	itemSwitch(const char*,const char*);
    void		initGrp(CallBacker*);

};


uiBuildPROPS::uiBuildPROPS( uiParent* p, PropertyRefSet& prs, bool allowmath )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"property type","usable property")
	    .withio(false).withtitles(true), "PropertyRef selection group")
    , props_(prs)
    , allowmath_(allowmath)
{
    const BufferStringSet dispnms( PropertyRef::StdTypeNames() );

    setAvailable( dispnms );

    BufferStringSet pnms;
    for ( int idx=0; idx<props_.size(); idx++ )
	pnms.add( props_[idx]->name() );
    pnms.sort();
    for ( int idx=0; idx<pnms.size(); idx++ )
	addItem( pnms.get(idx) );

    postFinalise().notify( mCB(this,uiBuildPROPS,initGrp) );
}

void uiBuildPROPS::initGrp( CallBacker* )
{
    if ( !props_.isEmpty() )
	setCurDefSel( props_[0]->name() );
}

const char* uiBuildPROPS::avFromDef( const char* nm ) const
{
    const PropertyRef* pr = props_.find( nm );
    if ( !pr ) return 0;
    return PropertyRef::toString( pr->stdType() );
}


class uiEditPropRef : public uiDialog
{
public:

	    		uiEditPropRef(uiParent*,PropertyRef&,bool,bool);
    bool		acceptOK(CallBacker*);

    PropertyRef&	pr_;
    const bool		withform_;

    uiGenInput*		namefld_;
    uiGenInput*		aliasfld_;
    uiColorInput*	colfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;
    uiGenInput*		deffld_;

    const UnitOfMeasure* curunit_;

    void		setForm(CallBacker*);
    void		unitSel(CallBacker*);

};


uiEditPropRef::uiEditPropRef( uiParent* p, PropertyRef& pr, bool isadd,
       				bool supportform )
    : uiDialog(p,uiDialog::Setup("Property definition",
		BufferString(isadd?"Add '":"Edit '",
		    PropertyRef::toString(pr.stdType()),"' property"),
		"110.1.1"))
    , pr_(pr)
    , withform_(supportform)
    , curunit_(0)
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec(pr.name()) );
    SeparString ss;
    for ( int idx=0; idx<pr_.aliases().size(); idx++ )
	ss += pr_.aliases().get(idx);
    aliasfld_ = new uiGenInput( this, "Aliases (e.g. 'abc, uvw*xyz')",
	   			StringInpSpec(ss.buf()) );
    aliasfld_->attach( alignedBelow, namefld_ );

    colfld_ = new uiColorInput( this, uiColorInput::Setup(pr_.disp_.color_)
	    				.lbltxt("Default display color") );
    colfld_->attach( alignedBelow, aliasfld_ );
    rgfld_ = new uiGenInput( this, "Typical value range",
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, colfld_ );
    unfld_ = new uiUnitSel( this, pr_.stdType() );
    unfld_->setUnit( pr_.disp_.unit_ );
    unfld_->attach( rightOf, rgfld_ );
    unfld_->selChange.notify( mCB(this,uiEditPropRef,unitSel) );
    curunit_ = unfld_->getUnit();

    Interval<float> vintv( pr_.disp_.range_ );
    if ( curunit_ )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = curunit_->getUserValueFromSI( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = curunit_->getUserValueFromSI( vintv.stop );
    }
    rgfld_->setValue( vintv );

    deffld_ = new uiGenInput( this, "[Default value (if supported)]" );
    deffld_->attach( alignedBelow, rgfld_ );
    if ( pr_.disp_.defval_ )
	deffld_->setText( pr_.disp_.defval_->def() );
    if ( withform_ )
    {
	uiPushButton* but = new uiPushButton( this, "&Formula",
				mCB(this,uiEditPropRef,setForm), false );
	but->attach( rightOf, deffld_ );
    }
}


void uiEditPropRef::unitSel( CallBacker* )
{
    const UnitOfMeasure* newun = unfld_->getUnit();
    if ( newun == curunit_ )
	return;

    Interval<double> vintv( rgfld_->getDInterval() );
    convUserValue( vintv.start, curunit_, newun );
    convUserValue( vintv.stop, curunit_, newun );
    rgfld_->setValue( vintv );

    curunit_ = newun;
}

static const int cMaxNrVars = 6;
class uiEditPropRefMathDef : public uiDialog
{
public:

uiEditPropRefMathDef( uiParent* p, const PropertyRef& pr,
			const UnitOfMeasure* un )
    : uiDialog(p,Setup(BufferString("Set ",pr.name()," default to formula"),
		mNoDlgTitle,"110.0.6") )
    , pr_(pr)
{
    uiMathExpression::Setup mesu( "Formula" ); mesu.withsetbut( true );
    formfld_ = new uiMathExpression( this, mesu );
    formfld_->formSet.notify( mCB(this,uiEditPropRefMathDef,formSet) );
    FileMultiString defstr( pr_.disp_.defval_ ? pr_.disp_.defval_->def() : "" );
    BufferString curdef = defstr[0];
    if ( !pr_.disp_.defval_ )
    {
	float val = pr_.disp_.range_.center();
	if ( un )
	    val = un->getUserValueFromSI( val );
	curdef = val;
    }
    formfld_->setText( curdef );
    uiToolButtonSetup tbsu( "rockphys", "Choose rockphysics formula",
	    mCB(this,uiEditPropRefMathDef,rockPhysReq), "RockPhysics");
    formfld_->addButton( tbsu );

    for ( int idx=0; idx<=cMaxNrVars; idx++ )
    {
	BufferString lbl;
	if ( idx == cMaxNrVars )
	    lbl = "Choose formula output unit";
	uiLabeledComboBox* unfld = new uiLabeledComboBox( this, lbl );
	const ObjectSet<const UnitOfMeasure>& alluom( UoMR().all() );
	unfld->box()->addItem( "-" );
	for ( int iduom=0; iduom<alluom.size(); iduom++ )
	    unfld->box()->addItem( alluom[iduom]->name() );

	unfld->attach( alignedBelow, idx ? (uiGroup*)unflds_[idx-1]
					 : (uiGroup*)formfld_ );
	unfld->display( idx == cMaxNrVars );
	unflds_ += unfld;
    }

    formSet(0);
}

void rockPhysReq( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Rock Physics",
		  "Use a rock physics formula", "110.0.7") );
    uiRockPhysForm* formgrp = new uiRockPhysForm( &dlg, pr_.stdType() );

    BufferString formula;
    BufferString formulaunit;
    BufferString outunit;
    BufferStringSet inputunits;
    TypeSet<PropertyRef::StdType> inputtypes;
    if ( dlg.go() && formgrp->getFormulaInfo( formula, formulaunit, outunit,
					      inputunits, inputtypes, true ) )
    {
	formfld_->setText( formula.buf() );
	if ( inputunits.size() != inputtypes.size() )
	    return;

	formSet(0);
	for ( int idx=0; idx<inputunits.size(); idx++ )
	{
	    if ( !unflds_[idx] ) return;

	    ObjectSet<const UnitOfMeasure> possibleunits;
	    UoMR().getRelevant( inputtypes[idx], possibleunits );
	    const UnitOfMeasure* un =
			    UnitOfMeasure::getGuessed( inputunits[idx]->buf() );

	    uiComboBox& cbb = *unflds_[idx]->box();
	    cbb.setEmpty(); cbb.addItem( "-" );
	    for ( int idu=0; idu<possibleunits.size(); idu++ )
		cbb.addItem( possibleunits[idu]->name() );

	    if ( un && cbb.isPresent(un->name()) )
		cbb.setText( un->name() );
	    else
		cbb.setText( "-" );
	}
	
	unflds_[unflds_.size()-1]->box()->setText( formulaunit.buf() );
    }
}

void formSet( CallBacker* c ) 
{                                                                               
    getMathExpr();
    int nrvars = expr_ ? expr_->nrUniqueVarNames() : 0;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	if ( idx >= unflds_.size()-1 )
	    return;

	BufferString lbl( "Choose unit for", expr_->uniqueVarName(idx) );	
    }

    for ( int idx=0; idx<unflds_.size()-1; idx++ )
    {
	BufferString lblstr( "Choose unit for ", 
			     idx<nrvars ? expr_->uniqueVarName(idx)
			     		: "                         " );
	unflds_[idx]->label()->setText( lblstr.buf() );
	unflds_[idx]->display( idx<nrvars );
    }
}


void getMathExpr()                                               
{                                                                               
    delete expr_; expr_ = 0;                                                    
    if ( !formfld_ ) return;                                                    

    const BufferString inp( formfld_->text() );                                 
    if ( inp.isEmpty() ) return;                                                
	    
    MathExpressionParser mep( inp );                                            
    expr_ = mep.parse();                                                        
		    
    if ( !expr_ )                                                               
	uiMSG().warning( 
		BufferString("The provided expression cannot be used:\n",
		    	     mep.errMsg()) );
}


    const PropertyRef&	pr_;
    uiMathExpression*	formfld_;
    MathExpression*	expr_;
    ObjectSet<uiLabeledComboBox> unflds_;
};


void uiEditPropRef::setForm( CallBacker* )
{
    uiEditPropRefMathDef dlg( this, pr_, curunit_ );
    if ( dlg.go() )
	deffld_->setText( dlg.formfld_->text() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiEditPropRef::acceptOK( CallBacker* )
{
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() || !isalpha(newnm[0]) || newnm.size() < 2 )
	mErrRet("Please enter a valid name");

    pr_.setName( newnm );
    SeparString ss( aliasfld_->text() ); const int nral = ss.size();
    pr_.aliases().erase();
    for ( int idx=0; idx<nral; idx++ )
	pr_.aliases().add( ss[idx] );
    pr_.disp_.color_ = colfld_->color();
    Interval<float> vintv( rgfld_->getFInterval() );
    if ( !curunit_ )
	pr_.disp_.unit_.setEmpty();
    else
    {
	pr_.disp_.unit_ = curunit_->name();
	if ( !mIsUdf(vintv.start) )
	    vintv.start = curunit_->getSIValue( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = curunit_->getSIValue( vintv.stop );
    }
    pr_.disp_.range_ = vintv;

    BufferString defvalstr( deffld_->text() );
    char* ptr = defvalstr.buf(); mTrimBlanks(ptr);
    if ( !*ptr )
	{ delete pr_.disp_.defval_; pr_.disp_.defval_ = 0; }
    else
    {
	if ( withform_ && isNumberString(ptr) )
	    pr_.disp_.defval_ = new ValueProperty( pr_, toFloat(ptr) );
	else
	    pr_.disp_.defval_ = new MathProperty( pr_, ptr );
    }

    return true;
}


void uiBuildPROPS::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm ) return;

    PropertyRef* pr = 0;
    if ( !isadd )
	pr = props_.find( nm );
    else
    {
	PropertyRef::StdType typ = PropertyRef::Other;
	PropertyRef::parseEnumStdType( nm, typ );
	pr = new PropertyRef( "", typ );
    }
    if ( !pr ) return;

    uiEditPropRef dlg( this, *pr, isadd, allowmath_ );
    if ( !dlg.go() )
    {
	if ( isadd )
	    delete pr;
    }
    else
    {
	if ( isadd )
	    props_ += pr;
	handleSuccessfullEdit( isadd, pr->name() );
    }
}


void uiBuildPROPS::removeReq()
{
    const char* prnm = curDefSel();
    if ( prnm && *prnm )
    {
	const int idx = props_.indexOf( prnm );
	if ( idx < 0 ) return;
	delete props_.removeSingle( idx );
	removeItem();
    }
}


void uiBuildPROPS::itemSwitch( const char* nm1, const char* nm2 )
{
    const int idx1 = props_.indexOf( nm1 );
    const int idx2 = props_.indexOf( nm2 );
    if ( idx1 < 0 || idx2 < 0 ) { pErrMsg("Huh"); return; }
    props_.swap( idx1, idx2 );
}


uiManPROPS::uiManPROPS( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Layer Properties - Definition",
				"Define possible layer properties","110.1.0"))
{
    setCtrlStyle( LeaveOnly );
    buildfld_ = new uiBuildPROPS( this, ePROPS(), true );
    const char* strs[] = { "For this survey only",
			   "As default for all surveys",
			   "As default for my user ID only", 0 };
    srcfld_ = new uiGenInput( this, "Store", StringListInpSpec(strs) );
    srcfld_->attach( centeredBelow, buildfld_ );
}


bool uiManPROPS::rejectOK( CallBacker* )
{
    if ( !haveUserChange() )
	return true;

    ePROPS() = buildfld_->props_;
    const int isrc = srcfld_->getIntValue();
    const Repos::Source repsrc =   isrc == 0	? Repos::Survey
				: (isrc == 1	? Repos::Data
						: Repos::User);

    if ( !PROPS().save(repsrc) )
	uiMSG().warning( "Could not store the definitions to file."
			 "\nPlease check file/directory permissions." );
    else if ( repsrc != Repos::Survey )
    {
	if ( !PROPS().save(Repos::Survey) )
	    uiMSG().warning( "Could not store the definitions in your survey."
			 "\nPlease check file/directory permissions there." );
    }

    return true;
}

bool uiManPROPS::haveUserChange() const
{
    return buildfld_->haveUserChange();
}



uiSelectPropRefs::uiSelectPropRefs( uiParent* p, PropertyRefSelection& prs,
			      const char* lbl )
    : uiDialog(p,uiDialog::Setup("Layer Properties - Selection",
				"Select layer properties to use","110.1.2"))
    , props_(PROPS())
    , prsel_(prs)
    , thref_(&PropertyRef::thickness())
    , structchg_(false)
{
    uiLabeledListBox* llb = 0;
    if ( !lbl || !*lbl )
	propfld_ = new uiListBox( this, "Available properties" );
    else
    {
	llb = new uiLabeledListBox( this, lbl, true,
				    uiLabeledListBox::AboveMid );
	propfld_ = llb->box();
    }
    propfld_->setItemsCheckable( true );
    fillList();

    uiToolButton* manpropsbut = new uiToolButton( this, "man_props",
	    				"Manage available properties",
					mCB(this,uiSelectPropRefs,manPROPS) );
    if ( llb )
	manpropsbut->attach( centeredRightOf, llb );
    else
	manpropsbut->attach( centeredRightOf, propfld_ );
}


void uiSelectPropRefs::fillList()
{
    BufferStringSet dispnms;
    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const PropertyRef* ref = props_[idx];
	if ( ref != thref_ )
	    dispnms.add( ref->name() );
    }
    if ( dispnms.isEmpty() )
	return;

    dispnms.sort();
    propfld_->addItems( dispnms );

    int firstsel = -1;
    for ( int idx=0; idx<dispnms.size(); idx++ )
    {
	const char* nm = dispnms.get( idx ).buf();
	const bool issel = prsel_.isPresent( nm );
	propfld_->setItemChecked( idx, issel );
	if ( issel && firstsel < 0 ) firstsel = idx;
    }

    propfld_->setCurrentItem( firstsel < 0 ? 0 : firstsel );
}


void uiSelectPropRefs::manPROPS( CallBacker* )
{
    BufferStringSet orgnms;
    for ( int idx=0; idx<prsel_.size(); idx++ )
	orgnms.add( prsel_[idx]->name() );

    uiManPROPS dlg( this );
    if ( !dlg.go() )
	return;

    structchg_ = structchg_ || dlg.haveUserChange();

    // Even if user will cancel we cannot leave removed PROP's in the set
    for ( int idx=0; idx<orgnms.size(); idx++ )
    {
	if ( !props_.isPresent(prsel_[idx]) )
	    { structchg_ = true; prsel_.removeSingle( idx ); idx--; }
    }

    propfld_->setEmpty();
    fillList();
}


bool uiSelectPropRefs::acceptOK( CallBacker* )
{
    prsel_.erase();
    prsel_.insertAt( &PropertyRef::thickness(), 0 );

    for ( int idx=0; idx<propfld_->size(); idx++ )
    {
	if ( !propfld_->isItemChecked(idx) )
	    continue;

	const char* pnm = propfld_->textOfItem( idx );
	const PropertyRef* pr = props_.find( pnm );
	if ( !pr ) { pErrMsg("Huh"); structchg_ = true; continue; }
	prsel_ += pr;
    }

    return true;
}
