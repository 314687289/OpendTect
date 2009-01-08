#ifndef sogridsurfacedragger_h
#define sogridsurfacedragger_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Marc Gerritsen, Jeroen Post
 Date:          23-04-2003
 RCS:           $Id: SoGridSurfaceDragger.h,v 1.4 2009-01-08 09:48:12 cvsnanne Exp $
________________________________________________________________________

-*/


#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFVec4f.h>

class SbLineProjector;


class COIN_DLL_API SoGridSurfaceDragger : public SoDragger 
{
    SO_KIT_HEADER( SoGridSurfaceDragger );
    SO_KIT_CATALOG_ENTRY_HEADER( rotator );
    SO_KIT_CATALOG_ENTRY_HEADER( translator );
    SO_KIT_CATALOG_ENTRY_HEADER( material );
    SO_KIT_CATALOG_ENTRY_HEADER( mainseparator);
    SO_KIT_CATALOG_ENTRY_HEADER( cylinder );
    SO_KIT_CATALOG_ENTRY_HEADER( cube );
    SO_KIT_CATALOG_ENTRY_HEADER( topcylinderseparator );
    SO_KIT_CATALOG_ENTRY_HEADER( topcylindertranslator );
    SO_KIT_CATALOG_ENTRY_HEADER( topcylinder );
    SO_KIT_CATALOG_ENTRY_HEADER( bottomcylinderseparator );
    SO_KIT_CATALOG_ENTRY_HEADER( bottomcylindertranslator );
    SO_KIT_CATALOG_ENTRY_HEADER( bottomcylinder );

public:
    				SoGridSurfaceDragger();
    SoSFVec3f 			translation;
    SoSFVec4f 			rotation;
    static void 		initClass();

protected:
    static void 		startCB(void*,SoDragger*);
    static void 		motionCB(void*,SoDragger*);
    static void 		finishCB(void*,SoDragger*);

    void 			dragStart();
    void 			drag();
    void 			dragFinish();
    SoFieldSensor*		translationFieldSensor;
    SoFieldSensor*		rotationFieldSensor;
    SbLineProjector*		lineProj;
    static void 		translationFieldSensorCB(void*,SoSensor*);
    static void 		rotationFieldSensorCB(void*,SoSensor*);
    static void 		valueChangedCB(void*,SoDragger*);
    virtual SbBool 		setUpConnections(SbBool onOff, 
	    					 SbBool doItAlways=FALSE);

private:
    				~SoGridSurfaceDragger();
};


#endif
