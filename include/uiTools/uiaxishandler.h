#ifndef uiaxishandler_h
#define uiaxishandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.h,v 1.16 2009-02-03 08:31:27 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "draw.h"
#include "ranges.h"
#include "bufstringset.h"
#include "namedobj.h"
#include "uigeom.h"
class uiGraphicsScene;
class uiGraphicsItemGroup;
class LinePars;
class uiLineItem;
class uiTextItem;

/*!\brief Handles an axis on a plot

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

mClass uiAxisHandler : public NamedObject
{
public:

    struct Setup
    {
			Setup( uiRect::Side s, int w=0, int h=0 )
			    : side_(s)
			    , noaxisline_(false)
			    , noaxisannot_(false)
			    , nogridline_(false)
			    , width_(w)
			    , height_(h)
			    , islog_(false)	{}

	mDefSetupMemb(uiRect::Side,side)
	mDefSetupMemb(int, width)
	mDefSetupMemb(int, height)
	mDefSetupMemb(bool,islog)
	mDefSetupMemb(bool,noaxisline)
	mDefSetupMemb(bool,noaxisannot)
	mDefSetupMemb(bool,nogridline)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(LineStyle,style)
	mDefSetupMemb(BufferString,name)
    };

			uiAxisHandler(uiGraphicsScene*,const Setup&);
			~uiAxisHandler();
    void		setRange(const StepInterval<float>&,float* axstart=0);
    void		setIsLog( bool yn )	{ setup_.islog_ = yn; reCalc();}
    void		setBorder( const uiBorder& b )
						{ setup_.border_ = b; reCalc();}
    void		setBegin( const uiAxisHandler* ah )
						{ beghndlr_ = ah; newDevSize();}
    void		setEnd( const uiAxisHandler* ah )
						{ endhndlr_ = ah; newDevSize();}

    float		getVal(int pix) const;
    float		getRelPos(float absval) const;
    int			getPix(float absval) const;

    void		plotAxis(); //!< draws gridlines if appropriate
    void		annotAtEnd(const char*);

    const Setup&	setup() const	{ return setup_; }
    Setup&		setup() 	{ return setup_; }
    StepInterval<float>	range() const	{ return rg_; }
    bool		isHor() const	{ return uiRect::isHor(setup_.side_); }
    int			pixToEdge() const;
    int			pixBefore() const;
    int			pixAfter() const;
    Interval<int>	pixRange() const;

    void		newDevSize(); //!< Call this when appropriate
    void		setNewDevSize(int devsz, int anotherdim ); //!< Call this when appropriate

    void		createAnnotItems();
    void		createGridLines();
    void		drawGridLine(int); //!< Already called by plotAxis

protected:

    uiGraphicsScene*	 scene_;
    uiGraphicsItemGroup* gridlineitmgrp_;
    uiGraphicsItemGroup* annottxtitmgrp_;
    uiGraphicsItemGroup* annotlineitmgrp_;

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
    int			wdthx_;
    int			wdthy_;
    BufferStringSet	strs_;
    TypeSet<float>	pos_;
    float		endpos_;
    int			devsz_;
    int			axsz_;
    bool		rgisrev_;
    bool		ynmtxtvertical_;
    float		rgwidth_;

    int			getRelPosPix(float) const;
    void		drawAxisLine();
    void		annotPos(int,const char*,const LineStyle&);
    void		drawName();

};

//! draws line not outside box defined by X and Y value ranges
mGlobal void drawLine(uiLineItem&,const LinePars&,const uiAxisHandler& xah,
	      const uiAxisHandler& yah,const Interval<float>* xvalrg = 0);


#endif
