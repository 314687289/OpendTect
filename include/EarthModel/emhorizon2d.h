#ifndef emhorizon2d_h
#define emhorizon2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emhorizon.h"
#include "bufstringset.h"
#include "horizon2dline.h"
#include "surv2dgeom.h"
#include "tableascio.h"
#include "surv2dgeom.h"
#include "od_istream.h"

class ZAxisTransform;
namespace Geometry { class Horizon2DLine; }
namespace PosInfo { class GeomID; }
namespace Table { class FormatDesc; }
template <class T> class Array1D;

namespace EM
{
class EMManager;

/*!
\brief 2D HorizonGeometry
*/

mExpClass(EarthModel) Horizon2DGeometry : public HorizonGeometry
{
public:
				Horizon2DGeometry(Surface&);
    Geometry::Horizon2DLine*	sectionGeometry(const SectionID&);
    const Geometry::Horizon2DLine* sectionGeometry(const SectionID&) const;

    int				nrLines() const;

    int				lineIndex(const PosInfo::GeomID&) const;
    int				lineIndex(Pos::GeomID geomid) const;

    int				lineIndex(const char* linenm) const;
    const char*			lineName(int id) const;
    const char*			lineSet(int id) const;
    PosInfo::GeomID		lineGeomID(int idx) const;
    Pos::GeomID			geomID(int idx) const;

    bool			includeLine(const PosInfo::GeomID&,int step=1);
    bool			includeLine(Pos::GeomID geomid,int step=1);

    bool 			addLine(const PosInfo::GeomID&,int step=1);
    bool 			addLine(Pos::GeomID geomid,int step=1);

    bool 			addLine(const PosInfo::GeomID&,
					const StepInterval<int>& trcrg);
    bool 			addLine(Pos::GeomID geomid,
					const StepInterval<int>& trcrg);

    void			removeLine(const PosInfo::GeomID&);
    void			removeLine(Pos::GeomID geomid);

    bool			isAtEdge(const PosID&) const;
    PosID			getNeighbor(const PosID&,bool nextcol,
	    				    bool retundef=false) const;
    				/*!<\param retundef specifies what to do
				           if no neighbor is found. If it
					   true, it returnes unf, if not
					   it return the id of the undef
					   neighbor. */

    static const char*		sKeyLineIDs()	{ return "Line IDs"; }
    static const char*		sKeyLineNames()	{ return "Line names"; }
    static const char*		sKeyLineSets()	{ return "Set ID of line "; }
    static const char*		sKeyID()	{ return "ID"; }
    static const char*		sKeyTraceRange()
    				{ return "Trace Range of line "; }
    static const char*		sKeyTrcRg()	{ return "Trace range"; }
    static const char*		sKeyNrLines()	{ return "Nr of Lines"; }

    int				getConnectedPos(const PosID& posid,
						TypeSet<PosID>* res) const;
    StepInterval<int>		colRange(const SectionID&,
	    				 const PosInfo::GeomID&) const;
    StepInterval<int>		colRange(const SectionID&,
					 Pos::GeomID geomid) const;

    StepInterval<int>		colRange(const PosInfo::GeomID&) const;
    StepInterval<int>		colRange(Pos::GeomID geomid) const;

protected:
    Geometry::Horizon2DLine*	createSectionGeometry() const;

    bool 			doAddLine(const PosInfo::GeomID&,
					const StepInterval<int>& trcrg,
					bool mergewithdouble);
    bool 			doAddLine(Pos::GeomID geomid,
					  const StepInterval<int>& trcrg,
					  bool mergewithdouble);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    TypeSet<PosInfo::GeomID>	oldgeomids_;
    TypeSet<Pos::GeomID>	geomids_;
};


/*!
\brief 2D Horizon. The Horizon is only present along 2d lines, set by addLine.
Each position's subid is formed by RowCol( lineid, tracenr ).getInt64(). If
multiple z-values per trace is needed, multiple sections can be added.
*/

mExpClass(EarthModel) Horizon2D : public Horizon
{ mDefineEMObjFuncs( Horizon2D );
public:

    virtual float		getZValue(const Coord&,bool allow_udf=true,
					  int nr=0) const;
    				//!< Convenience function. If you need speed,
    				//!< don't use it.

    bool			unSetPos(const EM::PosID&,bool addtohistory);
    bool			unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohistory);
    Coord3			getPos(const EM::PosID&) const;
    Coord3			getPos(const EM::SectionID&,
				       const EM::SubID&) const;
    TypeSet<Coord3>		getPositions(int lineidx,int trcnr) const;
    Coord3			getPos(EM::SectionID,int lidx,int trcnr) const;

    Coord3			getPos(EM::SectionID,const PosInfo::GeomID&,
	    			       int trcnr) const;
    Coord3			getPosition(EM::SectionID,Pos::GeomID geomid
					    ,int trcnr) const;

    bool			setPos(EM::SectionID,const PosInfo::GeomID&,
	    			       int trcnr,float z,bool addtohistory);
    bool			setPos(EM::SectionID,Pos::GeomID geomid,
	    			       int trcnr,float z,bool addtohistory);

    bool			setPos(const EM::PosID&,const Coord3&,bool);
    bool			setPos(const EM::SectionID&,const EM::SubID&,
	    			       const Coord3&,bool addtohistory);

    Horizon2DGeometry&		geometry()		{ return geometry_; }
    const Horizon2DGeometry&	geometry() const	{ return geometry_; }

    virtual void		removeAll();
    void			removeSelected(const Selector<Coord3>& selector,
	    				       TaskRunner* tr );

    bool			setArray1D(const Array1D<float>&,
					   SectionID sid,
					   const PosInfo::GeomID& geomid,
					   bool onlyfillundefs);
    bool			setArray1D(const Array1D<float>&,
					   const StepInterval<int>& trcrg,
					   SectionID sid,
					   const PosInfo::GeomID& geomid,
					   bool onlyfillundefs);
    bool			setArray1D(const Array1D<float>&,SectionID sid,
	    				   Pos::GeomID geomid,
					   bool onlyfillundefs );

    Array1D<float>*		createArray1D(SectionID,
	    				      const PosInfo::GeomID& geomid,
	    				      const ZAxisTransform* =0) const;
    Array1D<float>*		createArray1D(SectionID,
	    				      Pos::GeomID geomid,
	    				      const ZAxisTransform* =0) const;

protected:

    const IOObjContext&		getIOObjContext() const;
    Horizon2DGeometry		geometry_;
};


/*!
\brief Ascii I/O for Horizon2D.
*/

mExpClass(EarthModel) Horizon2DAscIO : public Table::AscIO
{
public:
    				Horizon2DAscIO( const Table::FormatDesc& fd,
						od_istream& strm )
				    : Table::AscIO(fd)
				    , udfval_(mUdf(float))
				    , finishedreadingheader_(false)
				    , strm_(strm)      		    {}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&,
	    				   const BufferStringSet&);
    static void                 createDescBody(Table::FormatDesc*,
	    				   const BufferStringSet&);

    static bool			isFormatOK(const Table::FormatDesc&,
	    				   BufferString&);
    int				getNextLine(BufferString& lnm,Coord& crd,
					    int& trcnr,TypeSet<float>& data);

protected:

    od_istream			strm_;
    float			udfval_;
    bool			finishedreadingheader_;
};

} // namespace EM

#endif

