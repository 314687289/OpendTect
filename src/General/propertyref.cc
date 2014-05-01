/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "propertyref.h"
#include "mathproperty.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include "ascstream.h"
#include "safefileio.h"
#include "ioman.h"
#include "separstr.h"
#include "globexpr.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"
#include <typeinfo>

static const char* filenamebase = "Properties";
static const char* sKeyAliases = "Aliases";
static const char* sKeyDefaultValue = "DefaultValue";
static const char* sKeyDefinition = "Definition";

mImplFactory1Param(Property,const PropertyRef&,Property::factory)

DefineEnumNames(PropertyRef,StdType,0,"Standard Property")
{
	"Anisotropy",
	"Area",
	"Classification",
	"Compressibility",
	"Density",
	"Distance/Depth",
	"Elastic Ratio",
	"Electrical Potential",
	"Gamma Ray",
	"Impedance",
	"Permeability",
	"Pressure",
	"Pressure-Weight",
	"Resistivity",
	"Sonic travel time",
	"Temperature",
	"Time",
	"Velocity",
	"Volumetrics", // ratios: relative
	"Volume", // absolute
	"Other",
	0
};

struct PropRef_ThickRef_Man : public CallBacker
{

PropRef_ThickRef_Man()
{
    ref_ = new PropertyRef( sKey::Thickness(), PropertyRef::Dist );
    ref_->aliases().add( "thick" );
    const PropertyRef* thref = PROPS().find( "thickness" );
    if ( thref )
	ref_->disp_ = thref->disp_;
    else
    {
	ref_->disp_.color_ = Color::Black();
	ref_->disp_.range_ = Interval<float>( 0, 100 );
    }
    setZUnit();
    IOM().afterSurveyChange.notify( mCB(this,PropRef_ThickRef_Man,setZUnit) );
}


void setZUnit( CallBacker* cb=0 )
{
    ref_->disp_.unit_ = UnitOfMeasure::zUnitAnnot(false, true, false );
}

    PropertyRef*	ref_;

};


const PropertyRef& PropertyRef::thickness()
{
    mDefineStaticLocalObject( PtrMan<PropRef_ThickRef_Man>, ptm,
			      = new PropRef_ThickRef_Man );
    return *ptm->ref_;
}


PropertyRef::DispDefs::~DispDefs()
{
    delete defval_;
}


float PropertyRef::DispDefs::commonValue() const
{
    if ( defval_ && defval_->isValue() )
	return defval_->value();

    const bool udf0 = mIsUdf(range_.start);
    const bool udf1 = mIsUdf(range_.stop);
    if ( udf0 && udf1 )
	return 0;
    if ( udf0 || udf1 )
	return udf0 ? range_.stop : range_.start;
    return range_.center();
}


const PropertyRef& PropertyRef::undef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, udf, = 0 );
    if ( !udf )
    {
	PropertyRef* newudf = new PropertyRef( "Undef" );
	newudf->aliases().add( "" );
	newudf->aliases().add( "undef*" );
	newudf->aliases().add( "?undef?" );
	newudf->aliases().add( "?undefined?" );
	newudf->aliases().add( "udf" );
	newudf->aliases().add( "unknown" );
	newudf->disp_.color_ = Color::LightGrey();

	if ( !udf.setIfNull(newudf) )
	    delete newudf;

    }
    return *udf;
}


PropertyRef::~PropertyRef()
{
    delete mathdef_;
}


PropertyRef& PropertyRef::operator =( const PropertyRef& pr )
{
    if ( this != &pr )
    {
	setName( pr.name() );
	stdtype_ = pr.stdtype_;
	aliases_ = pr.aliases_;
	disp_ = pr.disp_;
    }
    return *this;
}


PropertyRef::StdType PropertyRef::surveyZType()
{
    return SI().zIsTime() ? Time : Dist;
}


void PropertyRef::setFixedDef( const MathProperty* mp )
{
    delete mathdef_;
    mathdef_ = mp ? mp->clone() : 0;
}


bool PropertyRef::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return this == &undef();

    if ( caseInsensitiveEqual(nm,name().buf(),0) )
	return true;
    for ( int idx=0; idx<aliases_.size(); idx++ )
    {
	GlobExpr ge( aliases_.get(idx), false );
	if ( ge.matches(nm) )
	    return true;
    }
    return false;
}


void PropertyRef::usePar( const IOPar& iop )
{
    aliases_.erase();
    FileMultiString fms( iop.find(sKeyAliases) );
    int sz = fms.size();
    for ( int ifms=0; ifms<sz; ifms++ )
	aliases_.add( fms[ifms] );

    iop.get( sKey::Color(), disp_.color_ );
    fms = iop.find( sKey::Range() );
    sz = fms.size();
    if ( sz > 1 )
    {
	disp_.range_.start = fms.getFValue( 0 );
	disp_.range_.stop = fms.getFValue( 1 );
	if ( sz > 2 )
	{
	    disp_.unit_ = fms[2];
	    const UnitOfMeasure* uom = UoMR().get( disp_.unit_ );
	    if ( uom )
	    {
		if ( !mIsUdf(disp_.range_.start) )
		    disp_.range_.start = uom->getSIValue(disp_.range_.start);
		if ( !mIsUdf(disp_.range_.stop) )
		    disp_.range_.stop = uom->getSIValue(disp_.range_.stop);
	    }
	}
    }

    delete disp_.defval_; disp_.defval_ = 0;
    delete mathdef_; mathdef_ = 0;

    BufferString mathdefstr;
    fms = iop.find( sKeyDefaultValue );
    sz = fms.size();
    if ( sz > 1 )
    {
	const BufferString typ( fms[0] );
	Property* prop = Property::factory().create( typ, *this );
	mDynamicCastGet(MathProperty*,mp,prop)
	if ( !mp )
	    disp_.defval_ = new ValueProperty( *this, toFloat(fms[1]) );
	else
	{
	    mathdef_ = mp;
	    mathdef_->setDef( fms[1] );
	}
    }

    const FixedString def = iop.find( sKeyDefinition );
    if ( !def.isEmpty() )
	mathdef_ = new MathProperty( *this, def );

    if ( !disp_.defval_ )
	disp_.defval_ = new ValueProperty( *this, disp_.commonValue() );
}


void PropertyRef::fillPar( IOPar& iop ) const
{
    if ( aliases_.isEmpty() )
	iop.removeWithKey( sKeyAliases );
    else
    {
	FileMultiString fms( aliases_.get(0) );
	for ( int idx=1; idx<aliases_.size(); idx++ )
	    fms += aliases_.get(idx);
	iop.set( sKeyAliases, fms );
    }

    iop.set( sKey::Color(), disp_.color_ );

    Interval<float> vintv( disp_.range_ );
    const UnitOfMeasure* uom = UoMR().get( disp_.unit_ );
    if ( uom )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = uom->getUserValueFromSI(vintv.start);
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = uom->getUserValueFromSI(vintv.stop);
    }
    FileMultiString fms;
    fms += ::toString( vintv.start );
    fms += ::toString( vintv.stop );
    if ( !disp_.unit_.isEmpty() )
	fms += disp_.unit_;
    iop.set( sKey::Range(), fms );
    if ( !disp_.defval_ )
	iop.removeWithKey( sKeyDefaultValue );
    else
	iop.set( sKeyDefaultValue, disp_.defval_->def() );

    if ( !mathdef_ )
	iop.removeWithKey( sKeyDefinition );
    else
	iop.set( sKeyDefinition, mathdef_->def() );
}


class PropertyRefSetMgr : public CallBacker
{
public:

PropertyRefSetMgr()
    : prs_(0)
{
    IOM().surveyChanged.notify( mCB(this,PropertyRefSetMgr,doNull) );
}

~PropertyRefSetMgr()
{
    delete prs_;
}

void doNull( CallBacker* )
{
    delete prs_; prs_ = 0;
}

void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	PropertyRefSet* oldprs = prs_;
	prs_ = new PropertyRefSet;
	prs_->readFrom( astrm );
	if ( prs_->isEmpty() )
	    { delete prs_; prs_ = oldprs; }
	else
	    { delete oldprs; sfio.closeSuccess(); break; }
    }

    if ( !prs_ )
	prs_ = new PropertyRefSet;
}

    PropertyRefSet*	prs_;

};

const PropertyRefSet& PROPS()
{
    mDefineStaticLocalObject( PropertyRefSetMgr, rsm, );
    if ( !rsm.prs_ )
	rsm.createSet();
    return *rsm.prs_;
}



PropertyRefSet& PropertyRefSet::operator =( const PropertyRefSet& prs )
{
    if ( this != &prs )
    {
	deepErase( *this );
	for ( int idx=0; idx<prs.size(); idx++ )
	    *this += new PropertyRef( *prs[idx] );
    }
    return *this;
}

int PropertyRefSet::indexOf( const char* nm ) const
{
    if ( nm && *nm )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    const PropertyRef& pr = *(*this)[idx];
	    if ( pr.name() == nm )
		return idx;
	}
	for ( int idx=0; idx<size(); idx++ )
	{
	    const PropertyRef& pr = *(*this)[idx];
	    if ( pr.isKnownAs(nm) )
		return idx;
	}
    }
    return -1;
}


int PropertyRefSet::indexOf( PropertyRef::StdType st, int occ ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *(*this)[idx];
	if ( pr.hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return idx;
	}
    }

    return -1;
}


PropertyRef* PropertyRefSet::fnd( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<PropertyRef*>( (*this)[idx] );
}


int PropertyRefSet::add( PropertyRef* pr )
{
    if ( !pr ) return -1;

    if ( !isPresent( pr->name() ) )
	{ ObjectSet<PropertyRef>::operator+=( pr ); return size()-1; }

    return -1;
}


int PropertyRefSet::ensurePresent( PropertyRef::StdType st, const char* nm1,
				   const char* nm2, const char* nm3 )
{
    int idx = indexOf( nm1 );
    if ( idx < 0 && nm2 )
	idx = indexOf( nm2 );
    if ( idx < 0 && nm3 )
	idx = indexOf( nm3 );
    if ( idx < 0 )
    {
	idx = indexOf( st );
	if ( idx >= 0 )
	    return idx;
    }
    if ( idx < 0 )
    {
	PropertyRef* pr = new PropertyRef( nm1, st );
	if ( nm2 && *nm2 ) pr->aliases().add( nm2 );
	if ( nm3 && *nm3 ) pr->aliases().add( nm3 );
	pr->disp_.color_ = Color::stdDrawColor( (int)st );
	idx = add( pr );
    }
    return idx;
}


bool PropertyRefSet::save( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    BufferString fnm = rfp.fileName( src );

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( sfio.errMsg() );
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    if ( !writeTo(astrm) )
	{ sfio.closeFail(); return false; }

    return sfio.closeSuccess();
}


void PropertyRefSet::readFrom( ascistream& astrm )
{
    deepErase( *this );

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	const BufferString propnm( iop.getKey(0) );
	if ( find(propnm) )
	    continue;

	const BufferString stdtypstr( iop.getValue(0) );
	PropertyRef::StdType st;
        PropertyRef::parseEnumStdType(stdtypstr, st );
	PropertyRef* pr = new PropertyRef( propnm, st );
	pr->usePar( iop );

	if ( add(pr) < 0 )
	    delete pr;
    }
}


bool PropertyRefSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Properties" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *(*this)[idx];
	IOPar iop;
	iop.set( pr.name(), PropertyRef::getStdTypeString(pr.stdType()) );
	pr.fillPar( iop );
	iop.putTo( astrm );
    }
    return astrm.isOK();
}


PropertyRefSelection::PropertyRefSelection()
{
    *this += &PropertyRef::thickness();
}


bool PropertyRefSelection::operator ==( const PropertyRefSelection& oth ) const
{
    if ( size() != oth.size() )
	return false;

    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] != oth[idx] )
	    return false;

    return true;
}


int PropertyRefSelection::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *((*this)[idx]);
	if ( pr.name() == nm )
	    return idx;
    }
    return -1;
}


int PropertyRefSelection::find( const char* nm ) const
{
    const int idxof = indexOf( nm );
    if ( idxof >= 0 )
	return idxof;

    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *((*this)[idx]);
	if ( pr.isKnownAs( nm ) )
	    return idx;
    }

    return -1;
}


PropertyRefSelection PropertyRefSelection::subselect(
					PropertyRef::StdType type ) const
{
    PropertyRefSelection subsel;
    subsel.erase();
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] && (*this)[idx]->hasType( type ) )
	    subsel += (*this) [idx];

    return subsel;
}


PropertyRefSelection PropertyRefSelection::getAll( bool withth,
					const PropertyRef* exclude )
{
    PropertyRefSelection ret;
    if ( !withth )
	ret -= &PropertyRef::thickness();

    const PropertyRefSet& props = PROPS();
    for ( int idx=0; idx<props.size(); idx++ )
    {
	const PropertyRef* pr = props[idx];
	if ( pr != exclude )
	    ret += pr;
    }
    return ret;
}


PropertyRefSelection PropertyRefSelection::getAll( PropertyRef::StdType typ )
{
    PropertyRefSelection ret;
    if ( typ != PropertyRef::Dist )
	ret -= &PropertyRef::thickness();

    const PropertyRefSet& props = PROPS();
    for ( int idx=0; idx<props.size(); idx++ )
    {
	const PropertyRef* pr = props[idx];
	if ( pr->stdType() == typ )
	    ret += pr;
    }
    return ret;
}
