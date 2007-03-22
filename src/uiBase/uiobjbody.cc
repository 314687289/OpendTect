/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiobjbody.cc,v 1.13 2007-03-22 13:23:07 cvsdgb Exp $
________________________________________________________________________

-*/


#include "uiobjbody.h"
#include "i_layout.h"
#include "i_layoutitem.h"
#include "errh.h"
#include "timer.h"
#include "pixmap.h"
#include "color.h"

#include <qpixmap.h>
#include <qtooltip.h>


#define mParntBody( p ) dynamic_cast<uiParentBody*>( p->body() )

    
uiObjectBody::uiObjectBody( uiParent* parnt, const char* nm )
    : uiBody()
    , NamedObject( nm )
    , layoutItem_( 0 )
    , parent_( parnt ? mParntBody(parnt) : 0  )
    , font_( 0 )
    , hStretch( mUdf(int)  )
    , vStretch(  mUdf(int) )
    , allowshrnk( false )
    , is_hidden( false )
    , finalised( false )
    , display_( true )
    , display_maximised( false )
    , pref_width_( 0 )
    , pref_height_( 0 )
    , pref_width_set( - 1 )
    , pref_char_width( -1 )
    , pref_height_set( -1 )
    , pref_char_height( -1 )
    , pref_width_hint( 0 )
    , pref_height_hint( 0 )
    , fnt_hgt( 0 )
    , fnt_wdt( 0 )
    , fnt_maxwdt( 0 )
    , hszpol( uiObject::Undef )
    , vszpol( uiObject::Undef )
#ifdef USE_DISPLAY_TIMER
    , displTim( *new Timer("Display timer"))
{ 
    displTim.tick.notify(mCB(this,uiObjectBody,doDisplay));
}
#else
{}
#endif


uiObjectBody::~uiObjectBody() 
{
#ifdef USE_DISPLAY_TIMER
    delete &displTim;
#endif
    delete layoutItem_;
}


void uiObjectBody::display( bool yn, bool shrink, bool maximised )
{
    display_ = yn;
    display_maximised = maximised;

    if ( !display_ && shrink )

    {
	pref_width_  = 0;
	pref_height_ = 0;

	is_hidden = true;

	qwidget()->hide();
    }
    else
    {
#ifdef USE_DISPLAY_TIMER
	if ( displTim.isActive() ) displTim.stop();
	displTim.start( 1, true );
#else
	doDisplay(0);
#endif
    }
}

void uiObjectBody::doDisplay(CallBacker*)
{
    if ( !finalised ) finalise();


    if ( display_ )
    {
	is_hidden = false;

	if ( display_maximised )	qwidget()->showMaximized();
	else			qwidget()->show();
    }
    else
    {
	if ( !is_hidden )
	{
	    int sz = prefHNrPics();
	    sz = prefVNrPics();
	    is_hidden = true;

	    qwidget()->hide();
	}
    }
}


void uiObjectBody::uisetFocus()
{ qwidget()->setFocus(); }

bool uiObjectBody::uihasFocus() const
{ return qwidget() ? qwidget()->hasFocus() : false; }


void uiObjectBody::uisetSensitive( bool yn )
{ qwidget()->setEnabled( yn ); }

bool uiObjectBody::uisensitive() const
{ return qwidget() ? qwidget()->isEnabled() : false; }


void uiObjectBody::reDraw( bool deep )
{
    qwidget()->update();
}


i_LayoutItem* uiObjectBody::mkLayoutItem( i_LayoutMngr& mngr )
{
    if( layoutItem_ )
    {
	pErrMsg("Already have layout itm");
	return layoutItem_ ;
    }

    layoutItem_ = mkLayoutItem_( mngr );
    return layoutItem_;
}


void uiObjectBody::finalise()
{
    if ( finalised ) return;

    uiObjHandle().finaliseStart.trigger( uiObjHandle() );
    finalise_();
    finalised = true;
    uiObjHandle().finaliseDone.trigger( uiObjHandle() );
    if ( !display_ ) 
	display( display_ );
}


void uiObjectBody::fontchanged()
{
    fnt_hgt = 0; fnt_wdt = 0; fnt_maxwdt = 0;
    pref_width_hint=0;
    pref_height_hint=0;
}


int uiObjectBody::fontHgt() const
{
    gtFntWdtHgt();
    return fnt_hgt;
}


int uiObjectBody::fontWdt( bool max ) const
{
    gtFntWdtHgt();
    return max ? fnt_maxwdt : fnt_wdt;
}


void uiObjectBody::setHSzPol( uiObject::SzPolicy pol )
{
    hszpol = pol;
    if ( pol >= uiObject::SmallMax && pol <= uiObject::WideMax )
    {
	int vs = stretch( false, true );
	setStretch( 2, vs);
    }
}


void uiObjectBody::setVSzPol( uiObject::SzPolicy pol )
{
    vszpol = pol;
    if ( pol >= uiObject::SmallMax && pol <= uiObject::WideMax )
    {
	int hs = stretch( true, true );
	setStretch( hs, 2 );
    }
}


const Color& uiObjectBody::uibackgroundColor() const
{
    return *new Color( qwidget()->backgroundColor().rgb() );
}


void uiObjectBody::uisetBackgroundColor( const Color& c )
{
    qwidget()->setPaletteBackgroundColor( QColor( QRgb( c.rgb() ) ) );
}


void uiObjectBody::uisetBackgroundPixmap( const char* img[] )
{
    qwidget()->setPaletteBackgroundPixmap( QPixmap(img) );
}


void uiObjectBody::uisetBackgroundPixmap( const ioPixmap& pm )
{
    qwidget()->setPaletteBackgroundPixmap( *pm.qpixmap() );
}


#define mChkLayoutItm() if ( !layoutItem_ ) { return 0; } 

void uiObjectBody::getSzHint()
{
    if ( pref_width_hint && pref_height_hint ) return;
    if ( pref_width_hint || pref_height_hint ) 
	{ pErrMsg("Only 1 defined size.."); }

    uiSize sh = layoutItem_->prefSize();

    pref_width_hint = sh.hNrPics();
    pref_height_hint = sh.vNrPics();
}


int uiObjectBody::prefHNrPics() const
{
    if ( pref_width_ <= 0 )
    {
	if ( is_hidden )		{ return pref_width_; }
	mChkLayoutItm();

	if ( pref_width_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_set; }
	else if ( pref_char_width >= 0 ) 
	{
	    int fw = fontWdt();
	    if ( !fw ){ pErrMsg("Font has 0 width."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_width_ =
					     mNINT( pref_char_width * fw ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();

	    const int baseFldSz = uiObject::baseFldSize();

	    int pwc=0;
	    bool var=false; 
	    switch( szPol(true) )
	    {
		case uiObject::Small:    pwc=baseFldSz;     break;
		case uiObject::Medium:   pwc=2*baseFldSz+1; break;
		case uiObject::Wide:     pwc=4*baseFldSz+3; break;

		case uiObject::SmallMax:
		case uiObject::SmallVar: pwc=baseFldSz;     var=true; break;

		case uiObject::MedMax:
		case uiObject::MedVar:   pwc=2*baseFldSz+1; var=true; break;

		case uiObject::WideMax:
		case uiObject::WideVar:  pwc=4*baseFldSz+3; var=true; break;
	    }

	    if ( !pwc )
		const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_hint;
	    else
	    {
		int fw = fontWdt();
		if ( !fw ){ pErrMsg("Font has 0 width."); }

		int pw = var ? mMAX(pref_width_hint, fw*pwc ) : fw*pwc ;
		const_cast<uiObjectBody*>(this)->pref_width_ = pw;
            }
	}
    }
    return pref_width_;
}


void uiObjectBody::setPrefWidth( int w )
{
    if( itemInited() )
    {
	if( pref_width_set != w )
	pErrMsg("Not allowed when finalized.");
	return;
    }

    pref_char_width = -1;
    pref_width_set = w;
    pref_width_  = 0;
    pref_height_ = 0;
}


void uiObjectBody::setPrefWidthInChar( float w )
{
    if( itemInited() )
    {
	if( pref_char_width != w )
	pErrMsg("Not allowed when finalized.");
	return;
    }

    pref_width_set = -1;
    pref_char_width = w;
    pref_width_  = 0;
    pref_height_ = 0;
}


int uiObjectBody::prefVNrPics() const
{
    if ( pref_height_ <= 0 )
    {
	if ( is_hidden )		{ return pref_height_; }
	mChkLayoutItm();

	if ( pref_height_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_height_= pref_height_set;}
	else if ( pref_char_height >= 0 ) 
	{
	    int fh = fontHgt();
	    if ( !fh ){ pErrMsg("Font has 0 height."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_height_ =
					    mNINT( pref_char_height * fh ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();

	    if ( nrTxtLines() < 0 && szPol(false) == uiObject::Undef  )
		const_cast<uiObjectBody*>(this)->pref_height_= pref_height_hint;
	    else 
	    {
		float lines = 1.51;
		if ( nrTxtLines() == 0 )	lines = 7;
		else if ( nrTxtLines() > 1 )	lines = nrTxtLines();

		const int baseFldSz = 1;

		bool var=false; 
		switch( szPol(false) )
		{
		    case uiObject::Small:    lines=baseFldSz;     break;
		    case uiObject::Medium:   lines=2*baseFldSz+1; break;
		    case uiObject::Wide:     lines=4*baseFldSz+3; break;

		    case uiObject::SmallMax:
		    case uiObject::SmallVar:
			    lines=baseFldSz; var=true; break;

		    case uiObject::MedMax:
		    case uiObject::MedVar: 
			    lines=2*baseFldSz+1; var=true; break;

		    case uiObject::WideMax:
		    case uiObject::WideVar:
			    lines=4*baseFldSz+3; var=true; break;
		}


		int fh = fontHgt();
		if ( !fh ){ pErrMsg("Font has 0 height."); return 0; }

		int phc = mNINT( lines * fh);
		const_cast<uiObjectBody*>(this)->pref_height_=
				var ? mMAX(pref_height_hint, phc ) : phc ;
	    }
	}
    }

    return pref_height_;
}


void uiObjectBody::setPrefHeight( int h )
{
    if( itemInited() )
    {
	if( pref_height_set != h )
	pErrMsg("Not allowed when finalized.");
	return;
    }

    pref_char_height = -1;
    pref_height_set = h;
    pref_width_  = 0;
    pref_height_ = 0;
}


void uiObjectBody::setPrefHeightInChar( float h )
{
    if( itemInited() )
    {
	if( pref_char_height != h )
	pErrMsg("Not allowed when finalized.");
	return;
    }

    pref_height_set = -1;
    pref_char_height = h;
    pref_width_  = 0;
    pref_height_ = 0;
}


void uiObjectBody::setStretch( int hor, int ver )
{
    if( itemInited() )
    {
	if( hStretch != hor || vStretch != ver )
	pErrMsg("Not allowed when finalized.");
	return;
    }
    
    hStretch = hor; 
    vStretch = ver;
}


int uiObjectBody::stretch( bool hor, bool retUndef ) const
{
    int s = hor ? hStretch : vStretch;
    if ( retUndef ) return s;

    return mIsUdf(s) ? 0 : s;
}


uiSize uiObjectBody::actualsize( bool include_border ) const
{
    return layoutItem_->actualsize( include_border );
}


void uiObjectBody::setToolTip( const char* txt )
{
#ifdef USEQT3
    QToolTip::add( qwidget(), txt );
#else
    qwidget()->setToolTip( txt );
#endif
}

#ifdef USEQT3
void uiObjectBody::enableToolTips( bool yn )
{
    QToolTip::setEnabled( yn );
}


bool uiObjectBody::toolTipsEnabled()
    { return QToolTip::enabled(); }
#endif

void uiObjectBody::uisetCaption( const char* str )
    { qwidget()->setCaption( QString( str ) ); }

i_LayoutItem* uiObjectBody::mkLayoutItem_( i_LayoutMngr& mngr )
    { return new i_uiLayoutItem( mngr , *this ); }



/*!
    attaches to parent if other=0
*/
void uiObjectBody::attach ( constraintType tp, uiObject* other, int margin,
			    bool reciprocal )
{
//    parent_->attachChild( tp, this, other, margin );
    parent_->attachChild( tp, &uiObjHandle(), other, margin, reciprocal );
}

const uiFont* uiObjectBody::uifont() const
{
    if ( !font_ )
    { 
// TODO: implement new font handling. uiParent should have a get/set font 
//       and all children should use this font.
#if 0
	const_cast<uiObjectBody*>(this)->font_ = 
					&uiFontList::get(className(*this)); 
#else
	QFont qf( qwidget()->font() );
	const_cast<uiObjectBody*>(this)->font_ = &uiFontList::getFromQfnt(&qf); 
#endif
	const_cast<uiObjectBody*>(this)->qwidget()->setFont( font_->qFont() );
    }

    return font_;
}


void uiObjectBody::uisetFont( const uiFont& f )
{
    font_ = &f;
    qwidget()->setFont( font_->qFont() );
}

int uiObjectBody::fontWdtFor( const char* str ) const
{
    gtFntWdtHgt();
    const QWidget* qw = qwidget();
    if ( !qw || __iswin__ )
	{ gtFntWdtHgt(); return strlen(str) * fnt_wdt; }

    return qw->fontMetrics().width( QString( str ) );
}

bool uiObjectBody::itemInited() const
{
    return layoutItem_ ? layoutItem_->inited() : false;
}


void uiObjectBody::gtFntWdtHgt() const
{
    if ( fnt_hgt && fnt_wdt && fnt_maxwdt )
	return;

    uiObjectBody& self = *const_cast<uiObjectBody*>(this);
    const QWidget* qw = qwidget();

//TODO: QFontMetrics simply crashes on windows
    if ( !qw || __iswin__ )
    {
	self.fnt_hgt = 14; self.fnt_wdt = 10; self.fnt_maxwdt = 15;
	return;
    }

    QFontMetrics fm( qw->fontMetrics() );

    self.fnt_hgt = fm.lineSpacing() + 2;
    self.fnt_wdt = fm.width( QChar('x') );
    self.fnt_maxwdt = fm.maxWidth();

    if ( fnt_hgt <= 0 || fnt_hgt > 100 )
    { 
	pErrMsg( "Font heigt no good. Taking 15." ); 
	self.fnt_hgt = 15;
    }
    if ( fnt_wdt <= 0 || fnt_wdt > 100 )
    { 
	pErrMsg( "Font width no good. Taking 10." ); 
	self.fnt_wdt = 10;
    }
    if ( fnt_maxwdt <= 0 || fnt_maxwdt > 100 )
    { 
	pErrMsg( "Font maxwidth no good. Taking 15." ); 
	self.fnt_maxwdt = 15;
    }
}
