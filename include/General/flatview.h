#ifndef flatview_h
#define flatview_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2005
 RCS:           $Id: flatview.h,v 1.42 2009-02-02 21:55:17 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "geometry.h"
#include "position.h"
#include "datapackbase.h"
#include "draw.h"
#include "samplingdata.h"

class IOPar;
class FlatView_CB_Rcvr;

namespace ColTab { struct MapperSetup; };


namespace FlatView
{

typedef Geom::Point2D<double> Point;
typedef Geom::PosRectangle<double> Rect;


/*!\brief Annotation data for flat views */

mClass Annotation
{
public:


    //!\brief Things like well tracks, cultural data, 2-D line positions
    mClass AuxData
    {
    public:
	//!\brief explains what part of the an auxdata's appearance that may be
	//!	  edited by the user
	mClass EditPermissions
	{
	public:			EditPermissions();
	    bool		onoff_;
	    bool		namepos_;
	    bool		linestyle_;
	    bool		linecolor_;
	    bool		fillcolor_;
	    bool		markerstyle_;
	    bool		markercolor_;
	    bool		x1rg_;
	    bool		x2rg_;
	};
				AuxData( const char* nm );
				AuxData( const AuxData& );
				~AuxData();

	EditPermissions*	editpermissions_;//!<If null no editing allowed

	bool			enabled_; 	//!<Turns on/off everything
	BufferString		name_;
	Alignment		namealignment_;
	int			namepos_;	//!<nodraw=udf, before first=-1,
						//!< center=0, after last=1
	LineStyle		linestyle_;
	Color			fillcolor_;
	TypeSet<MarkerStyle2D>	markerstyles_;
	bool			areMarkersVisible() const;

	Interval<double>*	x1rg_;		//!<if 0, use viewer's rg & zoom
	Interval<double>*	x2rg_;		//!<if 0, use viewer's rg & zoom

	TypeSet<Point>		poly_;
	bool			close_;

	bool			isEmpty() const;
	void			empty();
    };

    mStruct AxisData
    {
				AxisData();

	BufferString		name_;
	SamplingData<float>	sampling_;
	bool			showannot_;
	bool			showgridlines_;
	bool			reversed_;

	void			showAll(bool yn);
    };


				Annotation(bool darkbg);
				~Annotation();

    BufferString		title_; //!< color not settable
    Color			color_; //!< For axes
    AxisData			x1_;
    AxisData			x2_;
    ObjectSet<AuxData>		auxdata_;
    bool			showaux_;

    inline void		setAxesAnnot( bool yn ) //!< Convenience all or nothing
			{ x1_.showAll(yn); x2_.showAll(yn); }
    inline bool		haveTitle() const
    			{ return !title_.isEmpty(); }
    inline bool		haveAxisAnnot( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showannot_)
				|| (!x1dir && x2_.showannot_)); }
    inline bool		haveGridLines( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showgridlines_)
				|| (!x1dir && x2_.showgridlines_)); }
    bool		haveAux() const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyAxes;
    static const char*	sKeyX1Sampl;
    static const char*	sKeyX2Sampl;
    static const char*	sKeyShwAnnot;
    static const char*	sKeyShwGridLines;
    static const char*	sKeyIsRev;
    static const char*	sKeyShwAux;

};


/*!\brief Data display paramters

  When data needs to be displayed, there is a load of parameters and
  options for display. The two main display modes are:
  Variable Density = display according to color table
  Wiggle/Variable Area = wiggles with (possibly) filled amplitude

  */

mClass DataDispPars
{
public:

    //!\brief Common to VD and WVA
    mClass Common
    {
    public:
			Common();

	bool		show_;	   // default=true
	bool		autoscale_;// default=true
	Interval<float>	rg_;	   // default=mUdf(float)
	Interval<float>	clipperc_; // default from ColorTable
				   // stop=undef
	bool		blocky_;   // default=false
	float		symmidvalue_; //!< undef => auto data mid
	bool		histeq_; //!< undef => auto data mid
	void		fill(ColTab::MapperSetup&) const;
    };

    //!\brief Variable Density (=color-bar driven) parameters
    mClass VD : public Common
    {
    public:

			VD()		{}	

	BufferString	ctab_;

    };
    //!\brief Wiggle/Variable Area parameters
    mClass WVA : public Common
    {
    public:

		    WVA()
			: wigg_(Color::Black())
			, mid_(Color::NoColor())
			, left_(Color::NoColor())
			, right_(Color::DgbColor())
			, overlap_(1)		{ midlinevalue_ = 0; }

	Color		wigg_;
	Color		mid_;
	Color		left_;
	Color		right_;
	float		overlap_;
	float		midlinevalue_;
    };

    			DataDispPars()		{}

    VD			vd_;
    WVA			wva_;
    void		show( bool wva, bool vd )
    			{ wva_.show_ = wva; vd_.show_ = vd; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyVD;
    static const char*	sKeyWVA;
    static const char*	sKeyShow;
    static const char*	sKeyDispRg;
    static const char*	sKeyColTab;
    static const char*	sKeyBlocky;
    static const char*  sKeyAutoScale;
    static const char*	sKeyClipPerc;
    static const char*	sKeyWiggCol;
    static const char*	sKeyMidCol;
    static const char*	sKeyLeftCol;
    static const char*	sKeyRightCol;
    static const char*	sKeyOverlap;
    static const char*	sKeySymMidValue;
    static const char*	sKeyMidLineValue;
};


/*!\brief Flat views: Appearance  */

mClass Appearance
{
public:
    			Appearance( bool drkbg=true )
			    : darkbg_(drkbg)
			    , annot_(drkbg)
			    , secondsetaxes_(drkbg)
			    , anglewithset1_(0)		{}

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Annotation		annot_;
    DataDispPars	ddpars_;

    //Second set of axes, especially handy in case flat viewer is horizontal
    Annotation		secondsetaxes_;
    float		anglewithset1_;
    
    void		setDarkBG(bool yn);
    bool		darkBG() const		{ return darkbg_; }

    void		setGeoDefaults( bool vert )
			{ if ( vert ) annot_.x2_.reversed_ = true; }

protected:

    bool		darkbg_;	//!< Two styles: dark (=black)
					//!< and lite (=white) background
    					//!< Impacts a lot of display choices

};



/*!\brief Flat Viewer using FlatView::Data and FlatView::Appearance

  Interface for displaying data and related annotations where at least one of
  the directions is sampled regularly.

  The viewer works with FlatDataPacks - period. in previous versions, you could
  pass 'loose' data but DataPacks are so neat&clean we got rid of this
  possibility.
  
  You can attach many datapacks to the viewer; the different modes (WVA, VD)
  can be attached to zero or one of those packs. If you pass the data pack in
  observe mode, the viewer will not release the pack when it's deleted or
  when removePack is called for the pack.

  addPack() -> add a DataPack to the list of available data packs
  usePack() -> sets one of the available packs for wva of vd display
  setPack() -> Combination of addPack and usePack.
  removePack() -> removes this pack from the available packs, if necessary
  		  it also clears the wva or vd display to no display.

  */

mClass Viewer
{
public:

    			Viewer();
    virtual		~Viewer();

    virtual Appearance&	appearance();
    const Appearance&	appearance() const
    			{ return const_cast<Viewer*>(this)->appearance(); }

    void		addPack(::DataPack::ID,bool observe=false);
    			//!< Adds to list, but doesn't use for WVA or VD
    void		usePack(bool wva,::DataPack::ID,bool usedefs=true);
    			//!< Does not add new packs, just selects from added
    void		removePack(::DataPack::ID);
    void		setPack( bool wva, ::DataPack::ID id, bool obs,
				 bool usedefs=true )
			{ addPack( id, obs ); usePack( wva, id, usedefs ); }

    const FlatDataPack*	pack( bool wva ) const
			{ return wva ? wvapack_ : vdpack_; }
    DataPack::ID	packID( bool wva ) const
			{ return pack(wva) ? pack(wva)->id()
			    		   : ::DataPack::cNoID(); }
    const TypeSet< ::DataPack::ID>&	availablePacks() const
							{ return ids_; }

    virtual bool	isVertical() const		{ return true; }
    bool		isVisible(bool wva) const;
    			//!< Depends on show_ and availability of data

    enum DataChangeType	{ None, All, Annot, WVAData, VDData, WVAPars, VDPars };
    virtual void	handleChange(DataChangeType,bool dofill=true)	= 0;

    			//!Does not store any data, just how data is displayed
    virtual void	fillAppearancePar( IOPar& iop ) const
			{ appearance().fillPar( iop ); }
    			/*!Does not retrieve any data, just how data is
			  displayed */
    virtual void	useAppearancePar( const IOPar& iop )
			{ appearance().usePar( iop ); }

    void		storeDefaults(const char* key) const;
    void		useStoredDefaults(const char* key);

    void		getAuxInfo(const Point&,IOPar&) const;
    
    virtual Interval<float> getDataRange(bool) const
    			{ return Interval<float>(mUdf(float),mUdf(float)); }

protected:

    TypeSet< ::DataPack::ID>	ids_;
    BoolTypeSet			obs_;
    const FlatDataPack*		wvapack_;
    const FlatDataPack*		vdpack_;
    Appearance*			defapp_;
    DataPackMgr&		dpm_;
    FlatView_CB_Rcvr*		cbrcvr_;

    void			addAuxInfo(bool,const Point&,IOPar&) const;

};


} // namespace FlatView

#endif
