/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoRandomTrackLineDragger.cc,v 1.4 2003-02-12 12:52:26 kristofer Exp $";

#include "SoRandomTrackLineDragger.h"

#include "errh.h"

#include "Inventor/draggers/SoDragPointDragger.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoRotation.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoScale.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoTriangleStripSet.h"

SO_KIT_SOURCE(SoRandomTrackLineDragger);

void SoRandomTrackLineDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoRandomTrackLineDragger, SoBaseKit, "BaseKit" );
}


SoRandomTrackLineDragger::SoRandomTrackLineDragger()
    : motionCBList( *new SoCallbackList )
{
    SO_KIT_CONSTRUCTOR(SoRandomTrackLineDragger);
    SO_KIT_ADD_CATALOG_ENTRY(subDraggerSep,SoSeparator, false,
				this, feedbackSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY(subDraggerScale,SoScale, false,
				subDraggerSep, subDraggerRot, true );
    SO_KIT_ADD_CATALOG_ENTRY(subDraggerRot,SoRotation, false,
				subDraggerSep, subDraggers, false );
    SO_KIT_ADD_CATALOG_LIST_ENTRY(subDraggers,SoGroup, false,
				subDraggerSep, , SoDragPointDragger, false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch,SoSwitch, false,
				this, , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, false,
	    			feedbackSwitch, , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackCoords, SoCoordinate3, false,
	    			feedback, feedbackMaterial, false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackMaterial, SoMaterial, false,
	    			feedback, feedbackShapeHints, false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackShapeHints, SoShapeHints, false,
	    			feedback, feedbackStrip , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackStrip, SoTriangleStripSet, false,
	    			feedback, , false );

    SO_KIT_ADD_FIELD(knots,(-1,0));
    SO_KIT_ADD_FIELD(z0, (-1));
    SO_KIT_ADD_FIELD(z1, (1));

    SO_KIT_ADD_FIELD( xyzStart, (-10,-10,-10) );
    SO_KIT_ADD_FIELD( xyzStop, (10,10,10) );
    SO_KIT_ADD_FIELD( xyzStep, (1,1,1) );
    knots.set1Value(1, SbVec2f(1,0));

    SO_KIT_INIT_INSTANCE();

    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    sw->whichChild = SO_SWITCH_NONE;

    SoRotation* rot = SO_GET_ANY_PART( this, "subDraggerRot", SoRotation );
    rot->rotation.setValue( SbVec3f( 1, 0, 0 ), M_PI/2 );

    SoScale* scale = SO_GET_ANY_PART( this, "subDraggerScale", SoScale );
    scale->scaleFactor.setValue( SbVec3f( 0.5, 0.5, 0.5 ) );

    SoShapeHints* hints =
		SO_GET_ANY_PART( this, "feedbackShapeHints", SoShapeHints );
    hints->vertexOrdering = SoShapeHints::CLOCKWISE;

    knotsfieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    knotsfieldsensor->setPriority(0);
    z0fieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    z0fieldsensor->setPriority(0);
    z1fieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    z1fieldsensor->setPriority(0);

    setUpConnections( true, true );
}


SoRandomTrackLineDragger::~SoRandomTrackLineDragger()
{
    delete knotsfieldsensor;
    delete z0fieldsensor;
    delete z1fieldsensor;
    delete &motionCBList;
}


void SoRandomTrackLineDragger::showFeedback(bool yn)
{
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    sw->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


void SoRandomTrackLineDragger::addMotionCallback(
				SoRandomTrackLineDraggerCB* func, void* data)
{
    motionCBList.addCallback((SoCallbackListCB*) func, data );
}


void SoRandomTrackLineDragger::removeMotionCallback(
			    SoRandomTrackLineDraggerCB* func, void* data)
{
    motionCBList.removeCallback((SoCallbackListCB*) func, data );
}




float SoRandomTrackLineDragger::xyzSnap( int dim, float val ) const
{
    if ( val>xyzStop.getValue()[dim] ) return xyzStop.getValue()[dim];
    val -= xyzStart.getValue()[dim];
    if ( val<0 ) return xyzStart.getValue()[dim];

    val /= xyzStep.getValue()[dim];
    const int idx = (int)(val+0.5);
    return xyzStart.getValue()[dim] + idx*xyzStep.getValue()[dim];
}


void SoRandomTrackLineDragger::startCB( void* parent, SoDragger* )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->dragStart();
}


void SoRandomTrackLineDragger::motionCB( void* parent, SoDragger* dragger )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->drag(dragger);
}


void SoRandomTrackLineDragger::finishCB( void* parent, SoDragger* )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->dragFinish();
}


void SoRandomTrackLineDragger::fieldChangeCB( void* parent, SoSensor* )
{
    SoRandomTrackLineDragger* myself = 
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);

    myself->updateDraggers();
}


void SoRandomTrackLineDragger::dragStart()
{
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    sw->whichChild = 0;
}


void SoRandomTrackLineDragger::drag(SoDragger* dragger_)
{
    SoDragPointDragger* dragger =
			reinterpret_cast<SoDragPointDragger*>( dragger_ );

    SoNodeKitListPart* partlist =
	SO_GET_ANY_PART( this, "subDraggers", SoNodeKitListPart );

    const int nrdraggers = partlist->getNumChildren();
    int draggerid = -1;
    for ( int idx=0; idx<nrdraggers; idx++ )
    {
	if ( dragger==partlist->getChild(idx) )
	{
	    draggerid = idx;
	    break;
	}
    }

    if ( draggerid==-1 )
    {
	pErrMsg("Hue!");
	return;
    }

    const bool istop = !(draggerid%2);
    movingknot = draggerid/2;

    SoScale* scale = SO_GET_ANY_PART( this, "subDraggerScale", SoScale );
    SbVec3f scalefactor = scale->scaleFactor.getValue();

    SbVec3f newpos_ = dragger->translation.getValue();
    SbVec3f newpos( xyzSnap(0,newpos_[0]*scalefactor[0]),
	    		xyzSnap(1,-newpos_[2]*scalefactor[1]),
			xyzSnap(2,newpos_[1]*scalefactor[2]) );
    bool ischanged = false;

    if ( istop )
    {
	const float z0val = z0.getValue();
	const float z1val = z1.getValue();
	if ( !mIS_ZERO(z0val-newpos[2]) && !mIS_ZERO(newpos[2]-z1val) &&
		newpos[2]<z1val)
	{
	    SbBool enabled = z0.enableNotify(false);
	    z0.setValue(newpos[2]);
	    z0.enableNotify(enabled);
	    ischanged = true;
	}
    }
    else
    {
	const float z0val = z0.getValue();
	const float z1val = z1.getValue();
	if ( !mIS_ZERO(z1val-newpos[2]) && !mIS_ZERO(z0val-newpos[2])
		&& newpos[2]>z0val)
	{
	    SbBool enabled = z1.enableNotify(false);
	    z1.setValue(newpos[2]);
	    z1.enableNotify(enabled);
	    ischanged = true;
	}
    }


    bool changexy = true;
    for ( int idx=0; idx<knots.getNum(); idx++ )
    {
	if ( idx==movingknot ) continue;

	const SbVec2f oldpos = knots[idx];
	if ( mIS_ZERO(oldpos[0]-newpos[0]) && mIS_ZERO(oldpos[1]-newpos[1]) )
	{
	    changexy = false;
	    break;
	}
    }

    const SbVec2f oldpos = knots[movingknot];

    if ( changexy &&
	   (!mIS_ZERO( oldpos[0]-newpos[0])||!mIS_ZERO( oldpos[1]-newpos[1])))
    {
	SbBool enabled = knots.enableNotify(false);
	knots.set1Value( movingknot, SbVec2f( newpos[0], newpos[1] ));
	knots.enableNotify( enabled );
	ischanged = true;
    }

    if ( ischanged )
    {
	knots.touch();
	motionCBList.invokeCallbacks( this );
    }
}


void SoRandomTrackLineDragger::dragFinish()
{
    updateDraggers();
}


void SoRandomTrackLineDragger::updateDraggers()
{
    SoNodeKitListPart* partlist = 
	SO_GET_ANY_PART( this, "subDraggers", SoNodeKitListPart );

    SoCoordinate3* coords =
	SO_GET_ANY_PART( this, "feedbackCoords", SoCoordinate3 );

    const int nrknots = knots.getNum();

    while ( partlist->getNumChildren()<nrknots*2 )
    {
	SoDragPointDragger* newdragger = new SoDragPointDragger;
	partlist->addChild( newdragger );
	newdragger->addStartCallback( &startCB, this );
	newdragger->addMotionCallback( &motionCB, this );
	newdragger->addFinishCallback( &finishCB, this );
    }

    while ( partlist->getNumChildren()>nrknots*2 )
	partlist->removeChild( partlist->getNumChildren()-1 );

    SoScale* scale = SO_GET_ANY_PART( this, "subDraggerScale", SoScale );
    SbVec3f scalefactor = scale->scaleFactor.getValue();


    const int nrdraggers = partlist->getNumChildren();
    for ( int idx=0; idx<nrdraggers; idx++ )
    {
	const int knotid = idx/2;
	const bool istop = !(idx%2);

	const SbVec2f knotpos = knots[knotid];

	SbVec3f draggerpos( knotpos[0]/scalefactor[0],
		(istop ? z0.getValue() : z1.getValue())/scalefactor[2],
		-knotpos[1]/scalefactor[1] );

	SoDragPointDragger* curdragger =
	    reinterpret_cast<SoDragPointDragger*>(partlist->getChild(idx));

	curdragger->translation.setValue(draggerpos);
	SbVec3f pos( knotpos[0], knotpos[1],
				istop ? z0.getValue() : z1.getValue());

	coords->point.set1Value(idx, pos );
    }

    SoTriangleStripSet* strips =
	SO_GET_ANY_PART( this, "feedbackStrip", SoTriangleStripSet );

    strips->numVertices.setValue(nrdraggers);
}


SbBool SoRandomTrackLineDragger::setUpConnections( SbBool onOff,
						   SbBool doItAlways )
{
    if ( !doItAlways && connectionsSetUp==onOff )
	return onOff;

    if ( onOff )
    {
	SoBaseKit::setUpConnections( onOff, doItAlways );
	fieldChangeCB( this, 0 );
	if ( knotsfieldsensor->getAttachedField()!= &knots )
	    knotsfieldsensor->attach(&knots);
	if ( z0fieldsensor->getAttachedField()!= &z0 )
	    z0fieldsensor->attach(&z0);
	if ( z1fieldsensor->getAttachedField()!= &z1 )
	    z1fieldsensor->attach(&z1);
    }
    else
    {
	if ( knotsfieldsensor->getAttachedField() )
	    knotsfieldsensor->detach();
	if ( z0fieldsensor->getAttachedField() )
	    z0fieldsensor->detach();
	if ( z1fieldsensor->getAttachedField() )
	    z1fieldsensor->detach();

	SoBaseKit::setUpConnections( onOff, doItAlways );
    }
    
    return !(connectionsSetUp = onOff);
}


