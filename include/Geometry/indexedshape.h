#ifndef indexedshape_h
#define indexedshape_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          September 2007
RCS:           $Id: indexedshape.h,v 1.9 2008-05-29 13:54:09 cvskris Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "thread.h"

class Coord3List;
class TaskRunner;

namespace Geometry
{

/*!A geomtetry that is defined by a number of coordinates (defined outside
   the class), by specifying connections between the coordiates. */

class IndexedGeometry
{
public:
    enum	Type { Lines, Triangles, TriangleStrip, TriangleFan };
    enum	NormalBinding { PerVertex, PerFace };

    		IndexedGeometry(Type,NormalBinding=PerFace,
				Coord3List* coords=0, Coord3List* normals=0,
				Coord3List* texturecoords=0);
		/*!<If coords or normals are given, used indices will be
		    removed when object deleted or removeAll is called. If
		    multiple geometries are sharing the coords/normals, 
		    this is probably not what you want. */
    virtual 	~IndexedGeometry();

    void	removeAll();
    bool	isEmpty() const;


    mutable Threads::Mutex	lock_;

    Type			type_;
    NormalBinding		normalbinding_;

    TypeSet<int>		coordindices_;
    TypeSet<int>		texturecoordindices_;
    TypeSet<int>		normalindices_;

    mutable bool		ischanged_;

protected:
    Coord3List*			coordlist_;
    Coord3List*			texturecoordlist_;
    Coord3List*			normallist_;
};


/*!Defines a shape with coodinates and connections between them. The shape
   is defined in an ObjectSet of IndexedGeometry. All IndexedGeometry share
   one common coordinate and normal list. */

class IndexedShape
{
public:
    virtual 		~IndexedShape();

    virtual void	setCoordList(Coord3List* cl,Coord3List* nl=0,
	    			     Coord3List* texturecoords=0);
    virtual bool	needsUpdate() const			{ return true; }
    virtual bool	update(bool forceall,TaskRunner* =0)	{ return true; }

    virtual void	setRightHandedNormals(bool);
    virtual void	removeAll();

    const ObjectSet<IndexedGeometry>&	getGeometry() const;

    const Coord3List*		coordList() const 	{ return coordlist_; }
    Coord3List*			coordList()	 	{ return coordlist_; }

    int				getVersion() const	{ return version_; }

protected:
    				IndexedShape();

    Threads::ReadWriteLock	geometrieslock_;
    ObjectSet<IndexedGeometry>	geometries_;

    Coord3List*			coordlist_;
    Coord3List*			normallist_;
    Coord3List*			texturecoordlist_;
    bool			righthandednormals_;

    void			addVersion();
    				/*!<Should be called every time object is
				    changed. */
private:

    int				version_;
};

}; //namespace

#endif
