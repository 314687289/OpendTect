/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiiosurface.cc,v 1.68 2009-04-02 13:51:26 cvsbert Exp $";

#include "uiiosurface.h"

#include "uipossubsel.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "embodytr.h"
#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"
#include "survinfo.h"


const int cListHeight = 5;

uiIOSurface::uiIOSurface( uiParent* p, bool forread, const char* typ )
    : uiGroup(p,"Surface selection")
    , ctio_( 0 )
    , sectionfld_(0)
    , attribfld_(0)
    , rgfld_(0)
    , attrSelChange(this)
    , forread_(forread)
{
    if ( !strcmp(typ,EMHorizon2DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMHorizon2D);
    else if (!strcmp(typ,EMHorizon3DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMHorizon3D);
    else if ( !strcmp(typ,EMFaultStickSetTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMFaultStickSet);
    else if ( !strcmp(typ,EMFault3DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMFault3D);
    else
	ctio_ = new CtxtIOObj( polygonEMBodyTranslator::getIOObjContext() );
}


uiIOSurface::~uiIOSurface()
{
    delete ctio_->ioobj; delete ctio_;
}


void uiIOSurface::mkAttribFld()
{
    attribfld_ = new uiLabeledListBox( this, "Calculated attributes", true,
				      uiLabeledListBox::AboveMid );
    attribfld_->setStretch( 1, 1 );
    attribfld_->box()->selectionChanged.notify( mCB(this,uiIOSurface,attrSel) );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    sectionfld_ = new uiLabeledListBox( this, "Available patches", true,
				     labelabove ? uiLabeledListBox::AboveMid 
				     		: uiLabeledListBox::LeftTop );
    sectionfld_->setPrefHeightInChar( cListHeight );
    sectionfld_->setStretch( 2, 2 );
    sectionfld_->box()->selectionChanged.notify( 
	    				mCB(this,uiIOSurface,ioDataSelChg) );
}


void uiIOSurface::mkRangeFld()
{
    rgfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    rgfld_->selChange.notify( mCB(this,uiIOSurface,ioDataSelChg) );
    if ( sectionfld_ ) rgfld_->attach( ensureBelow, sectionfld_ );
}


void uiIOSurface::mkObjFld( const char* lbl )
{
    ctio_->ctxt.forread = forread_;
    objfld_ = new uiIOObjSel( this, *ctio_, lbl );
    if ( forread_ )
	objfld_->selectiondone.notify( mCB(this,uiIOSurface,objSel) );
}


bool uiIOSurface::fillFields( const MultiID& id, bool showerrmsg )
{
    EM::SurfaceIOData sd;

    if ( forread_ )
    {
	const char* res = EM::EMM().getSurfaceData( id, sd );
	if ( res )
	{
	    if ( showerrmsg )
		uiMSG().error( res );
	    return false;
	}
    }
    else
    {
	const EM::ObjectID emid = EM::EMM().getObjectID( id );
	mDynamicCastGet(EM::Surface*,emsurf,EM::EMM().getObject(emid));
	if ( emsurf )
	    sd.use(*emsurf);
	else
	{
	    if ( showerrmsg )
		uiMSG().error( "Surface not loaded" );
	    return false;
	}
    }

    fillAttribFld( sd.valnames );
    fillSectionFld( sd.sections );
    fillRangeFld( sd.rg );
    return true;
}


void uiIOSurface::fillAttribFld( const BufferStringSet& valnames )
{
    if ( !attribfld_ ) return;

    attribfld_->box()->empty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attribfld_->box()->addItem( valnames[idx]->buf() );
}


void uiIOSurface::fillSectionFld( const BufferStringSet& sections )
{
    if ( !sectionfld_ ) return;

    sectionfld_->box()->empty();
    for ( int idx=0; idx<sections.size(); idx++ )
	sectionfld_->box()->addItem( sections[idx]->buf() );
    sectionfld_->box()->selectAll( true );
}


void uiIOSurface::fillRangeFld( const HorSampling& hrg )
{
    if ( !rgfld_ ) return;
    CubeSampling cs( rgfld_->envelope() );
    cs.hrg = hrg; rgfld_->setInput( cs );
}


bool uiIOSurface::haveAttrSel() const
{
    for ( int idx=0; idx<attribfld_->box()->size(); idx++ )
    {
	if ( attribfld_->box()->isSelected(idx) )
	    return true;
    }
    return false;
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels ) const
{
    if ( !rgfld_ || rgfld_->isAll() )
	sels.rg.init( false );
    else
    {
	IOPar par;
	rgfld_->fillPar( par );
	sels.rg.usePar( par );
    }

    if ( SI().sampling(true) != SI().sampling(false) )
    {
	if ( sels.rg.isEmpty() )
	    sels.rg.init( true );
	sels.rg.limitTo( SI().sampling(true).hrg );
    }
	
    sels.selsections.erase();
    int nrsections = sectionfld_ ? sectionfld_->box()->size() : 1;
    for ( int idx=0; idx<nrsections; idx++ )
    {
	if ( nrsections == 1 || sectionfld_->box()->isSelected(idx) )
	    sels.selsections += idx;
    }

    sels.selvalues.erase();
    int nrattribs = attribfld_ ? attribfld_->box()->size() : 0;
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	if ( attribfld_->box()->isSelected(idx) )
	    sels.selvalues += idx;
    }
}


IOObj* uiIOSurface::selIOObj() const
{
    objfld_->commitInput();
    return ctio_->ioobj;
}


void uiIOSurface::objSel( CallBacker* )
{
    IOObj* ioobj = objfld_->ctxtIOObj().ioobj;
    if ( !ioobj ) return;

    fillFields( ioobj->key() );
}


void uiIOSurface::attrSel( CallBacker* )
{
    attrSelChange.trigger();
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p,
				const uiSurfaceWrite::Setup& setup )
    : uiIOSurface(p,false,setup.typ_)
    , displayfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
{
    surfrange_.init( false );

    if ( setup.typ_ != EMHorizon2DTranslatorGroup::keyword() )
    {
	if ( setup.withsubsel_ )
    	    mkRangeFld();
	if ( sectionfld_ && rgfld_ )
	    rgfld_->attach( alignedBelow, sectionfld_ );
    }

    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( "Output Stickset" );
    else
	mkObjFld( "Output Surface" );


    if ( rgfld_ )
	objfld_->attach( alignedBelow, rgfld_ );

    if ( setup.withstratfld_ )
    {
	stratlvlfld_ = new uiStratLevelSel( this );
	stratlvlfld_->attach( alignedBelow, objfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiSurfaceWrite,stratLvlChg) );
    }

    if ( setup.withcolorfld_ )
    {
	colbut_ = new uiColorInput( this, 
	    uiColorInput::Setup(getRandStdDrawColor()).lbltxt("Base color") );
	colbut_->attach( alignedBelow, objfld_ );
	if ( stratlvlfld_ ) colbut_->attach( ensureBelow, stratlvlfld_ );
    }

    if ( setup.withdisplayfld_ )
    {
       displayfld_ = new uiCheckBox( this, setup.displaytext_ );
       displayfld_->attach( alignedBelow, objfld_ );
       if ( stratlvlfld_ ) displayfld_->attach( ensureBelow, stratlvlfld_ );
       if ( colbut_ ) displayfld_->attach( ensureBelow, colbut_ );
       displayfld_->setChecked( true );
    }

    setHAlignObj( objfld_ );
    ioDataSelChg( 0 );
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const EM::Surface& surf, 
				const uiSurfaceWrite::Setup& setup )
    : uiIOSurface(p,false,setup.typ_)
    , displayfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
{
    surfrange_.init( false );

    if ( setup.typ_!=EMHorizon2DTranslatorGroup::keyword() &&
	 setup.typ_!=EMFaultStickSetTranslatorGroup::keyword() &&
	 setup.typ_!=EMFault3DTranslatorGroup::keyword() &&
	 setup.typ_!=polygonEMBodyTranslator::sKeyUserName() )
    {
	if ( surf.nrSections() > 1 )
	    mkSectionFld( false );

	if ( setup.withsubsel_ )
	{
    	    mkRangeFld();
	    EM::SurfaceIOData sd;
	    sd.use( surf );
	    surfrange_ = sd.rg;
	}

	if ( sectionfld_ && rgfld_ )
	    rgfld_->attach( alignedBelow, sectionfld_ );
    }
    
    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( "Output Stickset" );
    else
	mkObjFld( "Output Surface" );

    if ( rgfld_ )
    {
	objfld_->attach( alignedBelow, rgfld_ );
	setHAlignObj( rgfld_ );
    }

    if ( setup.withdisplayfld_ )
    {
       displayfld_ = new uiCheckBox( this, "Replace in tree" );
       displayfld_->attach( alignedBelow, objfld_ );
       displayfld_->setChecked( true );
    }

    fillFields( surf.multiID() );

    ioDataSelChg( 0 );
}


bool uiSurfaceWrite::processInput()
{
    if ( sectionfld_ && !sectionfld_->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    if ( !objfld_->commitInput() )
    {
	uiMSG().error( "Please select an output surface" );
	return false;
    }

    return true;
}


bool uiSurfaceWrite::replaceInTree() const       
{ return displayfld_->isChecked(); }


void uiSurfaceWrite::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const Color col( stratlvlfld_->getColor() );
    if ( col != Color::NoColor() )
	colbut_->setColor( col );
}


int uiSurfaceWrite::getStratLevelID() const
{
    return stratlvlfld_ ? stratlvlfld_->getID() : -1;
}


Color uiSurfaceWrite::getColor() const
{
    return colbut_ ? colbut_->color() : getRandStdDrawColor();
}


void uiSurfaceWrite::ioDataSelChg( CallBacker* )
{
    bool issubsel = sectionfld_ &&
		sectionfld_->box()->size()!=sectionfld_->box()->nrSelected();

    if ( !issubsel && rgfld_ && !rgfld_->isAll() )
    {
	const HorSampling& hrg = rgfld_->envelope().hrg;
	issubsel = surfrange_.isEmpty() ? true :
	    !hrg.includes(surfrange_.start) || !hrg.includes(surfrange_.stop);
    }

    if ( displayfld_ && issubsel )
    {
	displayfld_->setChecked( false );
	displayfld_->setSensitive( false );
    }
    else if ( displayfld_ && !displayfld_->sensitive() )
    {
	displayfld_->setSensitive( true );
	displayfld_->setChecked( true );
    }
}


uiSurfaceRead::uiSurfaceRead( uiParent* p, const Setup& setup )
    : uiIOSurface(p,true,setup.typ_)
{
    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( "Input Stickset" );
    else
	mkObjFld( "Input Surface" );

    uiGroup* attachobj = objfld_;

    if ( setup.withsectionfld_ )
	mkSectionFld( setup.withattribfld_ );

    if ( objfld_->ctxtIOObj().ioobj )
	objSel(0);

    if ( setup.withattribfld_ )
    {
	mkAttribFld();
	attribfld_->attach( alignedBelow, objfld_ );
	sectionfld_->attach( rightTo, attribfld_ );
	attachobj = attribfld_;
    }
    else if ( setup.withsectionfld_ )
    {
	sectionfld_->attach( alignedBelow, objfld_ );
	attachobj = sectionfld_;
    }

    if ( setup.withsubsel_ )
    {
	mkRangeFld();
	rgfld_->attach( alignedBelow, attachobj );
    }

    setHAlignObj( objfld_ );
}


void uiSurfaceRead::setIOObj( const MultiID& mid )
{
    ctio_->setObj( mid );
    objfld_->updateInput();
    objSel( 0 );
}


bool uiSurfaceRead::processInput()
{
    if ( !objfld_->commitInput() )
    {
	uiMSG().error( "Please select input" );
	return false;
    }

    if ( sectionfld_ && !sectionfld_->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    return true;
}
