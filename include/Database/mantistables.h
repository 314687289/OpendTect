#ifndef mantistables_h
#define mantistables_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id: mantistables.h,v 1.5 2011-09-19 10:04:03 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "typeset.h"
#include "objectset.h"

class BufferString;
class BufferStringSet;


namespace SqlDB
{

mClass BugHistoryTableEntry
{
public:    

    			BugHistoryTableEntry();

    static const char*	sKeyBugHistoryTable();
    void		init();
    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values);

    int			userid_;
    int			bugid_;
    BufferString	date_;
    BufferString	fieldnm_;
    BufferString	oldvalue_;
    BufferString	newvalue_;
    int			type_;

};


mClass BugTextTableEntry
{
public:

    			BugTextTableEntry();

    static const char*  sKeyBugTextTable();

    void		init();
    void		setDescription(BufferString& desc);
    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values);
    void		addToHistory(const char* fieldnm);
    void		deleteHistory();
    BugHistoryTableEntry* getHistory()		{ return history_; }

    BufferString	description_;

protected:

    BugHistoryTableEntry* history_;

};


mClass BugTableEntry
{
public:

    			BugTableEntry();

    static const char*  sKeyBugTable();
    static const char*	sKeyFixedInVersion();
    static const char*	sKeySevere();
    static const char*	sKeyMinor();
    static const int    cStatusNew();
    static const int    cStatusAssigned();
    static const int    cStatusResolved();
    static const int    cResolutionOpen();
    static const int    cResolutionFixed();
    static const int    cSeverityFeature();
    static const int    cSeverityTrivial();
    static const int    cSeverityText();
    static const int    cSeverityTweak();
    static const int    cSeverityMinor();
    static const int    cSeverityMajor();
    static const int    cSeverityCrash();
    static const int    cSeverityBlock();

    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values,bool isedit);
    void		init();
    void		setSeverity(int);
    void		setHandlerID(int);
    void		setStatus(int);
    void		setPlatform(const char* plf);
    void		setVersion(const char* version);
    void		setResolution(int resolution);
    void		addToHistory(const char* fldnm,const char* oldval,
	    			     const char* newval);
    void		deleteHistory();
    ObjectSet<BugHistoryTableEntry>&	getHistory()	{ return historyset_; }
    static bool		isSevere(int);

    int			id_;
    int			projectid_;
    int			reporterid_;
    int			handlerid_;
    int			severity_;
    int			status_;
    int			resolution_;
    BufferString	category_;
    BufferString	date_;
    BufferString	platform_;
    BufferString	version_;
    BufferString	fixedinversion_;
    BufferString	summary_;

protected:

    ObjectSet<BugHistoryTableEntry> historyset_;

};


} // namespace

#endif
