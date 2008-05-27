#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshape.h,v 1.21 2008-05-27 19:47:55 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoIndexedShape;
class SoMaterialBinding;
class SoNormalBinding;
class SoSeparator;
class SoShape;
class SoShapeHints;
class SoSwitch;
class SoVertexShape;

namespace visBase
{
class VisColorTab;
class Material;
class Texture2;
class Texture3;
class Coordinates;
class Normals;
class TextureCoords;


#undef mDeclSetGetItem
#define mDeclSetGetItem( ownclass, clssname, variable ) \
protected: \
    clssname*		   gt##clssname() const; \
public: \
    inline clssname*	   get##clssname()	 { return gt##clssname(); } \
    inline const clssname* get##clssname() const { return gt##clssname(); } \
    void		   set##clssname(clssname*)


class Shape : public VisualObject
{
public:
    void			turnOn(bool);
    bool			isOn() const;
    void			removeSwitch();
    				/*!<Will turn the object permanently on.
				    \note Must be done before giving away the
				    SoNode with getInventorNode() to take
				    effect. */

    void			setRenderCache(int mode);
				    //!<\param mode=0 off, 1=on, 2=auto (deflt)
    int				getRenderCache() const;

    mDeclSetGetItem( Shape,	Texture2, texture2_ );
    mDeclSetGetItem( Shape,	Texture3, texture3_ );
    mDeclSetGetItem( Shape,	Material, material_ );

    void			setMaterialBinding( int );
    static int			cOverallMaterialBinding()	{ return 0; }
    static int			cPerFaceMaterialBinding()	{ return 1; }
    static int			cPerVertexMaterialBinding()	{ return 2; }
    static int			cPerPartMaterialBinding()	{ return 3; }

    int				getMaterialBinding() const;

    int				usePar(const IOPar&);
    void			fillPar(IOPar&,TypeSet<int>&) const;

    SoNode*			getInventorNode();
    void			insertNode( SoNode* );
				    /*!< Inserts the node _before_ the shape */
    void			removeNode(SoNode*);
    virtual void		replaceShape(SoNode*);
    SoNode*			getShape() { return shape_; }

protected:
				Shape( SoNode* );
    virtual			~Shape();

    SoNode*			shape_;
    SoSwitch*			onoff_;

    Texture2*			texture2_;
    Texture3*			texture3_;
    Material*			material_;

private:
    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();

    SoSeparator*		root_;
    SoMaterialBinding*		materialbinding_;

};


class VertexShape : public Shape
{
public:

    mDeclSetGetItem( VertexShape, Coordinates, coords_ );
    mDeclSetGetItem( VertexShape, Normals, normals_ );
    mDeclSetGetItem( VertexShape, TextureCoords, texturecoords_ );


    void		setDisplayTransformation( Transformation* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.  */
    Transformation*	getDisplayTransformation();
    			/*!<\note Direcly relayed to the coordinates */

    void		setNormalPerFaceBinding( bool yn );
    			/*!< If yn==false, normals are set per vertex */
    bool		getNormalPerFaceBinding() const;
    			/*!< If yn==false, normals are set per vertex */

    void		setVertexOrdering(int vo);
    int			getVertexOrdering() const;
    static int		cClockWiseVertexOrdering()		{ return 0; }
    static int		cCounterClockWiseVertexOrdering()	{ return 1; }
    static int		cUnknownVertexOrdering()		{ return 2; }

    void		setFaceType(int);
    int			getFaceType() const;
    static int		cUnknownFaceType()			{ return 0; }
    static int		cConvexFaceType()			{ return 1; }

    void		setShapeType(int);
    int			getShapeType() const;
    static int		cUnknownShapeType()			{ return 0; }
    static int		cSolidShapeType()			{ return 1; }

protected:
    			VertexShape( SoVertexShape* );
    			~VertexShape();

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

private:
    SoNormalBinding*	normalbinding_;
    SoShapeHints*	shapehints_;
};

#undef mDeclSetGetItem


class IndexedShape : public VertexShape
{
public:
    int		nrCoordIndex() const;
    void	setCoordIndex(int pos,int idx);
    void	setCoordIndices(int* idxs, int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	copyCoordIndicesFrom(const IndexedShape&);
    void	removeCoordIndexAfter(int);
    int		getCoordIndex(int) const;

    int		nrTextureCoordIndex() const;
    void	setTextureCoordIndex(int pos,int idx);
    void	setTextureCoordIndices(int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	removeTextureCoordIndexAfter(int);
    int		getTextureCoordIndex(int) const;

    int		nrNormalIndex() const;
    void	setNormalIndex(int pos,int idx);
    void	setNormalIndices(int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	removeNormalIndexAfter(int);
    int		getNormalIndex(int) const;

    int		nrMaterialIndex() const;
    void	setMaterialIndex(int pos,int idx);
    void	setMaterialIndices(int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	removeMaterialIndexAfter(int);
    int		getMaterialIndex(int) const;

    int		getClosestCoordIndex(const EventInfo&) const;
    void	replaceShape(SoNode*);

protected:
    		IndexedShape( SoIndexedShape* );

private:

    SoIndexedShape*	indexedshape_;
};

};


#endif
