#ifndef faultstickset_h
#define faultstickset_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        J.C. Glas
Date:          November 2008
RCS:           $Id: faultstickset.h,v 1.3 2008-12-25 11:55:38 cvsranojay Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{

mClass FaultStickSet : public RowColSurface
{
public:
    			FaultStickSet();
    			~FaultStickSet();
    bool		isEmpty() const		{ return !sticks_.size(); }
    Element*		clone() const;

    virtual bool	insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0);
    bool		removeStick(int stick);

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    int			nrSticks() const;
    int			nrKnots(int stick) const;

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int stick) const;

    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    const Coord3&	getEditPlaneNormal(int stick) const;				
    enum ChangeTag	{StickChange=__mUndefIntVal+1,StickInsert,StickRemove};
    
    			// To be used by surface reader only
    void		addUdfRow(int stickidx,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3&);

protected:

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	sticks_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Coord3>		editplanenormals_;
};

};

#endif
