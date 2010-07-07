/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: attribsel.cc,v 1.50 2010-07-07 20:58:34 cvskris Exp $";

#include "attribsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "globexpr.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "zdomain.h"

namespace Attrib
{

#define mDescIDRet(val) static DescID res(val,true); return res

const DescID& SelSpec::cOtherAttrib()	{ mDescIDRet(-1); }
const DescID& SelSpec::cNoAttrib()	{ mDescIDRet(-2); }
const DescID& SelSpec::cAttribNotSel()	{ mDescIDRet(-3); }

const char* SelSpec::sKeyRef()		{ return "Attrib Reference"; }
const char* SelSpec::sKeyID()		{ return "Attrib ID"; }
const char* SelSpec::sKeyIsNLA()	{ return "Is attrib NLA Model"; }
const char* SelSpec::sKeyObjRef()	{ return "Object Reference"; }
const char* SelSpec::sKeyDefStr()	{ return "Definition"; }
const char* SelSpec::sKeyIs2D()		{ return "Is 2D"; }
const char* SelSpec::sKeyOnlyStoredData()
				      { return "DescSet only for stored data"; }
static const char* isnnstr = "Is attrib NN"; // for backward compatibility

bool SelSpec::operator==( const SelSpec& ss ) const
{
    return id()==ss.id() && isNLA()==ss.isNLA() && ss.ref_==ref_ &&
	ss.objref_==objref_ && ss.defstring_==defstring_ && is2D()==ss.is2D();
}


bool SelSpec::operator!=( const SelSpec& ss ) const
{
    return !(*this==ss);
}


void SelSpec::setDepthDomainKey( const Desc& desc )
{
    zdomainkey_ = "";

    BufferString storedid = desc.getStoredID();
    if ( storedid.isEmpty() ) return;

    PtrMan<IOObj> ioobj = IOM().get( MultiID(storedid.buf()) );
    if ( ioobj )
    {
	if ( !ioobj->pars().get( ZDomain::sKey(), zdomainkey_ ) )
	{
	    //Legacy. Can be removed in od4
	    ioobj->pars().get( "Depth Domain", zdomainkey_ );
	}
    }
}


void SelSpec::set( const Desc& desc )
{
    isnla_ = false;
    is2d_ = desc.is2D();
    id_ = desc.id();
    ref_ = desc.userRef();
    desc.getDefStr( defstring_ );
    setDepthDomainKey( desc );
    setDiscr( *desc.descSet() );
}


void SelSpec::set( const NLAModel& nlamod, int nr )
{
    isnla_ = true;
    id_ = DescID(nr,false);
    setRefFromID( nlamod );
}


void SelSpec::setDiscr( const NLAModel& nlamod )
{
    if ( nlamod.design().isSupervised() )
	discrspec_ = StepInterval<int>(0,0,0);
    else
    {
	discrspec_.start = discrspec_.step = 1;
	discrspec_.stop = nlamod.design().hiddensz;
    }
}


void SelSpec::setDiscr( const DescSet& ds )
{
    discrspec_ = StepInterval<int>(0,0,0);
}


void SelSpec::setIDFromRef( const NLAModel& nlamod )
{
    isnla_ = true;
    const int nrout = nlamod.design().outputs.size();
    id_ = cNoAttrib();
    for ( int idx=0; idx<nrout; idx++ )
    {
	if ( ref_ == *nlamod.design().outputs[idx] )
	    { id_ = DescID(idx,false); break; }
    }
    setDiscr( nlamod );
}


void SelSpec::setIDFromRef( const DescSet& ds )
{
    isnla_ = false;
    id_ = ds.getID( ref_, true );
    BufferString attribname;
    if ( Desc::getAttribName( defstring_.buf(), attribname ) )
    {
	if ( ds.getDesc(id_) && 
	     strcmp( attribname, ds.getDesc(id_)->attribName() ) )
	    id_ = ds.getID( defstring_, ds.containsStoredDescOnly() );
    }
    const Desc* desc = ds.getDesc( id_ );
    if ( desc )
	setDepthDomainKey( *desc );
    setDiscr( ds );
}


void SelSpec::setRefFromID( const NLAModel& nlamod )
{
    ref_ = id_.asInt() >= 0 ? nlamod.design().outputs[id_.asInt()]->buf() : "";
    setDiscr( nlamod );
}


void SelSpec::setRefFromID( const DescSet& ds )
{
    const Desc* desc = ds.getDesc( id_ );
    ref_ = "";
    if ( desc )
    {
	ref_ = desc->userRef();
	desc->getDefStr( defstring_ );
	setDepthDomainKey( *desc );
    }
    setDiscr( ds );
}


void SelSpec::fillPar( IOPar& par ) const
{
    par.set( sKeyRef(), ref_ );
    par.set( sKeyID(), id_.asInt() );
    par.setYN( sKeyOnlyStoredData(), id_.isStored() );
    par.setYN( sKeyIsNLA(), isnla_ );
    par.set( sKeyObjRef(), objref_ );
    par.set( sKeyDefStr(), defstring_ );
    par.set( ZDomain::sKey(), zdomainkey_ );
    par.setYN( sKeyIs2D(), is2d_ );
}


bool SelSpec::usePar( const IOPar& par )
{
    ref_ = ""; 			par.get( sKeyRef(), ref_ );
    id_ = cNoAttrib();		par.get( sKeyID(), id_.asInt() );
    bool isstored = false;	par.getYN( sKeyOnlyStoredData(), isstored );
    id_.setStored( isstored );
    isnla_ = false; 		par.getYN( sKeyIsNLA(), isnla_ );
    				par.getYN( isnnstr, isnla_ );
    objref_ = "";		par.get( sKeyObjRef(), objref_ );
    defstring_ = "";		par.get( sKeyDefStr(), defstring_ );
    zdomainkey_ = "";		if ( !par.get( ZDomain::sKey(), zdomainkey_ ) )
				    par.get( "Depth Domain", zdomainkey_);
    is2d_ = false;		par.getYN( sKeyIs2D(), is2d_ );
    		
    return true;
}


bool SelSpec::isStored() const
{
    return id_.isValid() && id_.isStored();
}



SelInfo::SelInfo( const DescSet* attrset, const NLAModel* nlamod, 
		  bool is2d, const DescID& ignoreid, bool usesteering,
       		  bool onlysteering, bool onlymulticomp )
    : is2d_( is2d )
    , usesteering_( usesteering )
    , onlysteering_( onlysteering )
    , onlymulticomp_( onlymulticomp )
{
    fillStored();

    if ( attrset )
    {
	for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	{
	    const DescID descid = attrset->getID( idx );
	    const Desc* desc = attrset->getDesc( descid );
	    if ( !desc || 
		 !strcmp(desc->attribName(),StorageProvider::attribName()) || 
		 attrset->getID(*desc) == ignoreid || desc->isHidden() )
		continue;

	    attrids_ += descid;
	    attrnms_.add( desc->userRef() );
	}
    }

    if ( nlamod )
    {
	const int nroutputs = nlamod->design().outputs.size();
	for ( int idx=0; idx<nroutputs; idx++ )
	{
	    BufferString nm( *nlamod->design().outputs[idx] );
	    if ( IOObj::isKey(nm) )
		nm = IOM().nameOf( nm, false );
	    nlaoutnms_.add( nm );
	}
    }
}


void SelInfo::fillStored( const char* filter )
{
    ioobjnms_.erase(); ioobjids_.erase();

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    GlobExpr* ge = filter && *filter ? new GlobExpr( filter ) : 0;

    const char* survdomain = SI().getZDomainString();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( *ioobj.group() == 'W' ) continue;
	if ( SeisTrcTranslator::isPS( ioobj ) ) continue;
	const bool is2d = SeisTrcTranslator::is2D(ioobj,true);
	const bool isvalid3d = !is2d && ioobj.isReadDefault();

	if ( (is2d_ != is2d) || (!is2d && !isvalid3d) )
	    continue;

	FixedString zdomain = ioobj.pars().find(ZDomain::sKey());
	if ( !zdomain )
	    zdomain = ioobj.pars().find( "Depth Domain" );

	if ( zdomain && zdomain!=survdomain )
	    continue;

	const char* res = ioobj.pars().find( sKey::Type );
	if ( res && ( (!usesteering_ && !strcmp(res,sKey::Steering) )
	         || ( onlysteering_ && strcmp(res,sKey::Steering) ) ) )
	    continue;

	if ( !res && onlysteering_ && !is2d ) continue;

	const char* ioobjnm = ioobj.name().buf();
	if ( ge && !ge->matches(ioobjnm) )
	    continue;

	if ( onlymulticomp_ )
	{
	    if ( is2d )
	    {
		BufferStringSet nms;
		SelInfo::getAttrNames( ioobj.key(), nms, usesteering_, true );
		if ( nms.isEmpty() ) continue;
	    }
	    else
	    {
		if ( SeisIOObjInfo(ioobj).nrComponents() < 2 )
		    continue;
	    }
	}

	ioobjnms_.add( ioobjnm );
	ioobjids_.add( (const char*)ioobj.key() );
	if ( ioobjnms_.size() > 1 )
	{
	    for ( int icmp=ioobjnms_.size()-2; icmp>=0; icmp-- )
	    {
		if ( ioobjnms_.get(icmp) > ioobjnms_.get(icmp+1) )
		{
		    ioobjnms_.swap( icmp, icmp+1 );
		    ioobjids_.swap( icmp, icmp+1 );
		}
	    }
	}
    }
}


SelInfo::SelInfo( const SelInfo& asi )
	: ioobjnms_(asi.ioobjnms_)
	, ioobjids_(asi.ioobjids_)
	, attrnms_(asi.attrnms_)
	, attrids_(asi.attrids_)
	, nlaoutnms_(asi.nlaoutnms_)
	, is2d_(asi.is2d_)
	, usesteering_(asi.usesteering_)
	, onlysteering_(asi.onlysteering_)
{
}


SelInfo& SelInfo::operator=( const SelInfo& asi )
{
    ioobjnms_ = asi.ioobjnms_;
    ioobjids_ = asi.ioobjids_;
    attrnms_ = asi.attrnms_;
    attrids_ = asi.attrids_;
    nlaoutnms_ = asi.nlaoutnms_;
    is2d_ = asi.is2d_;
    usesteering_ = asi.usesteering_;
    onlysteering_ = asi.onlysteering_;
    return *this;
}


bool SelInfo::is2D( const char* defstr )
{
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    return ioobj ? SeisTrcTranslator::is2D(*ioobj,true) : false;
}


void SelInfo::getAttrNames( const char* defstr, BufferStringSet& nms, 
			    bool issteer, bool onlymulticomp )
{
    nms.erase();
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    if ( !ioobj || !SeisTrcTranslator::is2D(*ioobj,true) )
	return;

    Seis2DLineSet ls( ioobj->fullUserExpr(true) );
    ls.getAvailableAttributes( nms, sKey::Steering, !issteer, issteer );

    if ( onlymulticomp )
    {
	for ( int idx=nms.size()-1; idx>=0; idx-- )
	{
	    LineKey tmpkey( "", nms.get(idx).buf() );
	    if ( SeisIOObjInfo(*ioobj).nrComponents(tmpkey) < 2 )
		nms.remove( idx );
	}
    }
}


void SelInfo::getZDomainItems( const char* zdomainkey, const char* zdomainid,
			       BufferStringSet& nms )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	const char* dres = ioobj.pars().find( "Depth Domain" ); // Legacy
	if ( dres && !strcmp(dres,zdomainkey) )
	{
	    IOObj& myioobj = const_cast<IOObj&>(ioobj);
	    ioobj.pars().remove( "Depth Domain" );
	    ioobj.pars().set( ZDomain::sKey(), dres );
	}

	const char* zkey = ioobj.pars().find( ZDomain::sKey() );
	const char* zid = ioobj.pars().find( ZDomain::sKeyID() );
	const bool matchkey = zkey && !strcmp(zkey,zdomainkey);
	const bool matchid = zid && !strcmp(zid,zdomainid);
	if ( matchkey && (!zdomainid || !zid || matchid) )
	    nms.add( ioobj.name() );
    }

    nms.sort();
}


} // namespace Attrib
