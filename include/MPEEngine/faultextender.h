#ifndef faultextender_h
#define faultextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faultextender.h,v 1.4 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"

#include "position.h"

namespace EM { class Fault3D; };

namespace MPE
{

mClass FaultExtender : public SectionExtender
{
public:
			FaultExtender(EM::Fault3D&,const EM::SectionID& =-1);

    const Coord3&	maxDistance() const;
    void		setMaxDistance(const Coord3&);

    virtual void	setDirection(const BinIDValue&);
    const BinIDValue*	getDirection() const { return &direction; }

    int			nextStep();

protected:
    BinIDValue		direction;
    EM::Fault3D&	fault;

    Coord3		maxdistance;
};

}; // namespace MPE

#endif

