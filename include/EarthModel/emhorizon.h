#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: emhorizon.h,v 1.5 2008-12-31 09:08:40 cvsranojay Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "keystrs.h"
#include "iopar.h"


namespace EM
{
class EMManager;

mClass HorizonGeometry : public RowColSurfaceGeometry
{
protected:
    				HorizonGeometry( Surface& surf )
				    : RowColSurfaceGeometry(surf)	{}
};


mClass Horizon : public Surface
{
public:
    virtual HorizonGeometry&		geometry()			= 0;
    virtual const HorizonGeometry&	geometry() const
					{ return const_cast<Horizon*>(this)
					    			->geometry(); }

    void			setStratLevelID( int lvlid )
				{ stratlevelid_ = lvlid; }
    int				stratLevelID() const
				{ return stratlevelid_; }

    virtual void		fillPar( IOPar& par ) const
				{
				    Surface::fillPar( par );
				    par.set( sKey::StratRef, stratlevelid_ );
				}

    virtual bool		usePar( const IOPar& par )
				{
				    par.get( sKey::StratRef, stratlevelid_ );
				    return Surface::usePar( par );
				}

protected:
    				Horizon( EMManager& emm )
				    : Surface(emm), stratlevelid_(-1)	{}

    virtual const IOObjContext&	getIOObjContext() const		= 0;

    int				stratlevelid_;
};

} // namespace EM

#endif
