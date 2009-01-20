#ifndef madproc_h
#define madproc_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: madproc.h,v 1.1 2009-01-20 12:35:47 cvsraman Exp $
-*/


#include "bufstringset.h"

class IOPar;

namespace ODMad
{

class Proc
{
public:

    enum IOType		{ Vol, VolPS, Line, LinePS, Madagascar, SegY, SU,
			  VPlot, None };

    			Proc(const char* cmd,const char* auxcmd=0);
    			~Proc();

    bool		isValid() const		{ return isvalid_; }
    IOType		inpType() const		{ return inptype_; }
    IOType		outpType() const	{ return outptype_; }
    int			nrPars() const		{ return parstrs_.size(); }
    const char*		progName() const	{ return progname_.buf(); }
    const char*		auxCommand() const	{ return auxcmd_.buf(); }

    const char*		parStr(int) const;
    const char*		getCommand() const;
    const char*		getSummary() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static bool		progExists(const char*);
    static const char*	sKeyCommand()		{ return "Command"; }
    static const char*  sKeyAuxCommand()	{ return "Auxillary Command"; }
protected:

    bool		isvalid_;
    BufferString	progname_;
    BufferStringSet	parstrs_;
    BufferString	auxcmd_;
    IOType		inptype_;
    IOType		outptype_;

    void		makeProc(const char* cmd,const char* auxcmd=0);

};

} // namespace ODMad

#endif
