#ifndef uiaxishandler_h
#define uiaxishandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
#include "bufstringset.h"
#include "namedobj.h"
#include "uigeom.h"
#include "uistring.h"
#include "uistrings.h"
#include "fontdata.h"

#include <cfloat>

class uiGraphicsScene;
class uiGraphicsItemGroup;
class uiLineItem;
class uiTextItem;
template <class T> class LineParameters;

/*!
\brief Handles an axis on a plot.

  Manages the positions in a 2D plot. The axis can be logarithmic. getRelPos
  returns the relative position on the axis. If the point is between the
  start and stop of the range, this will be between 0 and 1.

  The axis will determine a good position wrt the border. To determine where
  the axis starts and stops, you can provide other axis handlers. If you don't
  provide these, the border_ will be used. The border_ on the side of the axis
  will always be used. If you do use begin and end handlers, you'll have to
  call setRange() for all before using plotAxis().

  The drawAxis will plot the axis. If LineStyle::Type is not LineStyle::None,
  grid lines will be drawn, too. If it *is* None, then still the color and
  size will be used for drawing the axis (the axis' style is always Solid).

  Use AxisLayout (linear.h) to find 'nice' ranges, like:
  AxisLayout al( Interval<float>(start,stop) );
  ahndlr.setRange( StepInterval<float>(al.sd.start,al.stop,al.sd.step) );
*/

mExpClass(uiTools) uiAxisHandler
{
public:

    struct Setup
    {
			Setup( uiRect::Side s, int w=0, int h=0 )
			    : side_(s)
			    , noaxisline_(false)
			    , noaxisannot_(false)
			    , nogridline_(false)
			    , noannotpos_(false)
			    , showauxpos_(false)
			    , annotinside_(false)
			    , annotinint_(false)
			    , fixedborder_(false)
			    , ticsz_(2)
			    , width_(w)
			    , height_(h)
			    , maxnrchars_(0)
			    , epsaroundzero_(FLT_MIN)
			    , islog_(false)
			    , zval_(4)
			    , nmcolor_(Color::NoColor())
			    , fontdata_(FontData(10))
			    {}

	mDefSetupMemb(uiRect::Side,side)
	mDefSetupMemb(int,width)
	mDefSetupMemb(int,height)
	mDefSetupMemb(bool,islog)
	mDefSetupMemb(bool,noannotpos)
	mDefSetupMemb(bool,noaxisline)
	mDefSetupMemb(bool,noaxisannot)
	mDefSetupMemb(bool,nogridline)
	mDefSetupMemb(bool,annotinside)
	mDefSetupMemb(bool,annotinint)
	mDefSetupMemb(bool,fixedborder)
	mDefSetupMemb(bool,showauxpos)
	mDefSetupMemb(int,ticsz)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(LineStyle,style)
	mDefSetupMemb(LineStyle,gridlinestyle)
	mDefSetupMemb(uiString,caption)
	mDefSetupMemb(int,maxnrchars)
	mDefSetupMemb(float,epsaroundzero)
	mDefSetupMemb(int,zval)
	mDefSetupMemb(Color,nmcolor)
	mDefSetupMemb(FontData,fontdata)

	Setup&		noannot( bool yn )
			{ noaxisline_ = noaxisannot_ = nogridline_ = yn;
			  return *this; }
    };

    mStruct(uiTools) AuxPosData
    {
			    AuxPosData()
				: pos_(mUdf(float))
				, name_(uiStrings::sEmptyString())
				, isbold_(false)	{}
	float		pos_;
	bool		isbold_;
	uiString	name_;

	AuxPosData& operator=( const AuxPosData& from )
	{
	    pos_ = from.pos_;
	    isbold_ = from.isbold_;
	    name_ = from.name_;
	    return *this;
	}

	bool 	operator==( const AuxPosData& from ) const
	{ return pos_ == from.pos_ && isbold_ == from.isbold_; }
    };

			uiAxisHandler(uiGraphicsScene*,const Setup&);
			~uiAxisHandler();

    void		setCaption(const uiString&);
    uiString		getCaption() const	{ return setup_.caption_; }
    void		setIsLog( bool yn )	{ setup_.islog_ = yn; reCalc();}
    void		setBorder( const uiBorder& b )
						{ setup_.border_ = b; reCalc();}
    void		setBegin( const uiAxisHandler* ah )
						{ beghndlr_ = ah; newDevSize();}
    void		setEnd( const uiAxisHandler* ah )
						{ endhndlr_ = ah; newDevSize();}

    void		setRange(const StepInterval<float>&,float* axstart=0);
    void		setBounds(Interval<float>); //!< makes annot 'nice'

    float		getVal(int pix) const;
    float		getRelPos(float absval) const;
    int			getPix(float absval) const;
    int			getPix(double abvsval) const;
    int			getPix(int) const;
    int			getRelPosPix(float relpos) const;
    void		setAuxPosData( const TypeSet<AuxPosData>& pos )
			{ auxpos_ = pos; }

    void		updateScene(); //!< update gridlines if appropriate
    void		annotAtEnd(const uiString&);

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }
    StepInterval<float>	range() const		{ return rg_; }
    float		annotStart() const	{ return annotstart_; }
    bool		isHor() const	{ return uiRect::isHor(setup_.side_); }
    int			pixToEdge(bool withborder=true) const;
    int			pixBefore() const;
    int			pixAfter() const;
    Interval<int>	pixRange() const;

			//!< Call this when appropriate
    void		newDevSize();
    void		updateDevSize(); //!< resized from sceme
    void		setNewDevSize(int,int); //!< resized by yourself

    void		updateAnnotations();
    void		updateGridLines();
    uiLineItem*		getFullLine(int pix);
    int			getNrAnnotCharsForDisp() const;
    void		setVisible(bool);

protected:

    uiGraphicsScene*	 scene_;
    uiGraphicsItemGroup* gridlineitmgrp_;
    uiGraphicsItemGroup* annottxtitmgrp_;
    uiGraphicsItemGroup* annotlineitmgrp_;
    uiGraphicsItemGroup* auxposlineitmgrp_;
    uiGraphicsItemGroup* auxpostxtitmgrp_;

    Setup		setup_;
    bool		islog_;
    StepInterval<float>	rg_;
    float		annotstart_;
    uiBorder		border_;
    int			ticsz_;
    int			height_;
    int			width_;
    const uiAxisHandler* beghndlr_;
    const uiAxisHandler* endhndlr_;

    uiLineItem*		axislineitm_;
    uiTextItem*		endannottextitm_;
    uiTextItem*		nameitm_;
    void		reCalc();
    int			calcwdth_;
    uiStringSet		strs_;
    TypeSet<float>	pos_;
    TypeSet<AuxPosData> auxpos_;
    float		endpos_;
    int			devsz_;
    int			axsz_;
    bool		rgisrev_;
    bool		ynmtxtvertical_;
    float		rgwidth_;

    int			ticSz() const;
    void		updateAxisLine();
    void		drawGridLine(int);
    void		drawAnnotAtPos(int,const uiString&,const LineStyle&,
	    			       bool aux=false, bool bold=false);
    void		updateName();

    bool		doPlotExtreme(float plottextrmval,bool isstart) const;
};

//! draws line not outside box defined by X and Y value ranges
mGlobal(uiTools) void setLine(uiLineItem&,const LineParameters<float>&,
			const uiAxisHandler& xah,const uiAxisHandler& yah,
			const Interval<float>* xvalrg = 0);


#endif

