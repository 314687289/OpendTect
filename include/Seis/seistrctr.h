#ifndef seistrctr_h
#define seistrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrctr.h,v 1.35 2004-07-28 16:44:45 bert Exp $
________________________________________________________________________

Translators for seismic traces.

-*/

#include "transl.h"
#include "ctxtioobj.h"
#include "samplingdata.h"
#include "basiccompinfo.h"

class BinID;
class Coord;
class SeisTrc;
class LinScaler;
class SeisTrcBuf;
class SeisSelData;
class SeisTrcInfo;
class CubeSampling;
class SeisPacketInfo;



/*!\brief Seismic Trace translator.

The protocol is as follows:

1) Client opens Connection apropriate for Translator. This connection will
   remain managed by client.

READ:

2) initRead() call initialises SeisPacketInfo, Component info and SeisSelData
   on input (if any)
3) Client subselects in components and space (SeisSelData)
4) commitSelections()
5) By checking readInfo(), client may determine whether space selection
   was satisfied. Space selection is just a hint. This is done to protect
   client against (possible) freeze during (possible) search.
6) readData() reads actual trace components, or skip() skips trace(s).

WRITE:

2) with initWrite() client passes Connection and example trace. Translator
   will fill default writing layout. If Translator is single component,
   only the first component will have a destidx != -1.
3) Client sets trace selection and components as wanted
4) commitSelections() writes 'global' header, if any
5) write() writes selected traces/trace sections


lastly) close() finishes work (does not close connection). If you don't close
yourself, the destructor will do it but make sure it's done because otherwise
you'll likely loose an entire inline when writing.

Note the existence of minimalHdrs(). If this is true, we have only
inline/crossline. If you use setMinimalHdrs(), only inline/crossline and trace
data are read & written. Of course, for rigid formats like SEG-Y, this has no
advantage, so then the flag will be ignored.

Another note: the SeisSelData type 'TrcNrs' is not supported by this class.
That is because of nasty implementation details on this level. The classes
SeisTrcReader and SeisTrcWriter do support it.

*/


class SeisTrcTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisTrc)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisTrc)
};


class SeisTrcTranslator : public Translator
{
public:

    /*!\brief Information for one component

    This is info on data and how it is stored, where the 'store' can be on disk
    (read) or in memory (write).

    */

    class ComponentData : public BasicComponentInfo
    {
	friend class	SeisTrcTranslator;

    protected:
			ComponentData( const char* nm="Seismic Data" )
			: BasicComponentInfo(nm)	{}
			ComponentData( const ComponentData& cd )
			: BasicComponentInfo(cd)	{}
			ComponentData(const SeisTrc&,int icomp=0,
				      const char* nm="Seismic Data");
	void		operator=(const ComponentData&);
			    //!< Protection against assignment.
    };


    /*!\brief ComponentData as it should be when the Translator puts it away.

    The data will be copied from the input ComponentData, but can then be
    changed to desired values. If a component should not be read/written,
    set destidx to -1.

    */

    class TargetComponentData : public ComponentData
    {
	friend class	SeisTrcTranslator;

    public:

	int			destidx;
	const ComponentData&	org;

    protected:

			    TargetComponentData( const ComponentData& c,
						 int idx )
			    : ComponentData(c), org(c), destidx(idx)	{}

	void		operator=(const TargetComponentData&);
			    //!< Protection against assignment.
    };


			SeisTrcTranslator(const char*,const char*);
    virtual		~SeisTrcTranslator();

			/*! Init functions must be called, because
			     Conn object must be always available */
    bool		initRead(Conn*);
			/*!< After call, component and packet info will be
			 available. Note that Conn may need to have an IOObj* */
    bool		initWrite(Conn*,const SeisTrc&);
			/*!< After call, default component and packet info
			   will be generated according to the example trace.
			   Note that Conn may need to have an IOObj* */
    Conn*		curConn()			{ return conn; }

    SeisPacketInfo&		packetInfo();
    const SeisSelData*		selData()		{ return seldata; }
    ObjectSet<TargetComponentData>& componentInfo()	{ return tarcds; }
    const SamplingData<float>&	inpSD() const		{ return insd; }
    int				inpNrSamples() const	{ return innrsamples; }
    const SamplingData<float>&	outSD() const		{ return outsd; }
    int				outNrSamples() const	{ return outnrsamples; }

    void		setSelData( const SeisSelData* t ) { seldata = t; }
			/*!< This SeisSelData is seen as a hint ... */
    bool		commitSelections();
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		{ return false; }
    virtual bool	read(SeisTrc&)			{ return false; }
    virtual bool	skip( int nrtrcs=1 )		{ return false; }
    bool		write(const SeisTrc&);

    void		close();
    const char*		errMsg() const			{ return errmsg; }

    virtual bool	inlCrlSorted() const		{ return true; }
    virtual int		bytesOverheadPerTrace() const	{ return 240; }
    virtual void	toSupported( DataCharacteristics& ) const {}
			//!< change the input to a supported characteristic

    virtual void	usePar( const IOPar& )		{}

    inline int		selComp( int nr=0 ) const	{ return inpfor_[nr]; }
    inline int		nrSelComps() const		{ return nrout_; }
    SeisTrc*		getEmpty();
			/*!< Returns an empty trace with the target data
				characteristics for component 0 */
    SeisTrc*		getFilled(const BinID&);
			/*!< Returns a full sized trace with zeros. */

    virtual bool	supportsGoTo() const		{ return false; }
    virtual bool	goTo(const BinID&)		{ return false; }
    bool		minimalHdrs() const		{ return false; }
    void		setMinimalHdrs()		{}

    virtual void	cleanUp();
    			//!< Prepare for new initialisation.

    static bool		getRanges(const MultiID&,CubeSampling&);
    static bool		getRanges(const IOObj&,CubeSampling&);
    static  bool	is2D(const IOObj&);

protected:

    Conn*		conn;
    const char*		errmsg;
    SeisPacketInfo*	pinfo;

    SamplingData<float>			insd;
    int					innrsamples;
    ObjectSet<ComponentData>		cds;
    ObjectSet<TargetComponentData>	tarcds;
    const SeisSelData*			seldata;
    SamplingData<float>			outsd;
    int					outnrsamples;
    Interval<int>			samps;

    void		addComp(const DataCharacteristics&,
				const char* nm=0,int dtype=0);

    bool		initConn(Conn*,bool forread);
    void		setDataType( int icomp, int d )
			{ cds[icomp]->datatype = tarcds[icomp]->datatype = d; }
    void		fillOffsAzim(SeisTrcInfo&,const Coord&,const Coord&);

			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }
    virtual bool	prepareWriteBlock(StepInterval<int>&,bool&)
    							{ return true; }
    virtual void	blockDumped(int nrtrcs)		{}
    void		prepareComponents(SeisTrc&,int actualsz) const;

			// Quick access to selected, like selComp() etc.
    ComponentData**	inpcds;
    TargetComponentData** outcds;

    			// Buffer written when writeBlock() is called
    SeisTrcBuf&		trcblock_;
    virtual bool	writeTrc_(const SeisTrc&)	{ return false; }

private:

    int*		inpfor_;
    int			nrout_;
    int			prevnr_;
    int			lastinlwritten;
    bool		enforce_regular_write;
    bool		enforce_survinfo_write;

    void		enforceBounds();
    bool		writeBlock();
    bool		dumpBlock();

};


#endif
