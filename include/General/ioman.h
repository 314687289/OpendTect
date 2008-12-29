#ifndef ioman_h
#define ioman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		3-8-1995
 RCS:		$Id: ioman.h,v 1.36 2008-12-29 11:14:26 cvsranojay Exp $
________________________________________________________________________

-*/
 

#include "namedobj.h"
#include "multiid.h"
#include "sets.h"

class IODir;
class IOObj;
class IOLink;
class CtxtIOObj;
class Translator;
class IOObjContext;

class IOMan;

/*!\brief manages the 'Meta-'data store for the IOObj's. This info
is read from the .omf files.

There will be one IOMan available through the global function IOM(). Creating
more instances is probably not a good idea, but it may work.

*/

mClass IOMan : public NamedObject
{
public:

    bool		bad() const		{ return state_ != Good; }
    bool		isReady() const;

			//! Next functions return a new (unmanaged) IOObj
    IOObj*		get(const MultiID&) const;
    IOObj*		getLocal(const char* objname) const;
    IOObj*		getOfGroup(const char* tgname,bool first=true,
	    			   bool onlyifsingle=false) const;
    IOObj*		getIfOnlyOne( const char* trgroupname )
			{ return getOfGroup(trgroupname,true,true); }
    IOObj*		getByName(const char* objname,
			      const char* partrgname=0,const char* parname=0);
    IOObj*		getFirst(const IOObjContext&,int* nrpresent=0) const;
    			//!< if interested in nrpresent pass valid address

    IODir*		dirPtr()		{ return dirptr; }
    const IODir*	dirPtr() const		{ return (IODir*)dirptr; }
    MultiID		key() const;		//!< of current IODir
    const char*		curDir() const;		//!< OS dir name
    int			curLevel() const	{ return curlvl; }
    const char*		rootDir() const		{ return rootdir; }
    int			levelOf(const char* dirnm) const;
    const char*		nameOf(const char* keystr,bool inc_parents=false) const;

    bool		to(const IOLink*);	//!< NULL -> up-dir
    bool		to(const MultiID&);
    void		back();

    void		getEntry(CtxtIOObj&,bool newistmp=false);
			//!< will create a new entry if necessary
    bool		haveEntries(const MultiID& dirid,const char* trgrpnm=0,
				     const char* trnm=0) const;
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);

    const char*		surveyName() const;
    static bool		newSurvey();
			/*!< if an external source has changed
				the .od/survey, force re-read it. */
    static void		setSurvey(const char*);
			/*!< will remove existing IO manager and
			     set the survey to 'name', thus bypassing the
			     .od/survey file */

    mClass CustomDirData
    {
    public:
			CustomDirData( const char* selkey, const char* dirnm,
					const char* desc="Custom data" )
			    : selkey_(selkey)
			    , dirname_(dirnm)
			    , desc_(desc)		{}

	MultiID		selkey_; //!< Make sure your selkey > 200000
				 //!< Lower than that will be refused!
				 //!< Example: "218745"
	BufferString	dirname_; //!< The subdirectory name in the tree
	BufferString	desc_; //!< Short description, mainly for error messages
			       //!< Example: "Geostatistical data"

	bool		operator ==( const CustomDirData& cdd ) const
	    		{ return selkey_ == cdd.selkey_; }
    };

    static const MultiID& addCustomDataDir(const CustomDirData&);
    			//!< Need to do this only once per OD run
    			//!< At survey change, dir will automatically be added

    bool		setRootDir(const char*);
    bool		setFileName(MultiID,const char*);
    const char*		generateFileName(Translator*,const char*);
    static bool		validSurveySetup(BufferString& errmsg);
    MultiID		newKey() const;

    Notifier<IOMan>	newIODir;
    Notifier<IOMan>	entryRemoved;	   // CallBacker: CBCapsule<MultiID>
    Notifier<IOMan>	surveyToBeChanged; // Before the change
    Notifier<IOMan>	surveyChanged;     // These restore OD to normal state
    Notifier<IOMan>	afterSurveyChange; // These operate in normal state

private:

    enum State		{ Bad, NeedInit, Good };
    State		state_;
    IODir*		dirptr;
    int			curlvl;
    MultiID		prevkey;
    FileNameString	rootdir;

    void		init();
			IOMan(const char* rd=0);
    			~IOMan();

    static IOMan*	theinst_;
    static void		setupCustomDataDirs(int);

    bool		setDir(const char*);

    friend class	IOObj;
    friend class	IODir;
    friend mGlobal	IOMan&	IOM();

};

mGlobal IOMan&	IOM();


/*!\mainpage
  \section Introduction Introduction

  This module uses the services from the Basic module and adds services that
  are (in general) more or less OpendTect specific. Just like the Basic module
  the services are used by all other modules.


  \section Content Content
  Some of the groups of services are:

<ul>
 <li>I/O management system
  <ul>
   <li>ioman.h : the IOM() object of the IOMan class provides a lookup of
       objects in the data store
   <li>ioobj.h : Subclasses of IOObj hold all data necessary to access a stored
       object.
   <li>iostrm.h : IOStream is the most common subclass of IOObj because
       OpendTect stores its data in files.
   <li>ctxtioobj.h : The context of an IOObj selection: what type of object,
       is it for read or write, should the user be able to create a new entry,
       etc.
  </ul>
 <li>Translators
  <ul>
   <li>transl.h : Translators are the objects that know file and database
       formats. All normal data will be put into and written from in-memory
       objects via subclasses of Translator.
  </ul>
 <li>ArrayND utils
  <ul>
   <li>array2dxxx.h : 2-D arrays have a couple of specific things inmplemented
   <li>arrayndxxx.h : slices, subselection and other utilities
  </ul>
 <li>CBVS
  <ul>
   <li>cbvsreadmgr.h : reads the 'Common Basic Volume Storage' format
   <li>cbvswritemgr.h : writes CBSV format.
  </ul>
 <li>Tables
  <ul>
   <li>tabledef.h : Specifying the information content of tabular data
   <li>tableascio.h : Utiltities to read/write table-formatted data
   <li>tableconv.h : Utilities for converting table data
  </ul>
 <li>Properties and units
  <ul>
   <li>property.h : handling properties like Density, Velocity, ...
   <li>uniofmeasure.h : handling units of measure
  </ul>
</ul>

*/


#endif
