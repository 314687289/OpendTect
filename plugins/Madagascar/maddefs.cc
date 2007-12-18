/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: maddefs.cc,v 1.9 2007-12-18 16:21:00 cvsbert Exp $";

#include "maddefs.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "dirlist.h"
#include "strmprov.h"
#include "executor.h"
#include "globexpr.h"
#include <iostream>


ODMad::ProgInfo::ProgInfo()
    	: scanned_(false)
{
    rsfroot_ = GetEnvVar( "RSFROOT" );
    doPreScanCheck();
}


void ODMad::ProgInfo::cleanUp()
{
    deepErase( defs_ );
    groups_.deepErase();
}


void ODMad::ProgInfo::doPreScanCheck()
{
    errmsg_ = "";

    if ( rsfroot_.isEmpty() )
    {
	errmsg_ = "RSFROOT not set - no program definitions available";
	return;
    }
    if ( !File_isDirectory(rsfroot_) )
    {
	errmsg_ = "$RSFROOT (Madagascar) is invalid:\n";
	errmsg_ += rsfroot_;
	errmsg_ += "\nThe directory does not exist or cannot be read";
	return;
    }

    FilePath fp( rsfroot_ ); fp.add( "defs" );
    defdir_ = fp.fullPath();

    if ( !File_isDirectory(defdir_) )
    {
	errmsg_ = "Madagascar installation not prepared. Directory:\n";
	errmsg_ += defdir_;
	errmsg_ += "\ndoes not exist. You need to issue the command:\n"
		   "$RSFROOT/bin/sfdoc -t $RSFROOT/defs\n";
    }
}


namespace ODMad
{

class ProgInfoScanner : public Executor
{
public:

ProgInfoScanner( ODMad::ProgInfo& pi )
    : Executor("Scanning Madagascar program definitions")
    , pi_(pi)
    , dl_(0)
    , totnr_(-1)
    , curnr_(0)
{
    pi_.cleanUp(); pi_.doPreScanCheck();
    pi_.scanned_ = true;
    if ( !pi_.errmsg_.isEmpty() ) return;

    dl_ = new DirList( pi_.defdir_, DirList::FilesOnly, "*.txt" );
    totnr_ = dl_->size();
    if ( totnr_ < 1 )
    {
	pi_.errmsg_ = "Madagascar Definition directory:\n";
	pi_.errmsg_ += pi_.defdir_;
	pi_.errmsg_ += "\ncontains no definition files";
	delete dl_; dl_ = 0;
    }
}

~ProgInfoScanner()
{
    delete dl_;
}

const char* message() const
{
    if ( !dl_ )
	return pi_.errMsg();
    if ( curnr_ < 0 || curnr_ >= dl_->size() )
	return "Scanning files";

    msg_ = "Scanning "; msg_ += dl_->get( curnr_ );
    return msg_;
}

const char*	nrDoneText() const	{ return "Files scanned"; }
int		nrDone() const		{ return curnr_; }
int		totalNr() const		{ return totnr_; }

int nextStep()
{
    if ( !dl_ )
	return Executor::ErrorOccurred;
    if ( curnr_ >= totnr_ )
	return Executor::Finished;

    pi_.addEntry( dl_->fullPath(curnr_) );
    curnr_++;

    return curnr_ >= totnr_ ? Executor::Finished : Executor::MoreToDo;
}

    ODMad::ProgInfo&	pi_;
    DirList*		dl_;
    int			totnr_;
    int			curnr_;
    mutable BufferString msg_;

};

}


Executor* ODMad::ProgInfo::getScanner() const
{
    return new ProgInfoScanner( const_cast<ODMad::ProgInfo&>( *this ) );
}


void ODMad::ProgInfo::search( const char* str,
			      ObjectSet<const ProgDef>& res ) const
{
    if ( !str || !*str || !scanned_ ) return;

    BufferString gestr;
    if ( strchr(str,'*') )
	gestr = str;
    else
	{ gestr = "*"; gestr += str; gestr += "*"; }

    const GlobExpr ge( gestr, false );
    for ( int ityp=0; ityp<3; ityp++ )
    {
    for ( int idx=0; idx<defs_.size(); idx++ )
    {
	const ProgDef* def = defs_[idx];
	const BufferString& matchstr( ityp == 0 ? def->name_
				   : (ityp == 1 ? def->shortdesc_
						: def->comment_) );
	if ( ge.matches(matchstr) && !res[def] )
	    res += def;
    }
    }
}



void ODMad::ProgInfo::addEntry( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() ) return;
    ODMad::ProgDef* def = new ODMad::ProgDef;

    static char buf[1024];
    sd.istrm->getline( buf, 1024 );

    char* ptr = buf; ptr += 8; // Skip 'Program '
    char* word = ptr;
    while ( *ptr && !isspace(*ptr) ) ptr++;
    if ( !*ptr ) { sd.close(); delete def; return; }
    *ptr = '\0'; def->name_ = word;

    ptr += 3; def->shortdesc_ = ptr;

    bool buildingcomment = false;
    BufferString tmp;
    while ( sd.istrm->getline( buf, 1024 ) )
    {
	if ( buf[0] != '[' )
	{
	    if ( buildingcomment )
	    {
		tmp = buf; tmp += "\n";
		def->comment_ += tmp;
	    }
	    continue;
	}

	const bool iscomment = !strcmp(buf,"[COMMENTS]");
	const bool isparams = !strcmp(buf,"[PARAMETERS]");
	if ( isparams )
	    { def->comment_ += "\n\nParameters:\n"; }
	    //TODO use PARAMETERS properly
	buildingcomment = iscomment || isparams;
	if ( buildingcomment )
	    continue;

	if ( !strcmp(buf,"[SYNOPSIS]") )
	{
	    sd.istrm->getline( buf, 1024 );
	    def->synopsis_ = buf;
	}
	else if ( !strcmp(buf,"[DIRECTORY]") )
	{
	    sd.istrm->getline( buf, 1024 );
	    groups_.addIfNew( buf );
	    def->group_ = find( groups_, buf );
	}
    }
    sd.close();

    if ( !def->group_ )
    {
	groups_.addIfNew( "<unknown>" );
	def->group_ = find( groups_, "<unknown>" );
    }

    defs_ += def;
}
