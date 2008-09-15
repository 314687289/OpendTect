#ifndef segyhdr_h
#define segyhdr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: segyhdr.h,v 1.18 2008-09-15 10:10:35 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "seisinfo.h"
#include "segythdef.h"

namespace SEGY
{

#define SegyTxtHeaderLength		3200
#define SegyBinHeaderLength		400
#define SegyBinHeaderUnassUShorts	170
#define SegyTracHeaderLength		240

/*!\brief 3200 byte SEG-Y text header.

  On construction, the 'txt' buffer is filled with data for writing the header.
  If used for reading, fill the buffer yourself and use getFrom.
 
 */


class TxtHeader
{
public:
    		TxtHeader(bool rev1=true); //!< rev1 relevant when writing
 
    void	setUserInfo(const char*);
    void	setPosInfo(const TrcHeaderDef&);
    void	setStartPos(float);
    void	getText(BufferString&);

    void	putAt(int,int,int,const char*);
    void	getFrom(int,int,int,char*) const;
 
    void        setAscii();
    void        setEbcdic();

    unsigned char txt[SegyTxtHeaderLength];

    static bool	info2d;

    void	dump(std::ostream&) const;

};


/*!\brief 400 byte SEG-Y binary header.

  On construction, the 'txt' buffer is filled with data for writing the header.
  If used for reading, fill the buffer yourself and use getFrom.
 
 */

class BinHeader
{
public:

		BinHeader(bool rev1=true); //!< rev1 relevant when writing
    void	getFrom(const void*);
    void	putTo(void*) const;
    int		bytesPerSample() const
		{ return formatBytes( format ); }
    static int	formatBytes( int frmt )
		{ return frmt == 3 ? 2 : (frmt == 8 ? 1 : 4); }

    bool	needswap;

    int		jobid;	/* job identification number */
    int		lino;	/* line number (only one line per reel) */
    int		reno;	/* reel number */
    short	ntrpr;	/* number of data traces per record */
    short	nart;	/* number of auxiliary traces per record */
    short	hdt;	/* sample interval in micro secs for this reel */
    short	dto;	/* same for original field recording */
    short	hns;	/* number of samples per trace for this reel */
    short	nso;	/* same for original field recording */
    short	format;	/* data sample format code:
                            1 = floating point (4 bytes)
                            2 = fixed point (4 bytes)
                            3 = fixed point (2 bytes)
                            4 = fixed point w/gain code (4 bytes)
			       + SEG-Y rev 1:
                            5 = IEEE float (4 bytes)
                            8 = signed char (1 byte)
		    */
    short	fold, tsort, vscode, hsfs, hsfe, hslen, hstyp, schn, hstas,
		hstae, htatyp, hcorr, bgrcv, rcvm;

    short		mfeet;
    			/* measurement system code: 1 = meters 2 = feet */

    short		polyt, vpol;
    unsigned short	hunass[SegyBinHeaderUnassUShorts];

    unsigned short	isrev1; //!< This must be considered final
    unsigned short	fixdsz;	//!< Rev 1 only
    unsigned short	nrstzs; //!< Rev 1 only

    void	dump(std::ostream&) const;
};


class TrcHeader
{
public:

			TrcHeader(unsigned char*,bool rev1,const TrcHeaderDef&);

    unsigned short	nrSamples() const;
    void		putSampling(SamplingData<float>,unsigned short);

    void		use(const SeisTrcInfo&);
    void		fill(SeisTrcInfo&,float) const;

    float		postScale(int numbfmt) const;
    Coord		getCoord(bool rcv,float extcoordsc);

    unsigned char*	buf;
    bool		needswap;
    const TrcHeaderDef&	hdef;
    bool		isrev1;
    int			seqnr;
    int			lineseqnr;
    int			previnl;

    void		dump(std::ostream&) const;

    struct Val
    {
	Val( int byt, const char* desc, int val )
	    : byte_(byt), desc_(desc), val_(val)		{}
	int byte_;
	const char* desc_;
	int val_;
    };
    static int		nrVals()		{ return 86; }
    static int		nrStdVals()		{ return 41; }
    Val			getVal(int) const;

protected:

    double		getCoordScale(float extcoordsc) const;

};

} // namespace


#endif
