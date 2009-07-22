#ifndef horizon3dextender_h
#define horizon3dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.8 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon3D; };

namespace MPE
{

mClass Horizon3DExtender : public SectionExtender
{
public:
    				Horizon3DExtender(EM::Horizon3D& surface_,
						const EM::SectionID& sid);

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const CubeSampling&		getExtBoundary() const;

protected:
    BinIDValue			direction;
    EM::Horizon3D&		surface;
};


}; // namespace MPE

#endif

