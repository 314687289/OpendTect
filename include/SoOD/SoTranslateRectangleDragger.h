#ifndef SoTranslateRectangleDragger_h
#define SoTranslateRectangleDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTranslateRectangleDragger.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

/*!\brief
Is a dragger that is made up of a plane that can be dragged back and forth
along the rectangle's normal. If certain properties should be set on the plane
(such as texture coordinates (as done in the sliding planes), nodes can be
inserted in the prefixgroup.
*/

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/sensors/SoFieldSensor.h>

class SbLineProjector;

class SoTranslateRectangleDragger : public SoDragger
{
    SO_KIT_HEADER( SoTranslateRectangleDragger );
    
    SO_KIT_CATALOG_ENTRY_HEADER(translator);
    SO_KIT_CATALOG_ENTRY_HEADER(prefixgroup);

public:
    				SoTranslateRectangleDragger();
    static void			initClass();

    SoSFVec3f			translation;
    SoSFFloat			min;
    SoSFFloat			max;

protected:
    SbLineProjector*		lineproj;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );

    void			dragStart();
    void			drag();

    SoFieldSensor*		fieldsensor;
    static void			fieldsensorCB(void*, SoSensor* );
    static void			valueChangedCB( void*, SoDragger* );

    virtual SbBool		setUpConnections( SbBool onoff,
	    					  SbBool doitalways = false );

private:
    static const char		geombuffer[];
    				~SoTranslateRectangleDragger();
};

#endif
