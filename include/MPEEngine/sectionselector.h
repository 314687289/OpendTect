#ifndef sectionselector_h
#define sectionselector_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselector.h,v 1.10 2012-08-03 13:00:31 cvskris Exp $
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "task.h"
#include "bufstring.h"
#include "emposid.h"
#include "sets.h"

namespace EM
{
    class EMObject;
};


namespace MPE
{

class TrackPlane;

mClass(MPEEngine) SectionSourceSelector : public SequentialTask
{
public:
    				SectionSourceSelector(
					const EM::SectionID& sid = -1);

    EM::SectionID		sectionID() const;
    virtual void		reset();

    virtual void		setTrackPlane(const MPE::TrackPlane&);

    int				nextStep();
    const char*			errMsg() const;

    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) { return true; }

    const TypeSet<EM::SubID>&	selectedPositions() const;

protected:
    EM::SectionID		sectionid_;
    TypeSet<EM::SubID>		selpos_;
    BufferString		errmsg_;
};

};

#endif


