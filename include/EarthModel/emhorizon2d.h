#ifndef emhorizon2d_h
#define emhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon2d.h,v 1.13 2008-06-19 07:40:05 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emhorizon.h"
#include "bufstringset.h"
#include "horizon2dline.h"
#include "tableascio.h"

namespace Geometry { class Horizon2DLine; }
namespace Table { class FormatDesc; }

namespace EM
{
class EMManager;

class Horizon2DGeometry : public HorizonGeometry
{
public:
				Horizon2DGeometry(Surface&);
    Geometry::Horizon2DLine*	sectionGeometry(const SectionID&);
    const Geometry::Horizon2DLine* sectionGeometry(const SectionID&) const;

    int				nrLines() const;
    int				lineIndex(int id) const;
    int				lineIndex(const char* linenm) const;
    int				lineID(int idx) const;
    const char*			lineName(int id) const;
    const MultiID&		lineSet(int id) const;
    int 			addLine(const MultiID& linesetid, 
					const char* line,int step=1);

    int				addLine(const TypeSet<Coord>&,
					int start,int step,
					const MultiID& lineset,
					const char* linename);
				/*!<\returns id of new line. */
    bool    			syncLine(const MultiID& lset,const char* lnm,
					 const PosInfo::Line2DData&);
    bool			syncBlocked(int id) const;
    void			setLineInfo(int id,const char* linenm,
	    				    const MultiID& linesetid);
    void			removeLine(int id);
    bool			isAtEdge(const PosID&) const;
    PosID			getNeighbor(const PosID&,bool nextcol,
	    				    bool retundef=false) const;
    				/*!<\param retundef specifies what to do
				           if no neighbor is found. If it
					   true, it returnes unf, if not
					   it return the id of the undef
					   neighbor. */
					   
    int				getConnectedPos(const PosID& posid,
						TypeSet<PosID>* res) const;

protected:
    Geometry::Horizon2DLine*	createSectionGeometry() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
    BufferStringSet		linenames_;
    TypeSet<MultiID>		linesets_;
    TypeSet<int>		lineids_;

    static const char*		lineidsstr_;
    static const char*		linenamesstr_;
    static const char*		linesetprefixstr_;

    int				synclineid_;
};

/*!
2d horizons. The horizons is only present along 2d lines, set by addLine. Each
position's subid is formed by RowCol( lineid, tracenr ).getSerialized(). If
multiple z-values per trace is needed, multiple sections can be added. */

class Horizon2D : public Horizon
{ mDefineEMObjFuncs( Horizon2D );
public:

    bool			unSetPos(const EM::PosID&,bool addtohistory);
    bool			unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohistory);

    Horizon2DGeometry&		geometry()		{ return geometry_; }
    const Horizon2DGeometry&	geometry() const	{ return geometry_; }

    void			syncGeometry();
    virtual void		removeAll();

protected:

    const IOObjContext&		getIOObjContext() const;
    Horizon2DGeometry		geometry_;
};


class Horizon2DAscIO : public Table::AscIO
{
public:
    				Horizon2DAscIO( const Table::FormatDesc& fd,
						std::istream& strm )
				    : Table::AscIO(fd)
				    , strm_(strm)      		    {}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&,
	    				   const BufferStringSet&);
    static void                 createDescBody(Table::FormatDesc*,
	    				   const BufferStringSet&);

    float			getUdfVal();
    bool			isXY() const;
    int				getNextLine(BufferString&,TypeSet<float>&);

protected:

    std::istream&		strm_;
};

} // namespace EM

#endif
