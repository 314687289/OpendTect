#ifndef madprocflow_h
#define madprocflow_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
 * ID       : $Id: madprocflow.h,v 1.4 2009-03-31 06:10:21 cvsraman Exp $
-*/

#include "iopar.h"
#include "madproc.h"
#include "namedobj.h"
#include "bufstringset.h"


namespace ODMad
{

class ProcFlow : public ::NamedObject
	       , public ObjectSet<Proc>
{
public:

    enum IOType		{ Vol, VolPS, Line, LinePS, Madagascar, SU, None };

    			ProcFlow(const char* nm=0);
    			~ProcFlow();

    IOPar&		input()			{ return inpiop_; }
    const IOPar&	input() const		{ return inpiop_; }
    const IOPar&	output() const		{ return outiop_; }
    IOPar&		output() 		{ return outiop_; }

    IOType		ioType( bool inp ) const
    			{ return ioType( inp ? inpiop_ : outiop_ ); }
    void		setIOType( bool inp, IOType iot )
    			{ setIOType( inp ? inpiop_ : outiop_, iot ); }

    bool		isOK(BufferString&) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static IOType	ioType(const IOPar&);
    static void		setIOType(IOPar&,IOType);

    static const char*	sKeyInp();
    static const char*	sKeyOutp();
    static const char*	sKeyProc();
    static const char*	sKeyNrProcs();

protected:

    IOPar		inpiop_;
    IOPar		outiop_;
};

} // namespace ODMad

#endif
