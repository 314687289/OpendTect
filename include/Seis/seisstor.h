#ifndef seisstor_h
#define seisstor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id: seisstor.h,v 1.16 2005-12-12 18:11:13 cvsbert Exp $
________________________________________________________________________

Trace storage objects handle seismic data storage.

-*/


#include "seisinfo.h"
class Conn;
class IOObj;
class Translator;
class SeisTrcBuf;
class SeisSelData;
class Seis2DLineSet;
class SeisPSIOProvider;
class SeisTrcTranslator;


/*!\brief base class for seis reader and writer. */

class SeisStoreAccess
{
public:

    virtual		~SeisStoreAccess();
    virtual bool	close();

    bool		is2D() const		{ return is2d; }
    const char*		errMsg() const
			{ return errmsg; }
    int			tracesHandled() const
			{ return nrtrcs; }

    const IOObj*	ioObj() const
			{ return ioobj; }
    void		setIOObj(const IOObj*);
    const SeisSelData*	selData() const
			{ return seldata; }
    void		setSelData(SeisSelData*);
			//!< The SeisSelData becomes mine
    int			selectedComponent() const
			{ return selcomp; }
			//!< default = -1 is all components
    void		setSelectedComponent( int i )
			{ selcomp = i; }

    virtual void	usePar(const IOPar&);
				// Afterwards check whether curConn is still OK.
    virtual void	fillPar(IOPar&) const;

    static const char*	sNrTrcs;

    // Note that the Translator is always created, but only actually used for 3D
    Translator*		translator()
			{ return trl; }
    Translator*		translator() const
			{ return trl; }

    // 3D only
    Conn*		curConn3D();
    const Conn*		curConn3D() const;

    // 3D and 2D
    SeisTrcTranslator*	seisTranslator()
			{ return strl(); }
    const SeisTrcTranslator* seisTranslator() const
			{ return strl(); }
    // 2D only
    Seis2DLineSet*	lineSet()
			{ return lset; }
    const Seis2DLineSet* lineSet() const
			{ return lset; }

    // Pre-Stack only
    const SeisPSIOProvider* psIOProv() const
			{ return psioprov; }

protected:

			SeisStoreAccess(const IOObj*);
			SeisStoreAccess(const char*,bool isps=false);
    virtual void	init()			{}
    bool		cleanUp(bool alsoioobj=true);

    IOObj*		ioobj;
    bool		is2d;
    int			nrtrcs;
    int			selcomp;
    Translator*		trl;
    Seis2DLineSet*	lset;
    SeisSelData*	seldata;
    const SeisPSIOProvider* psioprov;
    BufferString	errmsg;

    SeisTrcTranslator*	strl() const;

};


#endif
