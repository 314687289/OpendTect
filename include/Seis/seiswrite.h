#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.16 2005-03-31 15:25:53 cvsarend Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the prepareWork() will be called if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator (3D only), you must first do prepareWork.

*/

#include "seisstor.h"
#include "linekey.h"
class BinIDRange;
class Seis2DLinePutter;


class SeisTrcWriter : public SeisStoreAccess
{
public:

			SeisTrcWriter(const IOObj*,
				      const LineKeyProvider* r=0);
			~SeisTrcWriter();
    virtual bool	close();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);

    void		fillAuxPar(IOPar&) const;
    bool		isMultiComp() const;
    bool		isMultiConn() const;

    			// 2D
    const LineKeyProvider* lineKeyProvider() const { return lkp; }
    void		setLineKeyProvider( const LineKeyProvider* l )
							 { lkp = l; }
    			//!< If no lineKeyProvider set,
    			//!< seldata's linekey will be used
    void		setAttrib( const char* a )	 { attrib = a; }
    			//!< if set, overrules attrib in linekey
    IOPar&		lineAuxPars()			{ return lineauxiopar; }

protected:

    bool		prepared;
    BinIDRange&		binids;
    int			nrtrcs;
    int			nrwritten;

    void		startWork();

    // 3D only
    Conn*		crConn(int,bool);
    bool		ensureRightConn(const SeisTrc&,bool);
    bool		start3DWrite(Conn*,const SeisTrc&);

    // 2D only
    BufferString	attrib;
    Seis2DLinePutter*	putter;
    IOPar&		lineauxiopar;
    LineKey		prevlk;
    const LineKeyProvider* lkp;
    bool		next2DLine();
    bool		put2D(const SeisTrc&);

};


#endif
