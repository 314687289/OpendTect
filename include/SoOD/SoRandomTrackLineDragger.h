#ifndef SoRandomTrackLineDragger_h
#define SoRandomTrackLineDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoRandomTrackLineDragger.h,v 1.4 2003-02-12 12:52:24 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodekits/SoBaseKit.h"
#include "Inventor/fields/SoMFVec2f.h"
#include "Inventor/fields/SoSFFloat.h"
#include "Inventor/fields/SoSFVec3f.h"

class SoDragger;
class SoFieldSensor;
class SoSensor;
class SoCallbackList;
class SoRandomTrackLineDragger;

typedef void SoRandomTrackLineDraggerCB(void * data,
					SoRandomTrackLineDragger* dragger);

/*!\brief

*/

class SoRandomTrackLineDragger : public SoBaseKit
{
    SO_KIT_HEADER(SoRandomTrackLineDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerSep);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerRot);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerScale);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggers);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(feedback);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackShapeHints);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackStrip);

public:
    		SoRandomTrackLineDragger();
    static void	initClass();

    SoMFVec2f	knots;
    SoSFFloat	z0;
    SoSFFloat	z1;

    SoSFVec3f	xyzStart;
    SoSFVec3f	xyzStop;
    SoSFVec3f	xyzStep;

    void	showFeedback(bool yn);
    		/*!< Feedback is turned on when
		     dragging starts. Use this function
		     to turn it off.
		 */


    void	addMotionCallback(SoRandomTrackLineDraggerCB*, void* = 0 );
    void	removeMotionCallback(SoRandomTrackLineDraggerCB*, void* = 0 );
    int		getMovingKnot() const { return movingknot; }
    		/*!< Only valid after cb has been issued */

protected:
    float			xyzSnap( int dim, float ) const;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );
    static void			fieldChangeCB( void*, SoSensor* );

    void			dragStart();
    void			drag(SoDragger*);
    void			dragFinish();

    void			updateDraggers();
    SbBool			setUpConnections(SbBool, SbBool);

    SoFieldSensor*		knotsfieldsensor;
    SoFieldSensor*		z0fieldsensor;
    SoFieldSensor*		z1fieldsensor;

    SoCallbackList&		motionCBList;
    int				movingknot;

private:
    				~SoRandomTrackLineDragger();
};

#endif

