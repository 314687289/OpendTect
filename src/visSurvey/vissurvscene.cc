/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vissurvscene.cc,v 1.83 2006-03-12 13:39:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "vissurvscene.h"

#include "iopar.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vissurvobj.h"
#include "zaxistransform.h"


mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {

const char* Scene::sKeyShowAnnot()			{ return "Show text"; }
const char* Scene::sKeyShowScale()			{ return "Show scale"; }
const char* Scene::sKeyShoCube()			{ return "Show cube"; }

Scene::Scene()
    : inlcrl2disptransform_(0)
    , utm2disptransform_(0)
    , zscaletransform_(0)
    , annot_(0)
    , marker_(0)
    , mouseposchange(this)
    , mouseposval_(0)
    , mouseposstr_("")
    , zscalechange(this)
    , curzscale_(-1)
    , datatransform_( 0 )
{
    events.eventhappened.notify( mCB(this,Scene,mouseMoveCB) );
    setAmbientLight( 1 );
    init();
}


void Scene::init()
{
    annot_ = visBase::Annotation::create();
    annot_->setText( 0, "In-line" );
    annot_->setText( 1, "Cross-line" );
    annot_->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );

    const CubeSampling& cs = SI().sampling(true);
    createTransforms( cs.hrg );
    float zsc = STM().defZScale();
    SI().pars().get( STM().zScaleStr(), zsc );
    setZScale( zsc );

    setAnnotationCube( cs );
    addInlCrlTObject( annot_ );
}


Scene::~Scene()
{
    events.eventhappened.remove( mCB(this,Scene,mouseMoveCB) );

    int objidx = getFirstIdx( inlcrl2disptransform_ );
    if ( objidx >= 0 ) removeObject( objidx );

    objidx = getFirstIdx( zscaletransform_ );
    if ( objidx >= 0 ) removeObject( objidx );

    objidx = getFirstIdx( annot_ );
    if ( objidx >= 0 ) removeObject( objidx );

    if ( utm2disptransform_ ) utm2disptransform_->unRef();
    if ( datatransform_ ) datatransform_->unRef();
}


void Scene::createTransforms( const HorSampling& hs )
{
    if ( !zscaletransform_ )
    {
	zscaletransform_ = STM().createZScaleTransform();
	visBase::DataObjectGroup::addObject(
		const_cast<visBase::Transformation*>(zscaletransform_) );
    }

    if ( !inlcrl2disptransform_ )
    {
	inlcrl2disptransform_ = STM().createIC2DisplayTransform( hs );
	visBase::DataObjectGroup::addObject(
		const_cast<visBase::Transformation*>(inlcrl2disptransform_) );
    }

    if ( !utm2disptransform_ )
    {
	utm2disptransform_ = STM().createUTM2DisplayTransform( hs );
	utm2disptransform_->ref();
    }
}


void Scene::setAnnotationCube( const CubeSampling& cs )
{
    if ( !annot_ ) return;

    BinID c0( cs.hrg.start.inl, cs.hrg.start.crl ); 
    BinID c1( cs.hrg.stop.inl, cs.hrg.start.crl ); 
    BinID c2( cs.hrg.stop.inl, cs.hrg.stop.crl ); 
    BinID c3( cs.hrg.start.inl, cs.hrg.stop.crl );

    annot_->setCorner( 0, c0.inl, c0.crl, cs.zrg.start );
    annot_->setCorner( 1, c1.inl, c1.crl, cs.zrg.start );
    annot_->setCorner( 2, c2.inl, c2.crl, cs.zrg.start );
    annot_->setCorner( 3, c3.inl, c3.crl, cs.zrg.start );
    annot_->setCorner( 4, c0.inl, c0.crl, cs.zrg.stop );
    annot_->setCorner( 5, c1.inl, c1.crl, cs.zrg.stop );
    annot_->setCorner( 6, c2.inl, c2.crl, cs.zrg.stop );
    annot_->setCorner( 7, c3.inl, c3.crl, cs.zrg.stop );
}


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setDisplayTransformation( utm2disptransform_ );
    const int insertpos = getFirstIdx( inlcrl2disptransform_ );
    insertObject( insertpos, obj );
}


void Scene::addInlCrlTObject( visBase::DataObject* obj )
{
    visBase::Scene::addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    mDynamicCastGet(visBase::VisualObject*,vo,obj)

    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->notify( mCB(this,Scene,objectMoved) );

    if ( so )
    {
	if ( so->getMovementNotification() )
	    so->getMovementNotification()->notify( mCB(this,Scene,objectMoved));

	so->setScene( this );
    }

    if ( so && so->isInlCrl() )
	addInlCrlTObject( obj );
    else if ( vo )
	addUTMObject( vo );

    if ( so && datatransform_ )
	so->setDataTransform( datatransform_ );
}


void Scene::removeObject( int idx )
{
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotification() )
	so->getMovementNotification()->remove( mCB(this,Scene,objectMoved) );

    visBase::DataObjectGroup::removeObject( idx );
}


void Scene::setZScale( float zscale )
{
    if ( !zscaletransform_ ) return;

    if ( mIsEqual(zscale,curzscale_,mDefEps) ) return;

    STM().setZScale( zscaletransform_, zscale );
    curzscale_ = zscale;
    zscalechange.trigger();
}


float Scene::getZScale() const
{ return curzscale_; }


mVisTrans* Scene::getZScaleTransform() const
{ return zscaletransform_; }


mVisTrans* Scene::getInlCrl2DisplayTransform() const
{ return inlcrl2disptransform_; }


mVisTrans* Scene::getUTM2DisplayTransform() const
{ return utm2disptransform_; }


void Scene::showAnnotText( bool yn )
{ annot_->showText( yn ); }


bool Scene::isAnnotTextShown() const
{ return annot_->isTextShown(); }


void Scene::showAnnotScale( bool yn )
{ annot_->showScale( yn ); }


bool Scene::isAnnotScaleShown() const
{ return annot_->isScaleShown(); }


void Scene::showAnnot( bool yn )
{ annot_->turnOn( yn ); }


bool Scene::isAnnotShown() const
{ return annot_->isOn(); }


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos_;
   
   Coord3 res = xytmousepos_;
   BinID binid = SI().transform( Coord(res.x,res.y) );
   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


float Scene::getMousePosValue() const
{ return mouseposval_; }


BufferString Scene::getMousePosString() const
{ return mouseposstr_; }


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
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseMovement ) return;

    mouseposval_ = mUdf(float);
    mouseposstr_ = "";
    xytmousepos_ = Coord3::udf();

    const int sz = eventinfo.pickedobjids.size();
    if ( sz )
    {
	const Coord3 displayspacepos = 
		zscaletransform_->transformBack( eventinfo.pickedpos );
	xytmousepos_ = utm2disptransform_->transformBack( displayspacepos );

	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	    mDynamicCastGet(const SurveyObject*,so,pickedobj);
	    if ( so )
	    {
		if ( mIsUdf(mouseposval_) )
		{
		    float newmouseposval;
		    BufferString newstr;
		    so->getMousePosInfo( eventinfo, xytmousepos_,
			    		 newmouseposval, newstr );
		    if ( newstr != "" )
			mouseposstr_ = newstr;
		    if ( !mIsUdf(newmouseposval) )
			mouseposval_ = newmouseposval;
		}

		break;
	    }
	}
    }

    mouseposchange.trigger();
}


void Scene::setDataTransform( ZAxisTransform* zat )
{
    if ( datatransform_ ) datatransform_->unRef();
    datatransform_ = zat;
    if ( datatransform_ ) datatransform_->ref();

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;

	so->setDataTransform( zat );
    }
}


ZAxisTransform* Scene::getDataTransform()
{ return datatransform_; }


void Scene::setMarkerPos( const Coord3& coord )
{
    const Coord3 displaypos( coord,
	    datatransform_ ? datatransform_->transform(coord) : coord.z);

    const bool defined = displaypos.isDefined();
    if ( !defined )
    {
	if ( marker_ )
	    marker_->turnOn( false );

	return;
    }

    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
    }

    marker_->turnOn( true );
    marker_->setCenterPos( displaypos );
}


visBase::Marker* Scene::createMarker() const
{
    visBase::Marker* marker = visBase::Marker::create();
    marker->setType( MarkerStyle3D::Cross );
    marker->getMaterial()->setColor( cDefaultMarkerColor() );
    marker->setScreenSize( 6 );
    return marker;
}


void Scene::setMarkerSize( float nv )
{
    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
	marker_->turnOn( false );
    }

    marker_->setScreenSize( nv );
}


float Scene::getMarkerSize() const
{
    if ( !marker_ )
	return visBase::Marker::cDefaultScreenSize();

    return marker_->getScreenSize();
}


void Scene::setMarkerColor( const Color& nc )
{
    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
	marker_->turnOn( false );
    }

    marker_->getMaterial()->setColor( nc );
}


const Color& Scene::getMarkerColor() const
{
    if ( !marker_ )
	return cDefaultMarkerColor();

    return marker_->getMaterial()->getColor();
}


const Color& Scene::cDefaultMarkerColor()
{
    static Color res( 255,255,255 );
    return res;
}


void Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::DataObject::fillPar( par, saveids );

    int kid = 0;
    while ( getObject(kid++) != zscaletransform_ )
	;

    int nrkids = 0;    
    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot_ || (marker_ && getObject(kid)==marker_) ||
	     getObject(kid)==inlcrl2disptransform_ ) continue;

	const int objid = getObject(kid)->id();
	if ( saveids.indexOf(objid) == -1 )
	    saveids += objid;

	BufferString key = kidprefix; key += nrkids;
	par.set( key, objid );
	nrkids++;
    }

    par.set( nokidsstr, nrkids );
    
    par.setYN( sKeyShowAnnot(), isAnnotTextShown() );
    par.setYN( sKeyShowScale(), isAnnotScaleShown() );
    par.setYN( sKeyShoCube(), isAnnotShown() );
}


void Scene::removeAll()
{
    visBase::DataObjectGroup::removeAll();
    zscaletransform_ = 0; inlcrl2disptransform_ = 0; annot_ = 0;
    curzscale_ = -1;
}


int Scene::usePar( const IOPar& par )
{
    removeAll();
    init();

    int res = visBase::DataObjectGroup::usePar( par );
    if ( res!=1 ) return res;

    bool txtshown = true;
    par.getYN( sKeyShowAnnot(), txtshown );
    showAnnotText( txtshown );

    bool scaleshown = true;
    par.getYN( sKeyShowScale(), scaleshown );
    showAnnotScale( scaleshown );

    bool cubeshown = true;
    par.getYN( sKeyShoCube(), cubeshown );
    showAnnot( cubeshown );

    return 1;
}

} // namespace visSurvey
