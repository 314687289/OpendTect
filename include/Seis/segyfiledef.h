#ifndef segyfiledef_h
#define segyfiledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledef.h,v 1.11 2008-12-29 11:24:59 cvsranojay Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "bufstring.h"
#include "position.h"
#include "samplingdata.h"
#include "seistype.h"
#include "segythdef.h"
class IOPar;
class IOObj;
class Scaler;
 

namespace SEGY
{


/*\brief Base class for SEG-Y parameter classes  */

mClass FileDef
{
public:
    			FileDef()		{}

    virtual void	fillPar(IOPar&) const				= 0;
    virtual void	usePar(const IOPar&)				= 0;
    virtual void	getReport(IOPar&,bool isrev1=true) const	= 0;

    static const char*	sKeySEGYRev;
    static const char*	sKeyForceRev0;

};


/*\brief Definition of input and output file(s)  */

mClass FileSpec
{
public:
    			FileSpec( const char* fnm=0 )
			    : fname_(fnm)
			    , nrs_(mUdf(int),0,1)
			    , zeropad_(0)	{}

    BufferString	fname_;
    StepInterval<int>	nrs_;
    int			zeropad_;	//!< pad zeros to this length

    bool		isMultiFile() const	{ return !mIsUdf(nrs_.start); }
    int			nrFiles() const	
    			{ return isMultiFile() ? nrs_.nrSteps()+1 : 1; }
    const char*		getFileName(int nr=0) const;
    IOObj*		getIOObj(bool temporary=true) const;

    void		getMultiFromString(const char*);
    static const char*	sKeyFileNrs;

    static void		ensureWellDefined(IOObj&);
    static void		fillParFromIOObj(const IOObj&,IOPar&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    virtual void	getReport(IOPar&,bool) const;

};


/*\brief Parameters that control the primary read/write process */

mClass FilePars
{
public:
    			FilePars( bool forread=true )
			    : ns_(0)
			    , fmt_(forread?0:1)
			    , byteswap_(0)
			    , forread_(forread)		{}

    int			ns_;
    int			fmt_;
    int			byteswap_;	//!, 0=no 1=data only 2=all

    static int		nrFmts( bool forread )	{ return forread ? 6 : 5; }
    static const char**	getFmts(bool forread);
    static const char*	nameOfFmt(int fmt,bool forread);
    static int		fmtOf(const char*,bool forread);

    static const char*	sKeyNrSamples;
    static const char*	sKeyNumberFormat;
    static const char*	sKeyByteSwap;

    void		setForRead(bool);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    virtual void	getReport(IOPar&,bool) const;

protected:

    bool		forread_;

};


/*\brief Options that control the actual read process */

mClass FileReadOpts
{
public:
    			FileReadOpts( Seis::GeomType gt=Seis::Vol )
			    : forread_(true)
			    , offsdef_(0,1)
			    , coordscale_(mUdf(float))
			    , timeshift_(mUdf(float))
			    , sampleintv_(mUdf(float))
			    , psdef_(InFile)
			    , icdef_(Both)
			{ setGeomType(gt); thdef_.fromSettings(); }

    Seis::GeomType	geomType() const	{ return geom_; }
    void		setGeomType(Seis::GeomType);

    enum ICvsXYType	{ Both=0, ICOnly=1, XYOnly=2 };
    enum PSDefType	{ InFile=0, SrcRcvCoords=1, UsrDef=2 };

    TrcHeaderDef	thdef_;
    ICvsXYType		icdef_;
    PSDefType		psdef_;
    SamplingData<int>	offsdef_;
    float		coordscale_;
    float		timeshift_;
    float		sampleintv_;
    bool		forread_;

    void		scaleCoord(Coord&,const Scaler* s=0) const;
    float		timeShift(float) const;
    float		sampleIntv(float) const;

    static const char*	sKeyICOpt;
    static const char*	sKeyPSOpt;
    static const char*	sKeyOffsDef;
    static const char*	sKeyCoordScale;
    static const char*	sKeyTimeShift;
    static const char*	sKeySampleIntv;

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    virtual void	getReport(IOPar&,bool) const;

protected:

    Seis::GeomType	geom_;

};

} // namespace


#endif
