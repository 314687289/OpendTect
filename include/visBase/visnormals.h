#ifndef visnormals_h
#define visnormals_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visnormals.h,v 1.8 2008-01-18 15:39:20 cvskris Exp $
________________________________________________________________________


-*/


#include "visdata.h"
#include "positionlist.h"

class CallBacker;
class SoNormal;
class Coord3;

namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

class Normals : public DataObject
{
public:
    static Normals*	create()
			mCreateDataObj(Normals);

    void		inverse();
    			//!<Sets all normals to -normal
    int			nrNormals() const;
    			//!<Envelope only, not all may be used.
    void		setNormal( int, const Coord3& );
    int			addNormal( const Coord3& );
    int			nextID(int previd) const;
    void		removeNormal( int );
    Coord3		getNormal(int) const;

    SoNode*		getInventorNode();

protected:
    			~Normals();
    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    SoNormal*			normals_;

    TypeSet<int>		unusednormals_;
    mutable Threads::Mutex&	mutex_;
    			
};

class NormalListAdapter : public Coord3List
{
public:
    		NormalListAdapter(Normals& n )
		    : normals_( n )
		{ normals_.ref(); }

    int		nextID(int previd) const{ return normals_.nextID(previd); }

    int		add(const Coord3& n )	{ return normals_.addNormal(n); }
    void	set(int idx,const Coord3& n)	{ normals_.setNormal(idx,n); }
    void	remove(int idx)		{ normals_.removeNormal(idx); }
    Coord3	get(int idx) const	{ return normals_.getNormal(idx); }

protected:
		~NormalListAdapter()	{ normals_.unRef(); }

    Normals&	normals_;
};

};

#endif

