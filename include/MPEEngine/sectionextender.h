#ifndef sectionextender_h
#define sectionextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionextender.h,v 1.20 2009-09-01 22:06:02 cvskris Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "emposid.h"
#include "sets.h"
#include "cubesampling.h"
#include "sortedlist.h"

class BinID;
class BinIDValue;

namespace EM
{
    class EMObject;
};

namespace MPE
{

class SectionSourceSelector;

mClass SectionExtender : public SequentialTask
{
public:
    				SectionExtender(const EM::SectionID& si = -1);
    EM::SectionID		sectionID() const;

    virtual void		reset();
    virtual void		setDirection( const BinIDValue& );
    virtual const BinIDValue*	getDirection() const;

    void			setStartPositions(const TypeSet<EM::SubID> ns);
    void			excludePositions(const TypeSet<EM::SubID>*); 
    bool			isExcludedPos(const EM::SubID&) const; 
    int				nextStep();
    
    void			extendInVolume(const BinID& bidstep,
    					       float zstep);

    const TypeSet<EM::SubID>&	getAddedPositions() const;
    const TypeSet<EM::SubID>&	getAddedPositionsSource() const;

    virtual const CubeSampling& getExtBoundary() const;
    void			setExtBoundary(const CubeSampling&);
    void			unsetExtBoundary();

    virtual int			maxNrPosInExtArea() const { return -1; }
    virtual void		preallocExtArea() {}

    const char*			errMsg() const;
    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) 	{ return true; }

    void			setUndo(bool yn)	{ setundo_ = yn; }

protected:
    void			addTarget(const EM::SubID& target,
	    				  const EM::SubID& src );

    SortedList<EM::SubID>	sortedaddedpos_;
    TypeSet<EM::SubID>		addedpos_;
    TypeSet<EM::SubID>		addedpossrc_;
    TypeSet<EM::SubID>		startpos_;
    
    const TypeSet<EM::SubID>*	excludedpos_;

    CubeSampling		extboundary_;

    const EM::SectionID		sid_;
    BufferString		errmsg;
    bool			setundo_;
};

};

#endif
