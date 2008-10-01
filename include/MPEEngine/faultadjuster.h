#ifndef faultadjuster_h
#define faultadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faultadjuster.h,v 1.2 2008-10-01 03:44:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sectionadjuster.h"

namespace EM { class Fault3D; };

namespace MPE
{

class FaultAdjuster : public SectionAdjuster
{
public:
    				FaultAdjuster(EM::Fault3D&,
					      const EM::SectionID&);
    				~FaultAdjuster();

    void			reset();
    int				nextStep();

    void			getNeededAttribs(
				    ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling		getAttribCube(const Attrib::SelSpec&) const;

    int				getNrAttributes() const;
    const Attrib::SelSpec*	getAttributeSel( int idx ) const;
    void			setAttributeSel( int, const Attrib::SelSpec& );

    bool			getTrackMax() const { return trackmaximum; }
    void			setTrackMax( bool yn ) { trackmaximum=yn; }

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:
    void			prepareCalc(EM::SubID);
    void			getTargetPositions(EM::SubID, const EM::SubID*,
	    					   TypeSet<BinID>&) const;
    float			computeScore(const Coord3&);

    EM::Fault3D&		fault_;
    bool			trackmaximum;
    Attrib::SelSpec&		attribsel;

    static const char*		sKeyTrackMax();

};


}; // namespace MPE

#endif

