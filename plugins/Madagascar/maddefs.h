#ifndef maddefs_h
#define maddefs_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: maddefs.h,v 1.6 2009-04-06 07:22:13 cvsranojay Exp $
-*/

#include "bufstringset.h"

class Executor;

namespace ODMad
{

class ProgDef
{
public:

    BufferString		name_;
    BufferString		shortdesc_;
    BufferString		synopsis_;
    BufferString		comment_;
    const BufferString*		group_; //!< Never null

};


/* Scans $RSFROOT/doc/txt directory for program definitions */

mClass ProgInfo
{
public:
    			//!< When PI() is first used, a Pre-Scan is done
    			//!< if Pre-Scan check is OK, err msg will be empty

    bool		scanned() const		{ return scanned_; }
    Executor*		getScanner() const;
    			//!< If scan fails, err msg will be filled

    const BufferString&	errMsg() const		{ return errmsg_; }

    const ObjectSet<ProgDef>&	defs() const	{ return defs_; }
    const BufferStringSet&	groups() const	{ return groups_; }
    void			search(const char* globexpr,
	    			       ObjectSet<const ProgDef>&) const;
    				//!< name_, shortdesc_, comment_

			ProgInfo();
			~ProgInfo()		{ cleanUp(); }

protected:

    BufferString	rsfroot_;
    BufferString	defdir_;
    BufferString	errmsg_;
    bool		scanned_;
    BufferStringSet	groups_;
    ObjectSet<ProgDef>	defs_;

    void		cleanUp();
    void		doPreScanCheck();
    void		addEntry(const char*);

    friend class	ProgInfoScanner;
    mGlobal friend	ProgInfo& PI();      
};

mGlobal ProgInfo& PI();



} // namespace ODMad

#endif
