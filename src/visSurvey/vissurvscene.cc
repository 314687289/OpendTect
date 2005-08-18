/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vissurvscene.cc,v 1.70 2005-08-18 19:37:53 cvskris Exp $
________________________________________________________________________

-*/

#include "vissurvscene.h"

#include "errh.h"
#include "iopar.h"
#include "linsolv.h"
#include "survinfo.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vislight.h"
#include "vispicksetdisplay.h"
#include "vistransform.h"

#include <limits.h>

mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {

const char* Scene::annottxtstr = "Show text";
const char* Scene::annotscalestr = "Show scale";
const char* Scene::annotcubestr = "Show cube";

Scene::Scene()
    : inlcrl2displtransform( SPM().getInlCrl2DisplayTransform() )
    , zscaletransform( SPM().getZScaleTransform() )
    , annot(0)
    , mouseposchange(this)
    , mouseposval(0)
    , mouseposstr("")
{
    setAmbientLight( 1 );
    setup();
}


Scene::~Scene()
{
    events.eventhappened.remove( mCB(this,Scene,mouseMoveCB) );
    removeObject( getFirstIdx(inlcrl2displtransform) );
    removeObject( getFirstIdx(zscaletransform) );
    removeObject( getFirstIdx(annot) );
}


void Scene::updateRange()
{ setCube(); }


void Scene::setCube()
{
    if ( !annot ) return;
    const HorSampling& hs = SI().sampling(true).hrg;
    const StepInterval<float>& vrg = SI().zRange(true);

    BinID c0( hs.start.inl, hs.start.crl ); 
    BinID c1( hs.stop.inl, hs.start.crl ); 
    BinID c2( hs.stop.inl, hs.stop.crl ); 
    BinID c3( hs.start.inl, hs.stop.crl );

    annot->setCorner( 0, c0.inl, c0.crl, vrg.start );
    annot->setCorner( 1, c1.inl, c1.crl, vrg.start );
    annot->setCorner( 2, c2.inl, c2.crl, vrg.start );
    annot->setCorner( 3, c3.inl, c3.crl, vrg.start );
    annot->setCorner( 4, c0.inl, c0.crl, vrg.stop );
    annot->setCorner( 5, c1.inl, c1.crl, vrg.stop );
    annot->setCorner( 6, c2.inl, c2.crl, vrg.stop );
    annot->setCorner( 7, c3.inl, c3.crl, vrg.stop );
}


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setDisplayTransformation( SPM().getUTM2DisplayTransform() );
    int insertpos = getFirstIdx( inlcrl2displtransform );
    insertObject( insertpos, obj );
}


void Scene::addInlCrlTObject( visBase::DataObject* obj )
{
    visBase::Scene::addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->notify( mCB(this,Scene,objectMoved) );

    if ( so && so->isInlCrl() )
    {
	addInlCrlTObject( obj );
	return;
    }

    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    if ( vo )
    {
	addUTMObject( vo );
	return;
    }
}


void Scene::removeObject( int idx )
{
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->remove( mCB(this,Scene,objectMoved) );

    DataObjectGroup::removeObject( idx );
}


void Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


void Scene::showAnnotScale( bool yn )
{
    annot->showScale( yn );
}


bool Scene::isAnnotScaleShown() const
{
    return annot->isScaleShown();
}


void Scene::showAnnot( bool yn )
{
    annot->turnOn( yn );
}


bool Scene::isAnnotShown() const
{
    return annot->isOn();
}


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Coord3 res = xytmousepos;
   BinID binid = SI().transform( Coord(res.x,res.y) );
   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void Scene::setup()
{
    setAmbientLight( 1 );

    events.eventhappened.notify( mCB(this,Scene,mouseMoveCB) );

    visBase::DataObjectGroup::addObject(
	    	const_cast<visBase::Transformation*>(zscaletransform));
    visBase::DataObjectGroup::addObject(
	    const_cast<visBase::Transformation*>(inlcrl2displtransform) );

    annot = visBase::Annotation::create();
    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );
    setCube();
}


void Scene::objectMoved( CallBacker* cb )
{
    ObjectSet<const SurveyObject> activeobjects;
    int movedid = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;
	if ( !so->getMovementNotification() ) continue;

	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx))
	if ( !vo ) continue;
	if ( !vo->isOn() ) continue;

	if ( cb==vo ) movedid = vo->id();

	activeobjects += so;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	
	if ( so ) so->otherObjectsMoved( activeobjects, movedid );
    }
}


void Scene::mouseMoveCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    const Coord3 displayspacepos =
           SPM().getZScaleTransform()->transformBack(eventinfo.pickedpos);
    xytmousepos =
           SPM().getUTM2DisplayTransform()->transformBack(displayspacepos);

    mouseposval = mUndefValue;
    mouseposstr = "";
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	mDynamicCastGet(const SurveyObject*,so,pickedobj)
	if ( so )
	{
	    if ( !validpicksurface )
		validpicksurface = true;

	    if ( mIsUndefined(mouseposval) )
	    {
		float newmouseposval;
		BufferString newstr;
		so->getMousePosInfo( eventinfo, xytmousepos, newmouseposval,
				     newstr );
		if ( newstr != "" )
		    mouseposstr = newstr;
		if ( !mIsUndefined(newmouseposval) )
		    mouseposval = newmouseposval;
	    }
	    else if ( validpicksurface )
		break;
	}
    }

    if ( validpicksurface )
	mouseposchange.trigger();
}


void Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::DataObject::fillPar( par, saveids );

    int kid = 0;
    while ( getObject(kid++) != zscaletransform )
	;

    int nrkids = 0;    
    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot || 
	     getObject(kid)==inlcrl2displtransform ) continue;

	const int objid = getObject(kid)->id();
	if ( saveids.indexOf(objid) == -1 )
	    saveids += objid;

	BufferString key = kidprefix; key += nrkids;
	par.set( key, objid );
	nrkids++;
    }

    par.set( nokidsstr, nrkids );
    
    bool txtshown = isAnnotTextShown();
    par.setYN( annottxtstr, txtshown );
    bool scaleshown = isAnnotScaleShown();
    par.setYN( annotscalestr, scaleshown );
    bool cubeshown = isAnnotShown();
    par.setYN( annotcubestr, cubeshown );
}


int Scene::usePar( const IOPar& par )
{
    removeAll();
    setup();

    int res = visBase::DataObjectGroup::usePar( par );
    if ( res!=1 ) return res;

    bool txtshown = true;
    par.getYN( annottxtstr, txtshown );
    showAnnotText( txtshown );

    bool scaleshown = true;
    par.getYN( annotscalestr, scaleshown );
    showAnnotScale( scaleshown );

    bool cubeshown = true;
    par.getYN( annotcubestr, cubeshown );
    showAnnot( cubeshown );

    return 1;
}

}; // namespace visSurvey
