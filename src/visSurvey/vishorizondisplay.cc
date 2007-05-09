/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: vishorizondisplay.cc,v 1.30 2007-05-09 18:20:22 cvskris Exp $
________________________________________________________________________

-*/

#include "vishorizondisplay.h"

#include "attribsel.h"
#include "binidvalset.h"
#include "emhorizon.h"
#include "mpeengine.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarker.h"
#include "vismpe.h"
#include "visparametricsurface.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "survinfo.h"
#include "iopar.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "zaxistransform.h"

#include <math.h>



mCreateFactoryEntry( visSurvey::HorizonDisplay );

visBase::FactoryEntry visSurvey::HorizonDisplay::surfdispentry(
	(visBase::FactPtr) visSurvey::HorizonDisplay::create,
	"visSurvey::SurfaceDisplay");

visBase::FactoryEntry visSurvey::HorizonDisplay::emobjentry(
	(visBase::FactPtr) visSurvey::HorizonDisplay::create,
	"visSurvey::EMObjectDisplay");

namespace visSurvey
{

const char* HorizonDisplay::sKeyTexture = "Use texture";
const char* HorizonDisplay::sKeyColorTableID = "ColorTable ID";
const char* HorizonDisplay::sKeyShift = "Shift";
const char* HorizonDisplay::sKeyWireFrame = "WireFrame on";
const char* HorizonDisplay::sKeyResolution = "Resolution";
const char* HorizonDisplay::sKeyEdgeLineRadius = "Edgeline radius";
const char* HorizonDisplay::sKeyRowRange = "Row range";
const char* HorizonDisplay::sKeyColRange = "Col range";


HorizonDisplay::HorizonDisplay()
    : parrowrg_( -1, -1, -1 )
    , parcolrg_( -1, -1, -1 )
    , curtextureidx_( 0 )
    , usestexture_( true )
    , useswireframe_( false )
    , translation_( 0 )
    , edgelineradius_( 3.5 )
    , validtexture_( false )
    , resolution_( 0 )
    , zaxistransform_( 0 )
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    coltabs_[0]->ref();
}


HorizonDisplay::~HorizonDisplay()
{
    deepErase(as_);

    setSceneEventCatcher( 0 );
    deepUnRef( coltabs_ );

    if ( translation_ )
    {
	removeChild( translation_->getInventorNode() );
	translation_->unRef();
	translation_ = 0;
    }

    removeEMStuff();
    if ( zaxistransform_ )
    {
	for ( int idx=0; idx<intersectionlinevoi_.size(); idx++ )
	    zaxistransform_->removeVolumeOfInterest(intersectionlinevoi_[idx]);

	zaxistransform_->unRef();
    }
}


void HorizonDisplay::setDisplayTransformation( mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setDisplayTransformation(transformation_);
}


bool HorizonDisplay::setDataTransform( ZAxisTransform* nz )
{
    if ( zaxistransform_ ) zaxistransform_->unRef();
    zaxistransform_ = nz;
    if ( zaxistransform_ ) zaxistransform_->ref();

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->setZAxisTransform( nz );
    }

    return true;
}


const ZAxisTransform* HorizonDisplay::getDataTransform() const
{ return zaxistransform_; }


void HorizonDisplay::setSceneEventCatcher(visBase::EventCatcher* ec)
{
    EMObjectDisplay::setSceneEventCatcher( ec );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setSceneEventCatcher( ec );

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setSceneEventCatcher( ec );

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setSceneEventCatcher( ec );

}




EM::PosID HorizonDisplay::findClosestNode( const Coord3& pickedpos ) const
{
    const mVisTrans* ztrans = scene_->getZScaleTransform();
    Coord3 newpos = ztrans->transformBack( pickedpos );
    if ( transformation_ )
	newpos = transformation_->transformBack( newpos );

    const BinID pickedbid = SI().transform( newpos );
    const EM::SubID pickedsubid = pickedbid.getSerialized();
    TypeSet<EM::PosID> closestnodes;
    for ( int idx=sids_.size()-1; idx>=0; idx-- )
    {
	if ( !emobject_->isDefined( sids_[idx], pickedsubid ) )
	    continue;
	closestnodes += EM::PosID( emobject_->id(), sids_[idx], pickedsubid );
    }

    if ( closestnodes.isEmpty() ) return EM::PosID( -1, -1, -1 );

    EM::PosID closestnode = closestnodes[0];
    float mindist = mUdf(float);
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emobject_->getPos( closestnodes[idx] );
	const Coord3 displaypos = ztrans->transform(
		transformation_ ? transformation_->transform(coord) : coord );

	const float dist = displaypos.distTo( pickedpos );
	if ( !idx || dist<mindist )
	{
	    closestnode = closestnodes[idx];
	    mindist = dist;
	}
    }

    return closestnode;
}


void HorizonDisplay::edgeLineRightClickCB( CallBacker* )
{}


void HorizonDisplay::removeEMStuff()
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	removeChild( sections_[idx]->getInventorNode() );
	sections_[idx]->unRef();
    }

    sections_.erase();
    sids_.erase();

    while ( intersectionlines_.size() )
    {
	intersectionlines_[0]->unRef();
	intersectionlines_.remove(0);
	intersectionlineids_.remove(0);
	if ( zaxistransform_ )
	    zaxistransform_->removeVolumeOfInterest( intersectionlinevoi_[0] );
	intersectionlinevoi_.remove(0);

    }

    mDynamicCastGet( EM::Horizon*, emhorizon, emobject_ );
    if ( emhorizon )
	emhorizon->edgelinesets.addremovenotify.remove(
			mCB(this,HorizonDisplay,emEdgeLineChangeCB ));

    while ( !edgelinedisplays_.isEmpty() )
    {
	EdgeLineSetDisplay* elsd = edgelinedisplays_[0];
	edgelinedisplays_ -= elsd;
	elsd->rightClicked()->remove(
		 mCB(this,HorizonDisplay,edgeLineRightClickCB) );
	removeChild( elsd->getInventorNode() );
	elsd->unRef();
    }

    EMObjectDisplay::removeEMStuff();
}


bool HorizonDisplay::setEMObject( const EM::ObjectID& newid )
{
    if ( !EMObjectDisplay::setEMObject( newid ) )
	return false;

    mDynamicCastGet( EM::Horizon*, emhorizon, emobject_ );
    if ( emhorizon ) emhorizon->edgelinesets.addremovenotify.notify(
			    mCB(this,HorizonDisplay,emEdgeLineChangeCB ));

    return true;
}


StepInterval<int> HorizonDisplay::displayedRowRange() const
{
    mDynamicCastGet(const EM::Horizon*, surface, emobject_ );
    if ( !surface ) return parrowrg_;
    
    return surface->geometry().rowRange();
}


StepInterval<int> HorizonDisplay::displayedColRange() const
{
    mDynamicCastGet(const EM::Horizon*, surface, emobject_ );
    if ( !surface ) return parcolrg_;
    
    return surface->geometry().colRange();
}


bool HorizonDisplay::updateFromEM()
{ 
    if ( !EMObjectDisplay::updateFromEM() )
	return false;

    useTexture( usestexture_ );
    return true;
}


void HorizonDisplay::updateFromMPE()
{
    const bool hastracker = MPE::engine().getTrackerByObject(getObjectID())>=0;
	
    if ( hastracker && !restoresessupdate_ )
    {
	useWireframe( true );
	useTexture( false );
    }

    if ( displayedRowRange().nrSteps()<=1 || displayedColRange().nrSteps()<=1 )
    {
	useWireframe( true );
	setResolution( nrResolutions()-1 );
    }

    EMObjectDisplay::updateFromMPE();
}


bool HorizonDisplay::addEdgeLineDisplay( const EM::SectionID& sid )
{
    mDynamicCastGet( EM::Horizon*, emhorizon, emobject_ );
    EM::EdgeLineSet* els = emhorizon
	? emhorizon->edgelinesets.getEdgeLineSet(sid,false) : 0;

    if ( els )
    {
	bool found = false;
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if ( edgelinedisplays_[idx]->getEdgeLineSet()==els )
	    {
		found = true;
		break;
	    }
	}
	
	if ( !found )
	{
	    visSurvey::EdgeLineSetDisplay* elsd =
		visSurvey::EdgeLineSetDisplay::create();
	    elsd->ref();
	    elsd->setConnect(true);
	    elsd->setEdgeLineSet(els);
	    elsd->setRadius(edgelineradius_);
	    addChild( elsd->getInventorNode() );
	    elsd->setDisplayTransformation(transformation_);
	    elsd->rightClicked()->notify(
		    mCB(this,HorizonDisplay,edgeLineRightClickCB));
	    edgelinedisplays_ += elsd;
	}
    }

    return true;
}


void HorizonDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( as_[idx]->id()==Attrib::SelSpec::cNoAttrib() )
	    {
		usestexture_ = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture_ = yn;

    getMaterial()->setColor( yn ? Color::White : nontexturecol_ );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->useTexture( yn );
    }
    
    if ( trigger )
	changedisplay.trigger();
}


bool HorizonDisplay::usesTexture() const
{ return usestexture_; }


bool HorizonDisplay::hasColor() const
{
    return !usesTexture();
}


bool HorizonDisplay::getOnlyAtSectionsDisplay() const
{ return displayonlyatsections_; }


bool HorizonDisplay::canHaveMultipleAttribs() const
{ return true; }


int HorizonDisplay::nrTextures( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() || sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->nrVersions( attrib ) : 0;
}


void HorizonDisplay::selectTexture( int attrib, int textureidx )
{
    curtextureidx_ = textureidx;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->selectActiveVersion( attrib, textureidx );
    }

    mDynamicCastGet(EM::Horizon*,emsurf,emobject_)
    if ( !emsurf || emsurf->auxdata.nrAuxData() == 0 ) return;

    if ( textureidx >= emsurf->auxdata.nrAuxData() )
	setSelSpec( 0, Attrib::SelSpec(0,Attrib::SelSpec::cAttribNotSel()) );
    else
    {
	BufferString attrnm = emsurf->auxdata.auxDataName( textureidx );
	setSelSpec( 0, Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()));
    }
}


int HorizonDisplay::selectedTexture( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() || sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->activeVersion( attrib ) : 0;
}


SurveyObject::AttribFormat HorizonDisplay::getAttributeFormat() const
{
    if ( sections_.isEmpty() ) return SurveyObject::None;
    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? SurveyObject::RandomPos : SurveyObject::None;
}


int HorizonDisplay::nrAttribs() const
{ return as_.size(); }


bool HorizonDisplay::canAddAttrib() const
{
    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    const int maxnr = psurf ? psurf->maxNrTextures() : 0;
    if ( !maxnr ) return true;

    return nrAttribs()<maxnr;
}


bool HorizonDisplay::addAttrib()
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    coltabs_[coltabs_.size()-1]->ref();

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	{
	    psurf->addTexture();
	    psurf->setColorTab( coltabs_.size()-1,
		    		*coltabs_[coltabs_.size()-1] );
	}
    }

    return true;
}


bool HorizonDisplay::removeAttrib( int attrib )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->removeTexture( attrib );
    }

    coltabs_[attrib]->unRef();
    coltabs_.remove( attrib );

    delete as_[attrib];
    as_.remove( attrib );
    return true;
}


bool HorizonDisplay::swapAttribs( int a0, int a1 )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->swapTextures( a0, a1 );
    }

    as_.swap( a0, a1 );
    coltabs_.swap( a0, a1 );
    return true;
}


void HorizonDisplay::setAttribTransparency( int attrib, unsigned char nt )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->setTextureTransparency( attrib, nt );
    }
}


unsigned char HorizonDisplay::getAttribTransparency( int attrib ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    return psurf->getTextureTransparency( attrib );
    }

    return 0;
}


void HorizonDisplay::enableAttrib( int attribnr, bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->enableTexture( attribnr, yn );
    }
}


bool HorizonDisplay::isAttribEnabled( int attribnr ) const
{
    if ( sections_.isEmpty() )
	return true;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->isTextureEnabled(attribnr) : true;
}


bool HorizonDisplay::isAngle( int attribnr ) const
{
    if ( sections_.isEmpty() )
	return false;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->isTextureAngle(attribnr) : false;
}
    

void HorizonDisplay::setAngleFlag( int attribnr, bool yn )
{
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setTextureAngleFlag( attribnr, yn );
    }
}

const Attrib::SelSpec* HorizonDisplay::getSelSpec( int attrib ) const
{ return as_[attrib]; }


void HorizonDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    (*as_[attrib]) = as;
}


void HorizonDisplay::setDepthAsAttrib( int attrib )
{
    as_[attrib]->set( "", Attrib::SelSpec::cNoAttrib(), false, "" );

    ObjectSet<BinIDValueSet> positions;
    getRandomPos( positions );

    if ( positions.isEmpty() )
    {
	useTexture( false );
	return;
    }

    for ( int idx=0; idx<positions.size(); idx++ )
    {
	if ( positions[idx]->nrVals()!=2 )
	    positions[idx]->setNrVals(2);

	BinIDValueSet::Pos pos;
	while ( positions[idx]->next(pos,true) )
	{
	    float* vals = positions[idx]->getVals(pos);
	    vals[1] = vals[0];
	}
    }

    setRandomPosData( attrib, &positions );
    useTexture( usestexture_ );
    deepErase( positions );
}


void HorizonDisplay::getRandomPos( ObjectSet<BinIDValueSet>& data ) const
{
    deepErase( data );
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( !psurf ) return;

	data += new BinIDValueSet( 1, false );
	BinIDValueSet& res = *data[idx];
	psurf->getDataPositions( res, getTranslation().z/SI().zFactor() );
    }
}


void HorizonDisplay::getRandomPosCache( int attrib,
				 ObjectSet<const BinIDValueSet>& data ) const
{
    data.erase();
    data.allowNull( true );
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( !psurf )
	{
	    data += 0;
	    continue;
	}

	data += psurf->getCache( attrib );
    }
}


void HorizonDisplay::setRandomPosData( int attrib,
				 const ObjectSet<BinIDValueSet>* data )
{
    validtexture_ = true;

    if ( !data || !data->size() )
    {
	for ( int idx=0; idx<sections_.size(); idx++ )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	    if ( psurf ) psurf->setTextureData( 0, attrib );
	    else useTexture(false);
	}
	return;
    }

    int idx = 0;
    for ( ; idx<data->size(); idx++ )
    {
	if ( idx>=sections_.size() ) break;

	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->setTextureData( (*data)[idx], attrib );
	else
	    useTexture(false);
    }

    for ( ; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setTextureData( 0, attrib );
    }
}


bool HorizonDisplay::hasStoredAttrib( int attrib ) const
{
    const char* userref = as_[attrib]->userRef();
    return as_[attrib]->id()==Attrib::SelSpec::cOtherAttrib() &&
	   userref && *userref;
}


Coord3 HorizonDisplay::getTranslation() const
{
    if ( !translation_ ) return Coord3(0,0,0);

    Coord3 shift = translation_->getTranslation();
    shift.z *= -1; 
    return shift;
}


void HorizonDisplay::setTranslation( const Coord3& nt )
{
    if ( !translation_ )
    {
	translation_ = visBase::Transformation::create();
	translation_->ref();
	insertChild( 0, translation_->getInventorNode() );
    }

    Coord3 shift( nt ); shift.z *= -1;
    translation_->setTranslation( shift );

    mDynamicCastGet(EM::Horizon*,horizon,emobject_);
    if ( horizon ) horizon->geometry().setShift( -shift.z );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	if ( ps ) ps->inValidateCache(-1);
    }
}


bool HorizonDisplay::usesWireframe() const { return useswireframe_; }


void HorizonDisplay::useWireframe( bool yn )
{
    useswireframe_ = yn;

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	if ( ps ) ps->useWireframe( yn );
    }
}


void HorizonDisplay::setEdgeLineRadius(float nr)
{
    edgelineradius_ = nr;
    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setRadius(nr);
}


float HorizonDisplay::getEdgeLineRadius() const
{ return edgelineradius_; }


void HorizonDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( sections_[idx]->getInventorNode() );
    sections_[idx]->unRef();
    sections_.remove( idx );
    sids_.remove( idx );
};


bool HorizonDisplay::addSection( const EM::SectionID& sid )
{
    visBase::ParametricSurface* surf = visBase::ParametricSurface::create();

    mDynamicCastGet( EM::Horizon*, horizon, emobject_ );
    surf->setSurface(horizon->geometry().sectionGeometry(sid), true );

    while ( surf->nrTextures()<nrAttribs() ) surf->addTexture();

    for ( int idx=0; idx<nrAttribs(); idx++ )
	surf->setColorTab( idx, *coltabs_[idx] );

    surf->useWireframe( useswireframe_ );
    surf->setResolution( resolution_-1 );

    surf->ref();
    surf->setMaterial( 0 );
    surf->setDisplayTransformation( transformation_ );
    surf->setZAxisTransform( zaxistransform_ );
    const int index = childIndex(drawstyle_->getInventorNode());
    insertChild( index, surf->getInventorNode() );
    surf->turnOn( !displayonlyatsections_ );

    sections_ += surf;
    sids_ += sid;

    hasmoved.trigger();

    return addEdgeLineDisplay( sid );
}


void HorizonDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->turnOn(!yn);

    EMObjectDisplay::setOnlyAtSectionsDisplay( yn );
}



void HorizonDisplay::emChangeCB( CallBacker* cb )
{
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	validtexture_ = false;
	if ( usesTexture() ) useTexture(false);

	const EM::SectionID sid = cbdata.pid0.sectionID();
	const int idx = sids_.indexOf( sid );
	if ( idx>=0 )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	    if ( ps ) ps->inValidateCache(-1);
	}
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = emobject_->preferredColor();
	if ( !usestexture_ )
	    getMaterial()->setColor( nontexturecol_ );
    }
}


void HorizonDisplay::emEdgeLineChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack( EM::SectionID, section, cb );

    mDynamicCastGet(const EM::Horizon*,emhorizon,emobject_);
    if ( !emhorizon ) return;

    if ( emhorizon->edgelinesets.getEdgeLineSet( section, false ) )
	 addEdgeLineDisplay(section);
    else
    {
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if (edgelinedisplays_[idx]->getEdgeLineSet()->getSection()==section)
  	    {
		EdgeLineSetDisplay* elsd = edgelinedisplays_[idx--];
		edgelinedisplays_ -= elsd;
		elsd->rightClicked()->remove(
			 mCB( this, HorizonDisplay, edgeLineRightClickCB ));
		removeChild( elsd->getInventorNode() );
		elsd->unRef();
		break;
	    }
	}
    }
}


int HorizonDisplay::nrResolutions() const
{
    if ( sections_.isEmpty() ) return 1;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? ps->nrResolutions()+1 : 1;
}


BufferString HorizonDisplay::getResolutionName( int res ) const
{
    BufferString str;
    if ( !res ) str = "Automatic";
    else
    {
	res = nrResolutions() - res;
	res--;
	int val = 1;
	for ( int idx=0; idx<res; idx++ )
	    val *= 2;

	if ( val==2 ) 		str = "Half";
	else if ( val==1 ) 	str = "Full";
	else 			{ str = "1 / "; str += val; }
    }

    return str;
}


int HorizonDisplay::getResolution() const
{
    if ( sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? ps->currentResolution()+1 : 0;
}


void HorizonDisplay::setResolution( int res )
{
    resolution_ = res;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	if ( ps ) ps->setResolution( res-1 );
    }
}


int HorizonDisplay::getColTabID(int attrib) const
{
    return usesTexture() ? coltabs_[attrib]->id() : -1;
}


float HorizonDisplay::calcDist( const Coord3& pickpos ) const
{
    if ( !emobject_ ) return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pickpos );
    mDynamicCastGet(const EM::Horizon*,hor,emobject_)
    if ( hor )
    {
	const BinID bid = SI().transform( xytpos );
	const EM::SubID bidid = bid.getSerialized();
	TypeSet<Coord3> positions;
	for ( int idx=sids_.size()-1; idx>=0; idx-- )
	{
	    if ( !emobject_->isDefined( sids_[idx], bidid ) )
		continue;

	    positions += emobject_->getPos( sids_[idx], bidid );
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<positions.size(); idx++ )
	{
	    const Coord3& pos = positions[idx] + 
				getTranslation()/SI().zFactor();
	    const float dist = fabs(xytpos.z-pos.z);
	    if ( dist < mindist ) mindist = dist;
	}

	return mindist;
    }

    return mUdf(float);
}


float HorizonDisplay::maxDist() const
{
    return SI().zRange(true).step;
}


EM::SectionID HorizonDisplay::getSectionID( int visid ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( sections_[idx]->id()==visid )
		return sids_[idx];
    }

    return -1;
}


void HorizonDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       const Coord3& pos, BufferString& val,
				       BufferString& info ) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, pos, val, info );
    if ( !emobject_ || !usesTexture() ) return;

    const EM::SectionID sid =
	EMObjectDisplay::getSectionID(&eventinfo.pickedobjids);

    const int sectionidx = sids_.indexOf( sid );
    if ( sectionidx<0 ) return;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,
	    	    sections_[sectionidx]);
    if ( !psurf ) return;

    const BinID bid( SI().transform(pos) );

    for ( int idx=as_.size()-1; idx>=0; idx-- )
    {
	if ( as_[idx]->id()<-1 )
	    return;

	if ( !psurf->isTextureEnabled(idx) ||
	       psurf->getTextureTransparency(idx)==255 )
	    continue;

	const BinIDValueSet* bidvalset = psurf->getCache( idx );
	if ( !bidvalset || bidvalset->nrVals()<2 ) continue;

	const BinIDValueSet::Pos setpos = bidvalset->findFirst( bid );
	if ( setpos.i<0 || setpos.j<0 ) continue;

	const float* vals = bidvalset->getVals( setpos );
	int curtexture = selectedTexture(idx);
	if ( curtexture>bidvalset->nrVals()-1 ) curtexture = 0;

	const float fval = vals[curtexture+1];
	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !psurf->getCache(idy) || !isAttribEnabled(idy) ||
		  psurf->getTextureTransparency(idy)==255 )
		continue;
							                 
		islowest = false;
		break;
	}    

	if ( !islowest )
	{
	    const Color col = coltabs_[idx]->color(fval);
	    if ( col.t()==255 )
		continue;
	}
	    
	if ( !mIsUdf(fval) )
	{
	    val = fval;
	    if ( as_.size()>1 )
	    {
		BufferString attribstr = "(";
		attribstr += as_[idx]->userRef();
		attribstr += ")";
		val.insertAt( cValNameOffset(), (const char*) attribstr );
	    }
	}

	return;
    }
}


#define mEndLine \
{ \
    if ( cii<2 || ( cii>2 && line->getCoordIndex(cii-2)==-1 ) ) \
    { \
	while ( cii && line->getCoordIndex(cii-1)!=-1 ) \
	    line->getCoordinates()->removePos(line->getCoordIndex(--cii)); \
    } \
    else if ( cii && line->getCoordIndex(cii-1)!=-1 )\
    { \
	line->setCoordIndex(cii++,-1); \
    } \
}

#define mTraverseLine( linetype, startbid, faststop, faststep, slowdim, fastdim ) \
    const int target##linetype = cs.hrg.start.linetype; \
    if ( !linetype##rg.includes(target##linetype) ) \
    { \
	mEndLine; \
	continue; \
    } \
 \
    const int rgindex = linetype##rg.getIndex(target##linetype); \
    const int prev##linetype = linetype##rg.atIndex(rgindex); \
    const int next##linetype = prev##linetype<target##linetype \
	? linetype##rg.atIndex(rgindex+1) \
	: prev##linetype; \
 \
    for ( BinID bid=startbid; bid[fastdim]<=faststop; bid[fastdim]+=faststep ) \
    { \
	if ( !cs.hrg.includes(bid) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	BinID prevbid( bid ); prevbid[slowdim] = prev##linetype; \
	BinID nextbid( bid ); nextbid[slowdim] = next##linetype; \
	Coord3 prevpos(horizon->getPos(sid,prevbid.getSerialized())); \
	if ( zaxistransform_ ) prevpos.z =zaxistransform_->transform(prevpos); \
	Coord3 pos = prevpos; \
	if ( nextbid!=prevbid && prevpos.isDefined() ) \
	{ \
	    Coord3 nextpos = \
		horizon->getPos(sid,nextbid.getSerialized()); \
	    if ( zaxistransform_ ) nextpos.z = \
	    	zaxistransform_->transform(nextpos); \
	    if ( nextpos.isDefined() ) \
	    { \
		pos += nextpos; \
		pos /= 2; \
	    } \
	} \
 \
	if ( !pos.isDefined() || !cs.zrg.includes(pos.z) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	line->setCoordIndex(cii++, line->getCoordinates()->addPos(pos)); \
    }

void HorizonDisplay::updateIntersectionLines(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    mDynamicCastGet(const EM::Horizon*,horizon,emobject_);
    if ( !horizon ) return;

    TypeSet<int> linestoupdate;
    BoolTypeSet lineshouldexist( intersectionlineids_.size(), false );

    if ( displayonlyatsections_ )
    {
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    int objectid = -1;
	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
		objectid = plane->id();
	    else
	    {
		mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
		if ( mped && mped->isDraggerShown() )
		{
		    CubeSampling cs;
		    if ( mped->getPlanePosition(cs) &&
			  ( cs.hrg.start.inl==cs.hrg.stop.inl ||
			    cs.hrg.start.crl==cs.hrg.stop.crl ))
			objectid = mped->id();
		}
	    }

	    if ( objectid==-1 )
		continue;

	    const int idy = intersectionlineids_.indexOf(objectid);
	    if ( idy==-1 )
	    {
		linestoupdate += objectid;
		lineshouldexist += true;
	    }
	    else
	    {
		if ( ( whichobj==objectid || whichobj==id() ) && 
		     linestoupdate.indexOf(whichobj)==-1 )
		{
		    linestoupdate += objectid;
		}

		lineshouldexist[idy] = true;
	    }
	}
    }

    for ( int idx=0; idx<intersectionlineids_.size(); idx++ )
    {
	if ( !lineshouldexist[idx] )
	{
	    removeChild( intersectionlines_[idx]->getInventorNode() );
	    intersectionlines_[idx]->unRef();

	    lineshouldexist.remove(idx);
	    intersectionlines_.remove(idx);
	    intersectionlineids_.remove(idx);
	    if ( zaxistransform_ )
	    {
		zaxistransform_->removeVolumeOfInterest(
			intersectionlinevoi_[idx] );
	    }

	    intersectionlinevoi_.remove(idx);
	    idx--;
	}
    }

    for ( int idx=0; idx<linestoupdate.size(); idx++ )
    {
	CubeSampling cs(false);
	mDynamicCastGet( PlaneDataDisplay*, plane,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( plane )
	    cs = plane->getCubeSampling(true,true,-1);
	else
	{
	    mDynamicCastGet( const MPEDisplay*, mped,
		    	     visBase::DM().getObject(linestoupdate[idx]));
	    mped->getPlanePosition(cs);
	}

	int lineidx = intersectionlineids_.indexOf(linestoupdate[idx]);
	if ( lineidx==-1 )
	{
	    lineidx = intersectionlineids_.size();
	    intersectionlineids_ += linestoupdate[idx];
	    intersectionlinevoi_ += -2;
	    visBase::IndexedPolyLine* newline =
		visBase::IndexedPolyLine::create();
	    newline->ref();
	    newline->setDisplayTransformation(transformation_);
	    intersectionlines_ += newline;
	    addChild( newline->getInventorNode() );
	}

	if ( zaxistransform_ )
	{
	    if ( intersectionlinevoi_[lineidx]==-2 )
	    {
		intersectionlinevoi_[lineidx] =
		    zaxistransform_->addVolumeOfInterest(cs,true);
	    }
	    else
	    {
		zaxistransform_->setVolumeOfInterest(
			intersectionlinevoi_[lineidx],cs,true);
	    }

	    if ( intersectionlinevoi_[lineidx]>=0 )
	    {
		zaxistransform_->loadDataIfMissing(
			intersectionlinevoi_[lineidx] );
	    }
	}
	    

	


	visBase::IndexedPolyLine* line = intersectionlines_[lineidx];
	line->getCoordinates()->removeAfter(-1);
	line->removeCoordIndexAfter(-1);
	int cii = 0;

	for ( int sectionidx=0; sectionidx<horizon->nrSections(); sectionidx++ )
	{
	    const EM::SectionID sid = horizon->sectionID(sectionidx);
	    const StepInterval<int> inlrg = horizon->geometry().rowRange(sid);
	    const StepInterval<int> crlrg = horizon->geometry().colRange(sid);

	    if ( cs.hrg.start.inl==cs.hrg.stop.inl )
	    {
		mTraverseLine( inl, BinID(targetinl,crlrg.start),
			       crlrg.stop, crlrg.step, 0, 1 );
	    }
	    else
	    {
		mTraverseLine( crl, BinID(inlrg.start,targetcrl),
			       inlrg.stop, inlrg.step, 1, 0 );
	    }

	    mEndLine;
	}
    }
}


void HorizonDisplay::updateSectionSeeds( 
	    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    bool refresh = movedobj==-1 || movedobj==id();
    TypeSet<int> planelist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const PlaneDataDisplay*,plane,objs[idx]);
	if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
	{
	    planelist += idx; 
	    if ( movedobj==plane->id() ) 
		refresh = true;
	}

	mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	if ( mped && mped->isDraggerShown() )
	{
	    CubeSampling cs;
	    if ( mped->getPlanePosition(cs) && cs.nrZ()!=1 )
	    {
		planelist += idx; 
		if ( movedobj==mped->id() ) 
		    refresh = true;
	    }
	}
    }

    if ( !refresh ) return;

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::DataObjectGroup* group = posattribmarkers_[idx];
	for ( int idy=0; idy<group->size(); idy++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idy))
	    if ( !marker ) continue;
	
	    marker->turnOn( !displayonlyatsections_ );
	    Coord3 pos = marker->centerPos(true);
	    for ( int idz=0; idz<planelist.size(); idz++ )
		{
		const float dist = objs[planelist[idz]]->calcDist(pos);
		if ( dist < objs[planelist[idz]]->maxDist() )
		{
		    marker->turnOn(true);
		    break;
		}
	    }
	}
    }
}


void HorizonDisplay::otherObjectsMoved(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{ 
    updateIntersectionLines( objs, whichobj ); 
    updateSectionSeeds( objs, whichobj );
}


void HorizonDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    EMObjectDisplay::fillPar( par, saveids );

    if ( emobject_ && !emobject_->isFullyLoaded() )
    {
	par.set( sKeyRowRange, displayedRowRange().start,
		 displayedRowRange().stop,
		 displayedRowRange().step );
	par.set( sKeyColRange, displayedColRange().start,
		  displayedColRange().stop,
		  displayedColRange().step );
    }

    par.setYN( sKeyTexture, usesTexture() );
    par.setYN( sKeyWireFrame, usesWireframe() );
    par.set( sKeyShift, getTranslation().z );
    par.set( sKeyResolution, getResolution() );

    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );
	const int coltabid = coltabs_[attrib]->id();
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	attribpar.setYN( sKeyIsOn(), isAttribEnabled(attrib) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
}


int HorizonDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    res = EMObjectDisplay::usePar( par );
    if ( res!=1 ) return res;

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    if ( !par.get(sKeyEarthModelID,parmid_) )
	return -1;

    par.get( sKeyRowRange, parrowrg_.start, parrowrg_.stop, parrowrg_.step );
    par.get( sKeyColRange, parcolrg_.start, parcolrg_.stop, parcolrg_.step );

    if ( !par.getYN(sKeyTexture,usestexture_) )
	usestexture_ = true;

    bool usewireframe = false;
    par.getYN( sKeyWireFrame, usewireframe );
    useWireframe( usewireframe );

    int resolution = 0;
    par.get( sKeyResolution, resolution );
    setResolution( resolution );

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift, shift.z );
    setTranslation( shift );

    int nrattribs;
    if ( par.get(sKeyNrAttribs(),nrattribs) ) //Current format
    {
	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    bool ison = true;
	    attribpar->getYN( sKeyIsOn(), ison );

	    int coltabid = -1;
	    if ( attribpar->get(sKeyColTabID(),coltabid) )
	    {
		visBase::DataObject* dataobj = 
		    			visBase::DM().getObject( coltabid );
		if ( !dataobj ) return 0;

		mDynamicCastGet(visBase::VisColorTab*,coltab,dataobj);
		if ( !coltab ) coltabid=-1;
		coltabs_[attribnr]->unRef();
		coltabs_.replace( attribnr, coltab );
		coltabs_[attribnr]->ref();

		for ( int idx=0; idx<sections_.size(); idx++ )
		{
		    mDynamicCastGet(visBase::ParametricSurface*,psurf,
			    	    sections_[idx]);
		    if ( psurf )
		    {
			psurf->setColorTab( attribnr, *coltab );
			psurf->enableTexture( attribnr, ison );
		    }
		}
	    }

	    as_[attribnr]->usePar( *attribpar );
	}
    }
    else //old format
    {
	as_[0]->usePar( par );
	int coltabid = -1;
	par.get( sKeyColorTableID, coltabid );
	if ( coltabid>-1 )
	{
	    DataObject* dataobj = visBase::DM().getObject( coltabid );
	    if ( !dataobj ) return 0;

	    mDynamicCastGet( visBase::VisColorTab*, coltab, dataobj );
	    if ( !coltab ) return -1;
	    if ( coltabs_[0] ) coltabs_[0]->unRef();
	    coltabs_.replace( 0, coltab );
	    coltab->ref();
	    for ( int idx=0; idx<sections_.size(); idx++ )
	    {
		mDynamicCastGet( visBase::ParametricSurface*,psurf,
				 sections_[idx]);
		if ( psurf ) psurf->setColorTab( 0, *coltab);
	    }
	}
    }

    return 1;
}


}; // namespace visSurvey
