#ifndef visrandomtracksection_h
#define visrandomtracksection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visrandomtrack.h,v 1.26 2005-02-04 14:31:34 kristofer Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "ranges.h"
#include "visobject.h"

class SoRandomTrackLineDragger;
template <class T> class Array2D;
class IOPar;


namespace visBase
{
class EventCatcher;
class Material;
class TriangleStripSet;
class VisColorTab;

/*!\brief

*/

class RandomTrack : public VisualObjectImpl
{
public:
    static RandomTrack*		create()
				mCreateDataObj(RandomTrack);

    void			showDragger( bool yn );
    bool			isDraggerShown() const;
    void			moveObjectToDraggerPos();
    void			moveDraggerToObjectPos();

    int				nrKnots() const;
    void			setTrack(const TypeSet<Coord>&);
    void			addKnot(const Coord&);
    void			insertKnot(int idx,const Coord&);
    Coord			getKnotPos(int) const;
    Coord			getDraggerKnotPos(int) const;
    void			setKnotPos(int,const Coord&);
    void			setDraggerKnotPos(int,const Coord&);
    void			removeKnot(int);

    void			setDepthInterval(const Interval<float>&);
    const Interval<float>	getDepthInterval() const;
    const Interval<float>	getDraggerDepthInterval() const;

    void			setXrange( const StepInterval<float>& );
    void			setYrange( const StepInterval<float>& );
    void			setZrange( const StepInterval<float>& );
    				/*!< sets limits for dragging */

    void			setDraggerSize( const Coord3& );
    Coord3			getDraggerSize() const;

    void			setClipRate( float );
    float			clipRate() const;

    void			setAutoScale( bool yn );
    bool			autoScale() const;

    void			setColorTab( VisColorTab& );
    VisColorTab&		getColorTab();
    const TypeSet<float>&	getHistogram() const;

    void			setResolution(int);
    int				getResolution() const;

    void			setMaterial( Material* );
    Material*			getMaterial();

    void			useTexture(bool);
    bool			usesTexture() const;

    int				getSectionIdx() const { return sectionidx; }

    void			setData(int sectn,const Array2D<float>*,int tp);
    				/*!< section ranges from 0 to nrKnots-2 */

    void			setColorPars(bool,bool,const Interval<float>&);
    const Interval<float>&	getColorDataRange() const;

    Notifier<RandomTrack>	knotnrchange;
    CNotifier<RandomTrack,int>	knotmovement;
    				/*!< Sends the index of the knot moving */

    static const char*		textureidstr;
    static const char*		draggersizestr;

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    
    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

protected:
    				~RandomTrack();
    void			rebuild();
    void			createDragger();

    static void			motionCB(void*,SoRandomTrackLineDragger*);
    static void			startCB(void*,SoRandomTrackLineDragger*);
    void			triggerRightClick(const EventInfo*);

    Interval<float>		depthrg;
    TypeSet<Coord>		knots;
    int				sectionidx;

    ObjectSet<TriangleStripSet>	sections;
    Transformation*		transformation;
    SoRandomTrackLineDragger*	dragger;
    SoSwitch*			draggerswitch;
};

};

#endif

