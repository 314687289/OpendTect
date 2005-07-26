#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.19 2005-07-26 08:41:38 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "enums.h"
#include "ranges.h"
#include "position.h"
#include "datachar.h"
#include "samplingdata.h"
class SUsegy;
class SeisTrc;
class PosAuxInfo;
class IOPar;

/*!\brief Information for a packet of seismics, AKA tape header info */


class SeisPacketInfo
{
public:

			SeisPacketInfo()	{ clear(); }

    BufferString	usrinfo;
    BufferString	stdinfo;
    int			nr;

    StepInterval<int>	inlrg;
    StepInterval<int>	crlrg;
    StepInterval<float>	zrg;
    bool		inlrev;
    bool		crlrev;

    void		clear();

    static const char*	sBinIDs;
    static const char*	sZRange;

    static BufferString	defaultusrinfo;

};


/*!\brief Information for a seismic trace, AKA trace header info */

class SeisTrcInfo
{
public:
			SeisTrcInfo()
			: sampling(0,defaultSampleInterval()), nr(0)
			, pick(mUdf(float)), refpos(mUdf(float))
			, offset(0), azimuth(0)
			, new_packet(NO), stack_count(1)
			, mute_pos(mUdf(float)), taper_length(0)
			{}

	    // persistent
    SamplingData<float>	sampling;
    int			nr;		// 0
    float		pick;		// 1
    float		refpos;		// 2
    Coord		coord;		// 3 4
    BinID		binid;		// 5 6
    float		offset;		// 7
    float		azimuth;	// 8

	    // temporary
    bool		new_packet;
    int			stack_count;
    float		mute_pos;
    float		taper_length;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;
    void		gettr(SUsegy&) const;
    void		puttr(const SUsegy&);

    static const char*	attrName( int idx )	{ return attrnames[idx]; }
    static int		nrAttrs()		{ return 9; }
    static int		attrNr(const char*);
    static const char*	attrnames[];
    double		getAttr(int) const;

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
    static float	defaultSampleInterval(bool forcetime=false);

    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);

};


#endif
