#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "picklocation.h"
#include "picklabel.h"
#include "sharedobject.h"
#include "monitoriter.h"
#include "enums.h"
#include "odpresentationmgr.h"
#include "trckey.h"
#include "sets.h"
#include "draw.h"
#include "iopar.h"
template <class T> class ODPolygon;


namespace Pick
{


/*!\brief Monitorable set of pick locations.

  A Pick::Set is either a loose bunch of locations, or a connected set of
  points: a polygon. Apart from this, a set may be labeled to be part of a
  'category', like 'ArrowAnnotations'.

  Each location has its own unique ID, the LocID.

*/

mExpClass(General) Set	: public SharedObject
{
public:

    typedef TypeSet<Location>::size_type	size_type;
    typedef size_type				IdxType;
    mDefIntegerIDType(IdxType,			LocID);
    typedef Label::ID				LabelID;

			Set(const char* nm=0,const char* category=0);
			mDeclInstanceCreatedNotifierAccess(Set);
			mDeclMonitorableAssignment(Set);

    bool		isPolygon() const;
    void		setIsPolygon(bool yn=true);
    void		setCategory(const char*);
    BufferString	type() const; //!< sKey::Polygon() or sKey::PickSet()
    BufferString	category() const;

    inline bool		isEmpty() const			    { return size()<1; }
    size_type		size() const;
    bool		validLocID(LocID) const;
    IdxType		idxFor(LocID) const;
    LocID		locIDAfter(LocID) const;
    bool		validIdx(IdxType) const;
    LocID		locIDFor(IdxType) const;

    Location		get(LocID) const;
    Coord		getPos(LocID) const;
    double		getZ(LocID) const;
    Location		first() const;
    Location		getByIndex(IdxType) const;

    Set&		setEmpty();
    Set&		append(const Set&);
    LocID		insertBefore(LocID,const Location&);
    LocID		add(const Location&);
    LocID		remove(LocID); //!< returns ID of loc taking its place
    Set&		set(LocID,const Location&,bool istemp=false);
    Set&		setPos(LocID,const Coord&,bool istemp=false);
    Set&		setZ(LocID,double,bool istemp=false);
    Set&		setByIndex(IdxType,const Location&,bool istemp=false);

			// Manipulate the defined labels
    void		addLabel(const Label&);
    void		removeLabel(LabelID);
    Label		getLabel(LabelID) const;
    LabelID		getLabelByText(const char*) const; //!< first match
    void		setLabel(const Label&); //!< adds if ID udf

			// Get/set location labels
    bool		haveLabels() const; //!< at any of the locations
    LabelID		labelID(LocID) const;
    LabelID		labelIDByIdx(IdxType) const;
    void		setLabelID(LocID,LabelID);
    void		setLabelIDs(Interval<IdxType>,LabelID);

    bool		isMultiGeom() const;
    Pos::GeomID		firstGeomID() const;
    bool		has2D() const;
    bool		has3D() const;
    bool		hasOnly2D() const;
    bool		hasOnly3D() const;

    void		getPolygon(ODPolygon<double>&) const; // coords
    void		getPolygon(ODPolygon<float>&) const; // binids
    void		getLocations(TypeSet<Coord>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    LocID		find(const TrcKey&) const;
    LocID		nearestLocation(const Coord&) const;
    LocID		nearestLocation(const Coord3&,bool ignorez=false) const;
    bool		removeWithPolygon(const ODPolygon<double>&,
					  bool inside=true);

    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,cParsChange())
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		updateInPar(const char* ky,const char* val);

    mExpClass(General) Disp
    {
    public:
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection);
			Disp() : connect_(None) {}
	inline bool	operator ==( const Disp& oth ) const
			{ return connect_ == oth.connect_
			      && mkstyle_ == oth.mkstyle_; }
	inline bool	operator !=( const Disp& oth ) const
			{ return !(*this == oth); }

	Connection		connect_;	//!< connect picks in set order
	OD::MarkerStyle3D	mkstyle_;
    };
    mImplSimpleMonitoredGetSet(inline,getDisp,setDisp,Disp,disp_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,connection,setConnection,
				Disp::Connection,disp_.connect_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,markerStyle,setMarkerStyle,
				OD::MarkerStyle3D,disp_.mkstyle_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispColor,setDispColor,
				Color,disp_.mkstyle_.color_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispSize,setDispSize,
				int,disp_.mkstyle_.size_,cDispChange())

    static ChangeType	cDispChange()		{ return 2; }
    static ChangeType	cParsChange()		{ return 3; }
    static ChangeType	cLabelsChange()		{ return 4; }
    static ChangeType	cLocationInsert()	{ return 5; }
    static ChangeType	cLocationRemove()	{ return 6; }
    static ChangeType	cLocationPreChange()	{ return 7; }
    static ChangeType	cLocationChange()	{ return 8; }
    static ChangeType	cLocationChangeTemp()	{ return 9; }
    static inline bool	isLocationUpdate( ChangeType ct )
			{ return ct > cLabelsChange(); }
    static inline bool	isLocationChange( ChangeType ct )
			{ return ct > 6; }
    static inline bool	isTempChange( ChangeType ct )
			{ return ct > 8; }

    static const Set&	emptySet()		{ return emptyset_; }
    static Set&		dummySet()		{ return dummyset_; }

    static const char*	sKeyMarkerType()	{ return "Marker Type"; }

protected:

			~Set();

    TypeSet<Location>	locs_;
    TypeSet<LocID>	locids_;
    TypeSet<Label>	labels_;
    Disp		disp_;
    IOPar		pars_;
    mutable Threads::Atomic<IdxType> curlocidnr_;
    mutable Threads::Atomic<int>     curlabelidnr_;
    static const Set	emptyset_;
    static Set		dummyset_;

    IdxType		gtIdxFor(LocID) const;
    LocID		insNewLocID(IdxType,AccessLocker&);
    void		replaceID(LocID from,LocID to);
    int			gtLblIdx(LabelID) const;

    friend class	SetIter;
    friend class	SetIter4Edit;
    friend class	SetChangeRecord;

};


/*!\brief const Set iterator. */

mExpClass(General) SetIter : public MonitorableIter4Read<Set::IdxType>
{
public:

			SetIter(const Set&,bool start_at_end=false);
			SetIter(const SetIter&);
    const Set&		pickSet() const;

    Set::LocID		ID() const;
    const Location&	get() const;
    Coord		getPos() const;
    double		getZ() const;

};


/*!\brief non-const Set iterator. Does not lock, so use this for non-shared
  Pick::Set's only. Really, because it locks totally nothing, many methods
  bypass the stand-alone Pick::Set's locking.

  Needs a next() before a valid LocID is reached.

  */

mExpClass(General) SetIter4Edit : public MonitorableIter4Write<Set::IdxType>
{
public:

			SetIter4Edit(Set&,bool start_at_end=false);
			SetIter4Edit(const SetIter4Edit&);
    Set&		pickSet();
    const Set&		pickSet() const;

    Set::LocID		ID() const;
    Location&		get() const;
    void		removeCurrent();
    void		insert(const Pick::Location&);

};


mExpClass(General) SetPresentationInfo : public OD::ObjPresentationInfo
{
public:

					SetPresentationInfo(const DBKey&);
					SetPresentationInfo();

    static OD::ObjPresentationInfo*	createFrom( const IOPar&);
    static void				initClass();
    static const char*			sFactoryKey();

};


} // namespace Pick
