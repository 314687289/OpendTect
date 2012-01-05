#ifndef mantisdatabase_h
#define mantisdatabase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: mantisdatabase.h,v 1.20 2012-01-05 06:56:59 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "sqldatabase.h"
#include "sqlquery.h"
#include "bufstringset.h"
#include "typeset.h"


namespace SqlDB
{
class BugTableEntry;
class BugTextTableEntry;
class BugHistoryTableEntry;


mClass MantisAccess : public MySqlAccess
{
public:

    			MantisAccess()
			    : MySqlAccess("Mantis")	{}

};


mClass MantisQuery : public Query
{
public:
			MantisQuery(MantisAccess&);

    bool		updateTables(BugTableEntry&,BugTextTableEntry&);

};


mClass MantisDBMgr
{
public:

    				MantisDBMgr(const ConnectionData* cd=0);
    				~MantisDBMgr();

    inline MantisAccess&	access() 	{ return acc_; }
    inline const MantisAccess&	access() const 	{ return acc_; }
    inline MantisQuery&		query()		{ return *query_; }
    inline const MantisQuery&	query() const	{ return *query_; }
    bool			isOK() const	{ return acc_.isOpen(); }
    const char*			errMsg() const;

    int				getUserID(bool isdeveloper) const;
    int				getMaxBugIDFromBugTable() const;
    int				getMaxNoteIDFromBugNoteTable() const;
    int				getMaxNoteIDFromBugNoteTextTable() const;
    BugTableEntry*		getBugTableEntry(int tableidx);
    BugTextTableEntry*		getBugTextTableEntry(int tableidx);
    inline int			nrBugs() const		{ return bugs_.size(); }
    const TypeSet<int>&		userIDs() const		{ return userids_; }
    const TypeSet<int>&		projectIDs() const	{ return projectids_; }
    const TypeSet<int>&		severityVals() const
    				{ return severityvals_; }
    const BufferStringSet&	developers() const	{ return developers_; }
    const BufferStringSet&	userNames() const	{ return usernames_; }
    const BufferStringSet&	categories() const	{ return categories_; }
    const BufferStringSet&	projects() const	{ return projectnms_; }
    void			getProjnm(int projid,
	    				  BufferString& projnm) const;
    const BufferStringSet&	severities() const	{ return sevirities_; }
    const BufferStringSet&	atchfiles() const	{return attachfilenms_;}
    const TypeSet<int>&		attachedids() const	{ return attachids_; }
    static void			editVersions(const BufferStringSet&,
	    				     BufferStringSet&,
					     bool ismajor=true,
					     bool isalladd=true);
    static void			parseVersion(const BufferString& fullver,
	    				     BufferString& numver,
					     BufferString& patch);
    bool			addBug(BugTableEntry&,BugTextTableEntry&,
	    			       const char* note);
    bool			editBug(BugTableEntry&,BugTextTableEntry&,
	    				const char* note);
    bool			deleteBug(int id);
    void			addBugTableEntryToSet(BugTableEntry&);
    void			addBugTextTableEntryToSet(BugTextTableEntry&);
    void			removeBugTableEntryFromSet(int tableidx);
    void			removeBugTextTableEntryFromSet(int tableidx);

    void			eraseCurrentEntries();
    bool			updateBugTableEntryHistory(int idx,bool isadded,
							   bool isnoteempty);
    void			updateBugTextTableEntryHistory(int idx);
    int				getBugTableIdx(int bugid);
    bool			getInfoFromTables();
    bool			fillBugsIdx(const char* projnm,
	    				    const char* usernm,bool onlyfixed,
	    				    TypeSet<int>& bugsassigned );
    TypeSet<int>&		getBugsIndex();
    static void			prepareForQuery(BufferString&);
    const BufferStringSet*	getVersions(const char* projnm) const;
    const BufferStringSet*	getVersions( int projid ) const;
    void			getAllVersions(BufferStringSet&) const;

    bool			getNotesInfo(int bugid,TypeSet<int>& noteids,
	    				     BufferStringSet& notes);

    static const char* 	sKeyAll();
    static const char* 	sKeyUnAssigned();
    static const char*	sKeyBugNoteTable();
    static const char*	sKeyBugNoteTextTable();
    static const char*	sKeyProjectCategoryTable();
    static const char*	sKeyUserTable();
    static const char*	sKeyProjectUserListTable();
    static const char*	sKeyProjectVersionTable();
    static const char*	sKeyBugFileTable();
    static const char*	sKeyProjectTable();
    static const int 	cOpenDtectProjectID();
    static const int 	cAccessLevelDeveloper();
    static const int 	cAccessLevelCaseStudy();


protected:

    bool		addToBugTable(BugTableEntry&);
    bool		addToBugTextTable(BugTextTableEntry&);
    bool		addToBugNoteTable(const char*,int);
    bool		addToBugNoteTextTable(const char*);
    bool		fillCategories();
    bool		fillBugTableEntries();
    bool		fillProjectsInfo();
    bool		fillUsersInfo();
    bool		fillVersionsByProject();
    void		fillSeverity();
    bool		fillAttachedFilesInfo();
    void		addHistoryToSet(BugHistoryTableEntry&);

    bool		updateBugHistoryTable(ObjectSet<BugHistoryTableEntry>&,
	    				      bool isadded);

    bool		deleteBugHistory(int id);
    bool		deleteBugNoteInfo(int id);
    bool		deleteBugTableInfo(int id);

    MantisAccess	acc_;
    mutable MantisQuery* query_;

    TypeSet<int>	userids_;
    TypeSet<int>	projectids_;
    BufferStringSet	projectnms_;
    BufferStringSet	usernames_;
    BufferStringSet	developers_;
    BufferStringSet	categories_;
    BufferStringSet	sevirities_;
    BugTableEntry*	bugtable_;
    BugTextTableEntry*	bugtexttable_;
    ObjectSet<BugTableEntry>	bugs_;
    ObjectSet<BugTextTableEntry> texttables_;
    TypeSet<int>	bugsindex_;
    TypeSet<int>	severityvals_;
    TypeSet<int>	attachids_;
    BufferStringSet	attachfilenms_;

    mutable BufferString errmsg_;
    ObjectSet<BufferStringSet>	versionsbyproject_;
};


} // namespace

#endif
