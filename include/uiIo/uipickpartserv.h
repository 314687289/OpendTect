#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.46 2011-06-17 05:23:36 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uipicksetmgr.h"
#include "uiapplserv.h"
#include "ranges.h"
#include "multiid.h"
#include "datapointset.h"
#include "bufstringset.h"
#include "cubesampling.h"

class Color;
class IOObj;
class SurfaceInfo;
class RandLocGenPars;
namespace Pick { class Set; class SetMgr; }
namespace PosInfo { class Line2DData; }


/*! \brief Service provider for application level - seismics */

mClass uiPickPartServer  : public uiApplPartServer
			, public uiPickSetMgr
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			managePickSets();
    Pick::Set*			pickSet()		{ return ps_; }
    void			impexpSet(bool import);
    void			fetchHors(bool);
    bool			loadSets(TypeSet<MultiID>&,bool ispolygon);
    				//!< Load set(s) by user sel
    bool			createEmptySet(bool aspolygon);
    bool			create3DGenSet();
    bool			createRandom2DSet();
    void			setMisclassSet(const DataPointSet&);
    void			setPickSet(const Pick::Set&);
    void			fillZValsFrmHor(Pick::Set*,int);

    static const int		evGetHorInfo2D();
    static const int		evGetHorInfo3D();
    static const int		evGetHorDef3D();
    static const int            evGetHorDef2D();
    static const int            evFillPickSet();
    static const int		evGet2DLineInfo();
    static const int		evGet2DLineDef();


				// Interaction stuff
    BinIDValueSet&			genDef()	{ return gendef_; }
    ObjectSet<PosInfo::Line2DData>&	lineGeoms()	{ return linegeoms_; }

    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos_; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids_; }
    HorSampling			selHorSampling() const	{ return selhs_; }
    MultiID			horID()			{ return horid_; }

    BufferStringSet&		lineSets()		{ return linesets_; }
    TypeSet<BufferStringSet>&	lineNames()		{ return linenms_; }
    TypeSet<MultiID>&		lineSetIds()		{ return setids_; }
    MultiID&			lineSetID()		{ return setid_; }
    BufferStringSet&		selectLines()		{ return selectlines_; }
    TypeSet<Coord>&		getPos2D()		{ return coords2d_; }
    TypeSet< Interval<float> >& getHor2DZRgs()		{ return hor2dzrgs_; }

    uiParent*			parent()
    				{ return appserv().parent(); }

protected:

    BinIDValueSet 			gendef_;
    ObjectSet<PosInfo::Line2DData>	linegeoms_;

    ObjectSet<SurfaceInfo> 	hinfos_;
    ObjectSet<MultiID>		selhorids_;
    HorSampling			selhs_;
    Pick::Set*			ps_;
    MultiID			horid_;

    BufferStringSet		linesets_;
    TypeSet<BufferStringSet>	linenms_;
    TypeSet<MultiID>		setids_;
    MultiID			setid_;
    BufferStringSet		selectlines_;
    TypeSet<Coord>		coords2d_;
    TypeSet< Interval<float> >	hor2dzrgs_;

    bool                        mkRandLocs2D(Pick::Set&,const RandLocGenPars&);
};


#endif
