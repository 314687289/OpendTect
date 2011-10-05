/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id: mantistables.cc,v 1.9 2011-10-05 11:03:35 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "mantistables.h"

#include "bufstringset.h"
#include "odplatform.h"
#include "mantisdatabase.h"
#include "string2.h"

//BugTextTableEntry
const char* SqlDB::BugTextTableEntry::sKeyBugTextTable()
{ return "mantis_bug_text_table"; }


SqlDB::BugTextTableEntry::BugTextTableEntry()
    : history_(0)
{
    init();
}


void SqlDB::BugTextTableEntry::deleteHistory()
{
    history_ = 0;
}


void SqlDB::BugTextTableEntry::getQueryInfo( BufferStringSet& colnms,
				      BufferStringSet& values )
{
    colnms.add( "description" );
    values.add( description_ );
}


void SqlDB::BugTextTableEntry::init()
{
    description_ = "";
}


void SqlDB::BugTextTableEntry::setDescription( BufferString& desc )
{
    if ( desc == description_ )
	return;

    addToHistory( "description" );
    description_ = desc;
}


void SqlDB::BugTextTableEntry::addToHistory( const char* fldnm )
{
    if ( history_ )
	return;

    history_ = new BugHistoryTableEntry();
    history_->fieldnm_ = fldnm;
    history_->type_ = 6;
}


//BugTableEntry
const char* SqlDB::BugTableEntry::sKeyBugTable() { return "mantis_bug_table"; }
const char* SqlDB::BugTableEntry::sKeyFixedInVersion()
{ return "fixed_in_version"; }
const char* SqlDB::BugTableEntry::sKeySevere()
{ return "Severe"; }
const char* SqlDB::BugTableEntry::sKeyMinor()
{ return "Minor"; }
const int SqlDB::BugTableEntry::cStatusNew() { return 10; }
const int SqlDB::BugTableEntry::cStatusAssigned() { return 50; }
const int SqlDB::BugTableEntry::cStatusResolved() { return 80; }
const int SqlDB::BugTableEntry::cResolutionOpen() { return 10; }
const int SqlDB::BugTableEntry::cResolutionFixed() { return 20; }
const int SqlDB::BugTableEntry::cResolutionWillNotFixed() { return 90; }
const int SqlDB::BugTableEntry::cSeverityFeature() { return 10; }
const int SqlDB::BugTableEntry::cSeverityTrivial() { return 20; }
const int SqlDB::BugTableEntry::cSeverityText() { return 30; }
const int SqlDB::BugTableEntry::cSeverityTweak() { return 40; }
const int SqlDB::BugTableEntry::cSeverityMinor() { return 50; }
const int SqlDB::BugTableEntry::cSeverityMajor() { return 60; }
const int SqlDB::BugTableEntry::cSeverityCrash() { return 70; }
const int SqlDB::BugTableEntry::cSeverityBlock() { return 80; }


SqlDB::BugTableEntry::BugTableEntry()
{
    init();
}


void SqlDB::BugTableEntry::init()
{
    id_ = 0;
    projectid_ = MantisDBMgr::cOpenDtectProjectID();
    reporterid_ = 0;
    handlerid_ = 0;
    severity_ = cSeverityMinor();
    status_ = cStatusNew();
    resolution_ = cResolutionOpen();
    category_ = "";
    date_ = "";
    platform_ = OD::Platform().longName();
    version_ = "";
    fixedinversion_ = "";
    summary_ = "";
}


void SqlDB::BugTableEntry::getQueryInfo( BufferStringSet& colnms,
				  BufferStringSet& values, bool isedit )
{
    if ( !isedit )
    {
	colnms.add( "project_id" ).add( "reporter_id" ).add( "date_submitted" )
	      .add( "summary" ).add( "category" );

	values.add( toString(projectid_) );
	values.add( toString(reporterid_) );
	values.add( date_ ).add( summary_ ).add( category_ );
    }

    colnms.add( "handler_id" ).add( "severity" ).add ( "status" )
	  .add( "resolution" ).add( "last_updated" )
	  .add( "platform" ).add( "version" ).add( "fixed_in_version" );

    values.add( toString(handlerid_) );
    values.add( toString(severity_) );
    values.add( toString(status_) );
    values.add( toString(resolution_) );
    values.add( date_ ).add( platform_ ).add( version_ ).add( fixedinversion_ );
}


void SqlDB::BugTableEntry::addToHistory( const char* fldnm, const char* oldval,
				  const char* newval )
{
    bool isexisted = false;
    for ( int idx=0; idx<historyset_.size(); idx++ )
    {
	BufferString fieldnm = historyset_[idx]->fieldnm_;
	if ( strcmp(fldnm,fieldnm.buf()) == 0 )
	{
	    historyset_[idx]->newvalue_ = newval;
	    isexisted = true;
	    break;
	}
    }

    if ( !isexisted )
    {
	BugHistoryTableEntry* historyentry = new BugHistoryTableEntry;
	historyentry->bugid_ = id_;
	historyentry->fieldnm_ = fldnm;
	historyentry->oldvalue_ = oldval;
	historyentry->newvalue_ = newval;
	historyset_ += historyentry;
    }
}


void SqlDB::BugTableEntry::deleteHistory()
{
    deepErase( historyset_ );
}


void SqlDB::BugTableEntry::setSeverity( int val )
{
    if ( val == severity_ ) return;

    BufferString oldvalue( toString(severity_) );
    addToHistory( "severity", oldvalue, toString(val) );
    severity_ = val;
}


void SqlDB::BugTableEntry::setHandlerID( int hid )
{
    if ( hid == handlerid_ )
	return;

    BufferString oldhid( toString(handlerid_) );
    addToHistory( "handler_id", oldhid, toString(hid) );
    handlerid_ = hid;
}


void SqlDB::BugTableEntry::setStatus( int status )
{
    if ( status_ == status )
	return;

    BufferString oldstatus( toString(status_) );
    addToHistory( "status", oldstatus, toString(status) );
    status_ = status;
}


void SqlDB::BugTableEntry::setResolution( int resolution )
{
    if ( resolution_ == resolution )
	return;

    BufferString oldres( toString(resolution_) );
    addToHistory( "resolution", oldres, toString(resolution) );
    resolution_ = resolution;
}


void SqlDB::BugTableEntry::setPlatform( const char* plf )
{
    if ( platform_ == plf )
	return;

    addToHistory( "platform", platform_, plf );
    platform_ = plf;
}


void SqlDB::BugTableEntry::setVersion( const char* version )
{
    if ( version_ == version )
	return;

    addToHistory( "version", version_, version );
    version_ = version;
}


bool SqlDB::BugTableEntry::isSevere( int severity )
{
    return severity == cSeverityFeature() || severity == cSeverityTrivial() ||
	   severity == cSeverityText() || severity == cSeverityTweak() ||
	   severity == cSeverityMinor() ? false : true;
}


//BugHistoryTableEntry
const char* SqlDB::BugHistoryTableEntry::sKeyBugHistoryTable()
{ return "mantis_bug_history_table"; }


SqlDB::BugHistoryTableEntry::BugHistoryTableEntry()
{
    init();
}


void SqlDB::BugHistoryTableEntry::init()
{
    userid_ = 0;
    bugid_ = 0;
    date_ = "";
    fieldnm_ = "";
    oldvalue_ = "";
    newvalue_ = "";
    type_ = 0;
}


void SqlDB::BugHistoryTableEntry::getQueryInfo( BufferStringSet& colnms,
					 BufferStringSet& values )
{
    colnms.add( "user_id" ).add( "bug_id" ).add( "date_modified" )
	  .add( "field_name" ).add( "old_value" ).add( "new_value" )
	  .add( "type" );

    values.add( toString(userid_) );
    values.add( toString(bugid_) );
    values.add( date_ ).add( fieldnm_ ).add( oldvalue_ ).add( newvalue_ );
    values.add( toString(type_) );
}
