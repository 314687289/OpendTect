/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "hostdata.h"
#include "strmoper.h"
#include "strmprov.h"
#include "ascstream.h"
#include "envvars.h"
#include "genc.h"
#include "msgh.h"
#include "oddirs.h"
#include "separstr.h"
#include "filepath.h"
#include "debug.h"
#include "od_strstream.h"
#include "oscommand.h"
#ifdef __win__
# include <windows.h>
#else
# include <unistd.h>
# include <netdb.h>
#endif

#define mDebugOn        (DBG::isOn(DBG_FILEPATH))

const char* HostData::localHostName()
{
    return GetLocalHostName();
}


bool HostData::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm ) return false;
    if ( name_ == nm ) return true;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return true;
    return false;
}


void HostData::addAlias( const char* nm )
{
    if ( !nm || !*nm || name_ == nm ) return;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return;
    aliases_ += new BufferString( nm );
}


BufferString HostData::getFullDispString() const
{
    BufferString ret( name() );
    for ( int idx=0; idx<nrAliases(); idx++ )
	ret.add( " / " ).add( alias(idx) );
    return ret;
}


void HostData::init( const char* nm )
{
    name_ = nm;

    const char* ptr = name_.buf();
    bool is_ip_adrr = true;

    while ( ptr++ && *ptr )
    {
	mSkipBlanks(ptr)
	if ( !isdigit(*ptr) && * ptr != '.' )
	    is_ip_adrr = false;
    }

    if ( !is_ip_adrr )
    {
	char* dot = name_.find( '.' );
	if ( dot ) { *dot ='\0'; addAlias(nm); }
    }
}


static FilePath getReplacePrefix( const FilePath& dir_,
		const FilePath& fromprefix_, const FilePath& toprefix_ )
{
    if ( !fromprefix_.nrLevels() || !toprefix_.nrLevels() )
	return FilePath(dir_);

    // convert paths to Unix style
    BufferString dir = dir_.fullPath( FilePath::Unix );
    BufferString fromprefix = fromprefix_.fullPath( FilePath::Unix );
    BufferString toprefix = toprefix_.fullPath( FilePath::Unix );

    const char* tail = dir.find( fromprefix );
    if ( !tail )
    {
	fromprefix.toLower();
	tail = dir.find( fromprefix );
    }

    if ( !tail )
	return FilePath(dir_);

    tail += fromprefix.size();

    BufferString ret = toprefix;
    ret += tail;

    if ( mDebugOn )
    {
	BufferString msg("getReplacePrefix:\n in: '");
	msg += dir;
	msg += "' \n replace '";
	msg += fromprefix;
	msg += "' with '";
	msg += toprefix;
	msg += "'\n to: '";
	msg += ret;
	msg += "'";

	DBG::message(msg);
    }

    return FilePath(ret);
}


FilePath HostData::convPath( PathType pt, const FilePath& fp,
			     const HostData* from ) const
{
    if ( !from ) from = &localHost();
    return getReplacePrefix( fp, from->prefixFilePath(pt), prefixFilePath(pt) );
}


HostDataList::HostDataList( bool readhostfile )
	: realaliases_(false)
	, rshcomm_("rsh")
	, defnicelvl_(19)
	, portnr_(1963)
{
    BufferString bhfnm = "BatchHosts";
    if ( GetEnvVar("DTECT_BATCH_HOSTS_FILENAME") )
	bhfnm = GetEnvVar("DTECT_BATCH_HOSTS_FILENAME");

    BufferString fname( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,
				bhfnm.buf(),true) );
    if ( GetEnvVar("DTECT_BATCH_HOSTS_FILEPATH") )
	fname = GetEnvVar("DTECT_BATCH_HOSTS_FILEPATH");

    if ( readhostfile )
    {
#ifdef __win__
	readHostFile(fname);
#else
	if ( !readHostFile(fname) )
	{
	    sethostent(0);
	    struct hostent* he;
	    while ( (he = gethostent()) )
	    {
		HostData* newhd = new HostData( he->h_name );
		char** al = he->h_aliases;
		while ( *al )
		{
		    if ( !newhd->isKnownAs(*al) )
			newhd->aliases_.add( *al );
		    al++;
		}
		*this += newhd;
	    }
	    endhostent();
	    realaliases_ = true;
	}
#endif
    }
    handleLocal();
}

bool HostDataList::readHostFile( const char* fname )
{
    od_istream strm( fname );
    if ( !strm.isOK() )
	return false;

    ascistream astrm( strm );
    if ( !astrm.isOfFileType("Batch Processing Hosts") )
    {
	const BufferString msg( fname, ": hosts file has invalid file header" );
	ErrMsg( msg ); return false;
    }

    BufferString sharehost;

    if ( atEndOfSection(astrm) ) astrm.next();

    bool foundrsh = false;
    while ( !atEndOfSection(astrm) )
    {
	if ( astrm.hasKeyword("Remote shell") )
	    { rshcomm_ = astrm.value(); foundrsh = !rshcomm_.isEmpty(); }
	if ( astrm.hasKeyword("Default nice level") )
	    defnicelvl_ = astrm.getIValue();
	if ( astrm.hasKeyword("First port") )
	    portnr_ = astrm.getIValue();
	if ( astrm.hasKeyword("Win appl prefix") )
	    win_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx appl prefix") )
	    unx_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Win data prefix") )
	    win_data_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx data prefix") )
	    unx_data_pr_.set( (char*) astrm.value() );

	if ( astrm.hasKeyword("Data host") )
	    sharehost = astrm.value();  // store in a temporary
	if ( astrm.hasKeyword("Data drive") )
	    sharedata_.drive_ = astrm.value();
	if ( astrm.hasKeyword("Data share") )
	    sharedata_.share_ = astrm.value();
	if ( astrm.hasKeyword("Data pass") )
	    sharedata_.pass_ = astrm.value();

	astrm.next();
    }

    if ( foundrsh )
	OS::MachineCommand::setDefaultRemExec( rshcomm_ );

    while ( !atEndOfSection(astrm.next()) )
    {
	HostData* newhd = new HostData( astrm.keyWord() );
	if ( *astrm.value() )
	{
	    SeparString val( astrm.value(), ':' );
	    BufferString vstr;

#define mGetVStr(valnr) \
	    vstr = val[valnr]; vstr.trimBlanks()

	    mGetVStr(0);
	    if ( !vstr.isEmpty() )
		newhd->aliases_.add( vstr );

	    mGetVStr(1);
	    newhd->iswin_ = vstr == "win";

	    mGetVStr(2);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->iswin_ )
		    vstr.replace( ';', ':' );
		newhd->data_pr_ = vstr;
	    }
	    else
		newhd->data_pr_ = newhd->iswin_ ? win_data_pr_ : unx_data_pr_;

	    mGetVStr(3);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->iswin_ )
		    vstr.replace( ';', ':' );
		newhd->appl_pr_ = vstr;
	    }
	    else
		newhd->appl_pr_ = newhd->iswin_ ? win_appl_pr_ : unx_appl_pr_;

	    mGetVStr(4);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->iswin_ )
		    vstr.replace( ';', ':' );
		newhd->pass_ = vstr;
	    }
	    else if ( newhd->iswin_ )
		newhd->pass_ = sharedata_.pass_;

	    newhd->setShareData( &sharedata_ );
	}
	*this += newhd;
    }

    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];
	if ( hd->isKnownAs(sharehost) )
	{
	    sharedata_.host_ = hd;
	    break;
	}
    }

    static bool complain = !GetEnvVarYN( "OD_NO_DATAHOST_CHK" );
    if ( sharehost.size() && !sharedata_.host_ && complain )
    {
	BufferString msg("No host "); msg += sharehost;
	msg += "  found in "; msg += fname;
	msg += ". Multi machine batch processing may not work as expected.";
	ErrMsg( msg );
	complain = false;
    }

    if ( mDebugOn )
    {
	od_ostrstream strstrm; dump( strstrm );
	DBG::message( strstrm.result() );
    }
    return true;
}


#define mPrMemb(obj,x) { strm << "-- " << #x << "='" << obj->x << "'\n"; }

void HostDataList::dump( od_ostream& strm ) const
{
    strm << "\n\n-- Host data list:\n--\n";
    mPrMemb(this,rshcomm_)
    mPrMemb(this,defnicelvl_)
    mPrMemb(this,portnr_)
    mPrMemb(this,win_appl_pr_.fullPath())
    mPrMemb(this,unx_appl_pr_.fullPath())
    mPrMemb(this,win_data_pr_.fullPath())
    mPrMemb(this,unx_data_pr_.fullPath())
    const ShareData* sd = &sharedata_;
    strm << "-- -- Global share data:\n";
    mPrMemb(sd,drive_)
    mPrMemb(sd,share_)
    mPrMemb(sd,pass_)
    if ( sd->host_ )
	mPrMemb(sd,host_->name())
    else
	strm << "-- No sharedata_->host_\n";
    strm << "--\n-- -- Host data:\n--\n";
    for ( int idx=0; idx<size(); idx++ )
    {
	const HostData* hd = (*this)[idx];
	mPrMemb(hd,name_)
	mPrMemb(hd,iswin_)
	mPrMemb(hd,appl_pr_.fullPath())
	mPrMemb(hd,data_pr_.fullPath())
	mPrMemb(hd,pass_)
	if ( hd->localhd_ )
	    mPrMemb(hd,localhd_->name())
	else
	    strm << "-- No localhd_\n";
	sd = hd->sharedata_;
	if ( sd )
	{
	    strm << "-- -- -- Share data:\n";
	    mPrMemb(sd,drive_)
	    mPrMemb(sd,share_)
	    mPrMemb(sd,pass_)
	    if ( sd->host_ )
		mPrMemb(sd,host_->name())
	    else
		strm << "-- No sharedata_->host_\n";
	}
	strm << "--\n";
    }
    strm.flush();
}


void HostDataList::handleLocal()
{
    const int sz = size();
    const char* localhoststd = "localhost";
    HostData* localhd = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];

	if ( hd->isKnownAs(localhoststd)
	  || hd->isKnownAs("localhost.localdomain") )
	{
	    // Ensure this is the first entry
	    if ( idx != 0 )
		swap( idx, 0 );

	    localhd = hd;
	    break;
	}
	else if ( hd->isKnownAs(HostData::localHostName()) )
	{
	    hd->addAlias( localhoststd );

	    // Ensure this is the first entry
	    if ( idx != 0 )
		swap( idx, 0 );

	    localhd = hd;
	    break;
	}
    }

    BufferString hnm( HostData::localHostName() );
    if ( hnm.isEmpty() ) return;
    if ( !localhd )
    {
	localhd = new HostData( hnm, __iswin__ );
	localhd->addAlias( localhoststd );
	insertAt( localhd, 0 );
    }

    HostData& lochd = *(*this)[0];
    if ( hnm != lochd.name() )
    {
	BufferString oldnm = lochd.name();
	lochd.name_ = hnm;
	lochd.addAlias( oldnm );
	for ( int idx=0; idx<lochd.aliases_.size(); idx++ )
	{
	    BufferString* al = lochd.aliases_[idx];
	    if ( *al == hnm )
		lochd.aliases_ -= al;
	}
    }

    for ( int idx=1; idx<size(); idx++ )
    {
	HostData* hd = (*this)[idx];
	hd->setLocalHost( *localhd );

	if ( hd->isKnownAs(hnm) )
	{
	    lochd.addAlias( hd->name() );
	    for ( int idy=0; idy<hd->aliases_.size(); idy++ )
		lochd.addAlias( *hd->aliases_[idy] );
	    *this -= hd;
	    idx--;
	}
    }
}


HostData* HostDataList::findHost( const char* nm ) const
{
    const HostData* ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->isKnownAs(nm) )
	    { ret = (*this)[idx]; break; }
    }
    if ( !ret )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    if ( (*this)[idx]->getFullDispString() == nm )
		{ ret = (*this)[idx]; break; }
	}
    }
    return const_cast<HostData*>( ret );
}


void HostDataList::fill( BufferStringSet& bss, bool inclocalhost ) const
{
    for ( int idx=(inclocalhost?0:1); idx<size(); idx++ )
	bss.add( (*this)[idx]->getFullDispString() );
}
