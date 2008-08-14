#ifndef gmtpar_h
#define gmtpar_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtpar.h,v 1.3 2008-08-14 10:52:47 cvsraman Exp $
________________________________________________________________________

-*/

#include "iopar.h"
#include "gmtdef.h"


class GMTPar : public IOPar
{
public:
    			GMTPar(const char* nm)
			    : IOPar(nm)	{}
			GMTPar(const IOPar& par)
			    : IOPar(par) {}

    virtual bool	execute(std::ostream&,const char*)		=0;
    virtual const char* userRef() const					=0;
    virtual bool	fillLegendPar(IOPar&) const	{ return false; }
};


typedef GMTPar* (*GMTParCreateFunc)(const IOPar&);

class GMTParFactory
{
public:

    int			add(const char* nm, GMTParCreateFunc);
    GMTPar*		create(const IOPar&) const;

    const char*		name(int) const;
    int			size() const	{ return entries_.size(); }

protected:

    struct Entry
    {
				Entry(	const char* nm,
					GMTParCreateFunc fn )
				    : name_(nm)
				    , crfn_(fn)		{}

	BufferString		name_;
	GMTParCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*) const;

    friend GMTParFactory&	GMTPF();
};

GMTParFactory& GMTPF();


#define mErrStrmRet(s) { strm << s << std::endl; return false; }

#define mGetRangeProjString( str, projkey ) \
    Interval<float> xrg, yrg, mapdim; \
    get( ODGMT::sKeyXRange, xrg ); \
    get( ODGMT::sKeyYRange, yrg ); \
    get( ODGMT::sKeyMapDim, mapdim ); \
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3; \
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3; \
    str = "-R"; str += xrg.start; str += "/"; \
    str += xrg.stop; str += "/"; \
    str += yrg.start; str += "/"; str += yrg.stop; str += " -J"; \
    str += projkey; str += mapdim.start; str += "c/"; \
    str += mapdim.stop; str += "c";

#define mGetColorString( col, str ) \
    str = (int) col.r(); \
    str += "/"; str += (int) col.g(); \
    str += "/"; str += (int) col.b();

#define mGetLineStyleString( ls, str ) \
    str = ls.width_; str += "p,"; \
    BufferString lscol; \
    mGetColorString( ls.color_, lscol ); \
    str += lscol; str += ","; \
    switch ( ls.type_ ) \
    { \
	case LineStyle::Dash: \
	    str += "-"; \
	    break; \
	case LineStyle::Dot: \
	    str += "."; \
	    break; \
	case LineStyle::DashDot: \
	    str += "-."; \
	    break; \
	case LineStyle::DashDotDot: \
	    str += "-.."; \
	    break; \
	default: \
	    break; \
    }

#endif
