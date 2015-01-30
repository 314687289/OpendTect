#ifndef welldata_h
#define welldata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"

#include "callback.h"
#include "multiid.h"
#include "namedobj.h"
#include "position.h"
#include "refcount.h"
#include "sets.h"
#include "welld2tmodel.h"


namespace Well
{

class Track;
class LogSet;
class Marker;
class MarkerSet;
class D2TModel;
class DisplayProperties;


/*!
\brief Information about a certain well.
*/

mExpClass(Well) Info : public ::NamedObject
{
public:

			Info( const char* nm )
			    : ::NamedObject(nm)
			    , srdelev(0)
			    , replvel(Well::getDefaultVelocity())
			    , groundelev(mUdf(float))	{}

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

    BufferString	uwid;
    BufferString	oper;
    BufferString	state;
    BufferString	county;
    BufferString	source_; //!< filename for OD storage

    Coord		surfacecoord;
    float		srdelev;
    float		replvel;
    float		groundelev;

    static const char*	sKeyuwid();
    static const char*	sKeyoper();
    static const char*	sKeystate();
    static const char*	sKeycounty();
    static const char*	sKeycoord();
    static const char*	sKeykbelev();
    static const char*	sKeyOldelev();
    static const char*	sKeySRD();
    static const char*	sKeyreplvel();
    static const char*	sKeygroundelev();
    static int		legacyLogWidthFactor();

};


/*!
\brief The holder of all data concerning a certain well.

  Note that a well is not a POSC well in the sense that it describes the data
  for one well bore. Thus, a well has a single track. This may mean duplication
  when more well tracks share an upper part.
*/

mExpClass(Well) Data : public CallBacker
{ mRefCountImplWithDestructor(Data, virtual ~Data(),
{prepareForDelete(); delete this; } );
public:

				Data(const char* nm=0);

    const MultiID&		multiID() const		{ return mid_; }
    void			setMultiID( const MultiID& mid ) const
							{ mid_ = mid; }

    const char*			name() const		{ return info_.name(); }
    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }
    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }
    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }
    const MarkerSet&		markers() const		{ return markers_; }
    MarkerSet&			markers()		{ return markers_; }
    const D2TModel*		d2TModel() const	{ return d2tmodel_; }
    D2TModel*			d2TModel()		{ return d2tmodel_; }
    const D2TModel*		checkShotModel() const	{ return csmodel_; }
    D2TModel*			checkShotModel()	{ return csmodel_; }
    void			setD2TModel(D2TModel*);	//!< becomes mine
    void			setCheckShotModel(D2TModel*); //!< mine, too
    DisplayProperties&		displayProperties( bool for2d=false )
				    { return for2d ? disp2d_ : disp3d_; }
    const DisplayProperties&	displayProperties( bool for2d=false ) const
				    { return for2d ? disp2d_ : disp3d_; }

    void			setEmpty(); //!< removes everything

    void			levelToBeRemoved(CallBacker*);

    bool			haveLogs() const;
    bool			haveMarkers() const;
    bool			haveD2TModel() const	{ return d2tmodel_; }
    bool			haveCheckShotModel() const { return csmodel_; }

    Notifier<Well::Data>	d2tchanged;
    Notifier<Well::Data>	csmdlchanged;
    Notifier<Well::Data>	markerschanged;
    Notifier<Well::Data>	trackchanged;
    Notifier<Well::Data>	disp3dparschanged;
    Notifier<Well::Data>	disp2dparschanged;

protected:
    void		prepareForDelete() const;

    Info		info_;
    mutable MultiID	mid_;
    Track&		track_;
    LogSet&		logs_;
    D2TModel*		d2tmodel_;
    D2TModel*		csmodel_;
    MarkerSet&		markers_;
    DisplayProperties&	disp2d_;
    DisplayProperties&	disp3d_;
};

} // namespace Well

#endif

