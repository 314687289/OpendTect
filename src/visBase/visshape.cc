/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visshape.h"

#include "errh.h"
#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdetail.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexturecoords.h"

#include <osg/PrimitiveSet>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>


mCreateFactoryEntry( visBase::VertexShape );

namespace visBase
{

const char* Shape::sKeyOnOff() 			{ return  "Is on";	}
const char* Shape::sKeyTexture() 		{ return  "Texture";	}
const char* Shape::sKeyMaterial() 		{ return  "Material";	}


    
Shape::Shape( SoNode* shape )
    : texture2_( 0 )
    , texture3_( 0 )
    , material_( 0 )
    , osgswitch_( new osg::Switch )
{
    setOsgNode( osgswitch_ );
}


Shape::~Shape()
{
    if ( texture2_ ) texture2_->unRef();
    if ( texture3_ ) texture3_->unRef();
    if ( material_ ) material_->unRef();
}


bool Shape::turnOn(bool n)
{
    const bool res = isOn();
    if ( osgswitch_ )
    {
	if ( n ) osgswitch_->setAllChildrenOn();
	else osgswitch_->setAllChildrenOff();
    }
    else
    {
	pErrMsg( "Turning off object without switch");
    }
    
    return res;
}


bool Shape::isOn() const
{
    return !osgswitch_ ||
	   (osgswitch_->getNumChildren() && osgswitch_->getValue(0) );
}
    
    
void Shape::removeSwitch()
{
    pErrMsg("Don't call." );
}

#define mDefSetGetItem(ownclass, clssname, variable, osgremove, osgset ) \
void ownclass::set##clssname( clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	osgremove; \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	osgset; \
    } \
} \
 \
 \
clssname* ownclass::gt##clssname() const \
{ \
    return const_cast<clssname*>( variable ); \
}


mDefSetGetItem( Shape, Texture2, texture2_, , );
mDefSetGetItem( Shape, Texture3, texture3_, , );
mDefSetGetItem( Shape, Material, material_,
    removeNodeState( material_ ),
    addNodeState( material_ ) )


void Shape::setMaterialBinding( int nv )
{
    pErrMsg("Not Implemented" );
}


int Shape::getMaterialBinding() const
{
    pErrMsg("Not implemented");

    return cOverallMaterialBinding();
}


void Shape::fillPar( IOPar& iopar ) const
{
    if ( material_ )
    {
	IOPar matpar;
	material_->fillPar( matpar );
	iopar.mergeComp( matpar, sKeyMaterial() );
    }
	
    iopar.setYN( sKeyOnOff(), isOn() );
}


int Shape::usePar( const IOPar& par )
{
    bool ison;
    if ( par.getYN( sKeyOnOff(), ison) )
	turnOn( ison );

    if ( material_ )
    {
	PtrMan<IOPar> matpar = par.subselect( sKeyMaterial() );
	material_->usePar( *matpar );
    }
    
    return 1;
}

    
#define mVertexShapeConstructor( shp, geode ) \
    Shape( shp ) \
    , normals_( 0 ) \
    , coords_( 0 ) \
    , texturecoords_( 0 ) \
    , geode_( geode ) \
    , node_( 0 ) \
    , osggeom_( 0 ) \
    , primitivetype_( Geometry::PrimitiveSet::Other )
    
    
VertexShape::VertexShape()
    : mVertexShapeConstructor( 0, new osg::Geode )
{
    setupGeode();
}
    
    
    
VertexShape::VertexShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp,
			  bool creategeode )
    : mVertexShapeConstructor( 0, creategeode ? new osg::Geode : 0 )
{
    setupGeode();
    setPrimitiveType( tp );
}
    
void VertexShape::setupGeode()
{
    if ( geode_ )
    {
	geode_->ref();
	osggeom_ = new osg::Geometry;
	geode_->addDrawable( osggeom_ );
	osgswitch_->addChild( geode_ );
	node_ = geode_;
    }
    
    setCoordinates( Coordinates::create() );
}
    
    
void VertexShape::setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp )
{
    primitivetype_ = tp;
    
    if ( osggeom_ )
    {
	if ( primitivetype_==Geometry::PrimitiveSet::Lines ||
	    primitivetype_==Geometry::PrimitiveSet::LineStrips )
	{
	    osggeom_->getOrCreateStateSet()->setMode( GL_LIGHTING,
						     osg::StateAttribute::OFF );
	}
    }
}


VertexShape::~VertexShape()
{
    if ( node_ ) node_->unref();
    if ( normals_ ) normals_->unRef();
    if ( coords_ ) coords_->unRef();
    if ( texturecoords_ ) texturecoords_->unRef();
    
    deepUnRef( primitivesets_ );
}
    
    
void VertexShape::removeSwitch()
{
    if ( osgswitch_ )
    {
	setOsgNode( geode_ );
	osgswitch_ = 0;
    }
    else
    {
	Shape::removeSwitch();
    }
}
    
    
void VertexShape::dirtyCoordinates()
{
    if ( osggeom_ )
    {
	osggeom_->dirtyDisplayList();
	osggeom_->dirtyBound();
    }
}


void VertexShape::setDisplayTransformation( const mVisTrans* tr )
{ coords_->setDisplayTransformation( tr ); }


const mVisTrans* VertexShape::getDisplayTransformation() const
{ return  coords_->getDisplayTransformation(); }


mDefSetGetItem( VertexShape, Coordinates, coords_,
if ( osggeom_ ) osggeom_->setVertexArray(0),
if ( osggeom_ ) osggeom_->setVertexArray(mGetOsgVec3Arr( coords_->osgArray())));
    
mDefSetGetItem( VertexShape, Normals, normals_,
if ( osggeom_ ) osggeom_->setNormalArray( 0 ),
if ( osggeom_ ) osggeom_->setNormalArray(mGetOsgVec3Arr(normals_->osgArray())));
    
mDefSetGetItem( VertexShape, TextureCoords, texturecoords_,
if ( osggeom_ ) osggeom_->setTexCoordArray( 0, 0 ),
if ( osggeom_ ) osggeom_->setTexCoordArray( 0,
mGetOsgVec2Arr(texturecoords_->osgArray())));



void VertexShape::setNormalPerFaceBinding( bool nv )
{
        pErrMsg("Not implemented");
}
    
    
bool VertexShape::getNormalPerFaceBinding() const
{
    pErrMsg("Not implemented");
    return false;
}


#define mCheckCreateShapeHints() \
    return;

void VertexShape::setVertexOrdering( int nv )
{
}


int VertexShape::getVertexOrdering() const
{
    return cUnknownVertexOrdering();
}


void VertexShape::setFaceType( int ft )
{
}


int VertexShape::getFaceType() const
{
    return cUnknownFaceType();
}


void VertexShape::setShapeType( int st )
{
}


int VertexShape::getShapeType() const
{
    return cUnknownShapeType();
}


IndexedShape::IndexedShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp )
    : VertexShape( tp, true )
{}


#define setGetIndex( resourcename, fieldname )  \
int IndexedShape::nr##resourcename##Index() const \
{ return 0; } \
 \
 \
void IndexedShape::set##resourcename##Index( int pos, int idx ) \
{ } \
 \
 \
void IndexedShape::remove##resourcename##IndexAfter(int pos) \
{ } \
 \
 \
int IndexedShape::get##resourcename##Index( int pos ) const \
{ return -1; } \
 \
 \
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz ) \
{ } \
\
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz, \
					       int start ) \
{ } \


setGetIndex( Coord, coordIndex );
setGetIndex( TextureCoord, textureCoordIndex );
setGetIndex( Normal, normalIndex );
setGetIndex( Material, materialIndex );


int IndexedShape::getClosestCoordIndex( const EventInfo& ei ) const
{
    pErrMsg( "Not implemented in osg. Needed?");
    return -1;
}
    

class OSGPrimitiveSet
{
public:
    virtual osg::PrimitiveSet*	getPrimitiveSet()	= 0;
    
    static GLenum	getGLEnum(Geometry::PrimitiveSet::PrimitiveType tp)
    {
	switch ( tp )
	{
	    case Geometry::PrimitiveSet::Triangles:
		return GL_TRIANGLES;
	    case Geometry::PrimitiveSet::TriangleFan:
		return GL_TRIANGLE_FAN;
	    case Geometry::PrimitiveSet::TriangleStrip:
		return GL_TRIANGLE_STRIP;
	    case Geometry::PrimitiveSet::Lines:
		return GL_LINES;
	    case Geometry::PrimitiveSet::Points:
		return GL_POINTS;
	    case Geometry::PrimitiveSet::LineStrips:
		return GL_LINE_STRIP;
	    default:
		break;
	}
	
	return GL_POINTS;
    }
};
    
    
void VertexShape::addPrimitiveSet( Geometry::PrimitiveSet* p )
{
    p->ref();
    p->setPrimitiveType( primitivetype_ );
    
    mDynamicCastGet(OSGPrimitiveSet*, osgps, p );
    addPrimitiveSetToScene( osgps->getPrimitiveSet() );
    
    primitivesets_ += p;
}
    
    
void VertexShape::removePrimitiveSet( const Geometry::PrimitiveSet* p )
{
    const int pidx = primitivesets_.indexOf( p );
    mDynamicCastGet( OSGPrimitiveSet*, osgps,primitivesets_[pidx]  );
    removePrimitiveSetFromScene( osgps->getPrimitiveSet() );
    
    primitivesets_.removeSingle( pidx )->unRef();
}
    
    
void VertexShape::addPrimitiveSetToScene( osg::PrimitiveSet* ps )
{
    osggeom_->addPrimitiveSet( ps );
}
    
    
int VertexShape::nrPrimitiveSets() const
{ return primitivesets_.size(); }


Geometry::PrimitiveSet* VertexShape::getPrimitiveSet( int idx )
{
    return primitivesets_[idx];
}
    
    
void VertexShape::removePrimitiveSetFromScene( const osg::PrimitiveSet* ps )
{
    const int idx = osggeom_->getPrimitiveSetIndex( ps );
    osggeom_->removePrimitiveSet( idx );
}

#define mImplOsgFuncs \
osg::PrimitiveSet* getPrimitiveSet() { return element_.get(); } \
void setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp ) \
{ \
    Geometry::PrimitiveSet::setPrimitiveType( tp ); \
    element_->setMode( getGLEnum( getPrimitiveType() )); \
}

    
template <class T>
class OSGIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet,
			       public OSGPrimitiveSet
{
public:
			OSGIndexedPrimitiveSet()
    			    : element_( new T ) {}
    
			mImplOsgFuncs
    virtual void	setEmpty()
    			{ element_->erase(element_->begin(), element_->end() ); }
    virtual void	append( int ) {}
    virtual int		pop() { return 0; }
    virtual int		size() const { return 0; }
    virtual int		get(int) const { return 0; }
    virtual int		set(int,int) { return 0; }
    void		set(const int* ptr, int num)
    {
	element_->clear();
	element_->reserve( num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }
    void		append(const int* ptr, int num)
    {
	element_->reserve( size()+num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }

    
    osg::ref_ptr<T>	element_;
};

    
class OSGRangePrimitiveSet : public Geometry::RangePrimitiveSet,
			     public OSGPrimitiveSet
{
public:
			OSGRangePrimitiveSet()
			    : element_( new osg::DrawArrays )
			{}
    
			mImplOsgFuncs
    
    int			size() const 	   { return element_->getCount();}
    int			get(int idx) const { return element_->getFirst()+idx;}

    void		setRange( const Interval<int>& rg )
    {
	element_->setFirst( rg.start );
	element_->setCount( rg.width()+1 );
    }
    
    Interval<int>	getRange() const
    {
	const int first = element_->getFirst();
	return Interval<int>( first, first+element_->getCount()-1 );
    }
    
    osg::ref_ptr<osg::DrawArrays>	element_;
};
    
    
    

class CoinIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet
{
public:
    virtual void	setEmpty() { indices_.erase(); }
    virtual void	append( int index ) { indices_ += index; }
    virtual int		pop()
    			{
			    const int idx = size()-1;
			    if ( idx<0 ) return mUdf(int);
			    const int res = indices_[idx];
			    indices_.removeSingle(idx);
			    return res;
			}
    
    virtual int		size() const { return indices_.size(); }
    virtual int		get(int pos) const { return indices_[pos]; }
    virtual int		set(int pos,int index)
			{ return indices_[pos] = index; }
    virtual void	set( const int* ptr,int num)
    {
	indices_ = TypeSet<int>( ptr, num );
    }
    
    virtual void	append( const int* ptr, int num )
    { indices_.append( ptr, num ); }
    
    TypeSet<int>	indices_;
};
    
    
class CoinRangePrimitiveSet : public Geometry::RangePrimitiveSet
{
public:
		   	CoinRangePrimitiveSet() : rg_(mUdf(int), mUdf(int) ) {}
    
    int			size() const 			{ return rg_.width()+1;}
    int			get(int idx) const 		{ return rg_.start+idx;}
    void		setRange(const Interval<int>& rg) { rg_ = rg; }
    Interval<int>	getRange() const 		{ return rg_; }
    
    Interval<int>	rg_;
};
    

Geometry::PrimitiveSet*
    PrimitiveSetCreator::doCreate( bool indexed, bool large )
{
    if ( indexed )
	return large
	       ? (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUInt>
	       : (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUShort>;
    
    return new OSGRangePrimitiveSet;
}
    


    
} // namespace visBase
