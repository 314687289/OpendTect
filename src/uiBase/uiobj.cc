/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiparent.h"
#include "uicursor.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uigroup.h"
#include "uitreeview.h"

#include "color.h"
#include "settingsaccess.h"
#include "timer.h"
#include "perthreadrepos.h"

#include <QEvent>
#include <QObject>

mUseQtnamespace


mDefineEnumUtils(uiRect,Side,"Side") { "Left", "Right", "Top", "Bottom", 0 };


class uiObjEventFilter : public QObject
{
public:
			uiObjEventFilter( uiObject& uiobj )
			    : uiobject_( uiobj )
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);
    uiObject&		uiobject_;
};


bool uiObjEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    if ( ev && ev->type() == mUsrEvLongTabletPress )
    {
	uiobject_.handleLongTabletPress();
	return true;
    }

    return false;
}


static ObjectSet<uiObject> uiobjectlist_;


static BufferString getCleanName( const char* nm )
{
    QString qstr( nm );
    qstr.remove( QChar('&') );
    return BufferString( qstr );
}

static int iconsz_ = -1;
static int fldsz_ = -1;

uiObject::uiObject( uiParent* p, const char* nm )
    : uiBaseObject( getCleanName(nm), 0 )
    , setGeometry(this)
    , closed(this)
    , parent_( p )
    , singlewidget_( 0 )
{
    if ( p ) p->addChild( *this );
    uiobjectlist_ += this;
    updateToolTip();

    uiobjeventfilter_ = new uiObjEventFilter( *this );
    if ( body() && body()->qwidget() )
	body()->qwidget()->installEventFilter( uiobjeventfilter_ );
}


uiObject::~uiObject()
{
    if ( singlewidget_ )
	singlewidget_->removeEventFilter( uiobjeventfilter_ );

    delete uiobjeventfilter_;

    closed.trigger();
    uiobjectlist_ -= this;
}


void uiObject::setSingleWidget( QWidget* w )
{
    if ( singlewidget_ )
	singlewidget_->removeEventFilter( uiobjeventfilter_ );

    singlewidget_ = w;

    if ( singlewidget_ )
	singlewidget_->installEventFilter( uiobjeventfilter_ );
}


void uiObject::setHSzPol( SzPolicy p )
    { mBody()->setHSzPol(p); }

void uiObject::setVSzPol( SzPolicy p )
    { mBody()->setVSzPol(p); }

uiObject::SzPolicy uiObject::szPol(bool hor) const
    { return mConstBody()->szPol(hor); }


void uiObject::setName( const char* nm )
{
    uiBaseObject::setName( getCleanName(nm) );
    updateToolTip();
}


const uiString& uiObject::toolTip() const
{
    return tooltip_;
}


void uiObject::setToolTip( const uiString& txt )
{
    tooltip_ = txt;
    updateToolTip();
}


void uiObject::updateToolTip(CallBacker*)
{
    mEnsureExecutedInMainThread( uiObject::updateToolTip );
    if ( !getWidget(0) ) return;

    if ( uiMain::isNameToolTipUsed() && !name().isEmpty() )
    {
	BufferString namestr = name().buf();
	uiMain::formatNameToolTipString( namestr );
	getWidget(0)->setToolTip( namestr.buf() );
    }
    else
	getWidget(0)->setToolTip( tooltip_.getQString() );
}


void uiObject::translateText()
{
    uiBaseObject::translateText();
    updateToolTip();
}


void uiObject::display( bool yn, bool shrink, bool maximize )
{
    finalise();
    mBody()->display(yn,shrink,maximize);
}

void uiObject::setFocus()
{ mBody()->uisetFocus(); }

bool uiObject::hasFocus() const
{ return mConstBody()->uihasFocus(); }

void uiObject::disabFocus()
{
    if ( getWidget(0) )
	getWidget(0)->setFocusPolicy( Qt::NoFocus );
}


void uiObject::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    body()->qwidget()->setCursor( qcursor );
}


bool uiObject::isCursorInside() const
{
    const uiPoint cursorpos = uiCursorManager::cursorPos();
    const QPoint objpos = mConstBody()->qwidget()->mapToGlobal( QPoint(0,0) );
    return cursorpos.x>=objpos.x() && cursorpos.x<objpos.x()+width() &&
	   cursorpos.y>=objpos.y() && cursorpos.y<objpos.y()+height();
}


void uiObject::setStyleSheet( const char* qss )
{
    body()->qwidget()->setStyleSheet( qss );
}


Color uiObject::backgroundColor() const
    { return mConstBody()->uibackgroundColor(); }


void uiObject::setBackgroundColor(const Color& col)
    { mBody()->uisetBackgroundColor(col); }


void uiObject::setBackgroundPixmap( const uiPixmap& pm )
    { mBody()->uisetBackgroundPixmap( pm ); }


void uiObject::setTextColor(const Color& col)
    { mBody()->uisetTextColor(col); }


void uiObject::setSensitive(bool yn)
    { mBody()->uisetSensitive(yn); }

bool uiObject::sensitive() const
    { return mConstBody()->uisensitive(); }


bool uiObject::visible() const
    { return mConstBody()->uivisible(); }

bool uiObject::isDisplayed() const
    { return mConstBody()->isDisplayed(); }

int uiObject::prefHNrPics() const
    { return mConstBody()->prefHNrPics(); }


void uiObject::setPrefWidth( int w )
    { mBody()->setPrefWidth(w); }


void uiObject::setPrefWidthInChar( int w )
    { mBody()->setPrefWidthInChar( (float)w ); }

void uiObject::setPrefWidthInChar( float w )
     { mBody()->setPrefWidthInChar(w); }

void uiObject::setMinimumWidth( int w )
    { mBody()->setMinimumWidth(w); }

void uiObject::setMinimumHeight( int h )
    { mBody()->setMinimumHeight(h); }

void uiObject::setMaximumWidth( int w )
    { mBody()->setMaximumWidth(w); }

void uiObject::setMaximumHeight( int h )
    { mBody()->setMaximumHeight(h); }

int uiObject::prefVNrPics() const
    { return mConstBody()->prefVNrPics(); }

void uiObject::setPrefHeight( int h )
    { mBody()->setPrefHeight(h); }

void uiObject::setPrefHeightInChar( int h )
    { mBody()->setPrefHeightInChar( (float)h ); }

void uiObject::setPrefHeightInChar( float h )
     {mBody()->setPrefHeightInChar(h);}

void uiObject::setStretch( int hor, int ver )
     {mBody()->setStretch(hor,ver); }

void uiObject::attach ( constraintType tp, int margin )
    { mBody()->attach(tp, (uiObject*)0, margin); }

void uiObject::attach ( constraintType tp, uiObject* other, int margin,
			bool reciprocal )
    { mBody()->attach(tp, other, margin, reciprocal); }


/*!
    Moves the \a second widget around the ring of focus widgets so
    that keyboard focus moves from the \a first widget to the \a
    second widget when the Tab key is pressed.

    Note that since the tab order of the \a second widget is changed,
    you should order a chain like this:

    \code
	setTabOrder( a, b ); // a to b
	setTabOrder( b, c ); // a to b to c
	setTabOrder( c, d ); // a to b to c to d
    \endcode

    \e not like this:

    \code
	setTabOrder( c, d ); // c to d   WRONG
	setTabOrder( a, b ); // a to b AND c to d
	setTabOrder( b, c ); // a to b to c, but not c to d
    \endcode

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.
*/
void uiObject::setTabOrder( uiObject* first, uiObject* second )
{
    QWidget::setTabOrder( first->body()->qwidget(), second->body()->qwidget() );
}


void uiObject::setFont( const uiFont& f )
    { mBody()->uisetFont(f); }

const uiFont* uiObject::font() const
    { return mConstBody()->uifont(); }

uiSize uiObject::actualsize( bool include_border ) const
    { return mConstBody()->actualsize( include_border ); }

void uiObject::setCaption( const uiString& c )
    { mBody()->uisetCaption(c); }

void uiObject::reDraw( bool deep )
    { mBody()->reDraw( deep ); }


uiMainWin* uiObject::mainwin()
{
    uiParent* par = parent();
    if ( !par )
    {
	mDynamicCastGet(uiMainWin*,mw,this)
	return mw;
    }

    return par->mainwin();
}


QWidget* uiObject::getWidget( int idx )
{ return !idx && body() ? body()->qwidget() : 0 ; }


void uiObject::close()
{
    if ( body() && body()->qwidget() )
	body()->qwidget()->close();
}


int uiObject::width() const
{
    return getConstWidget(0) ? getConstWidget(0)->width() : 1;
}


int uiObject::height() const
{
    return getConstWidget(0) ? getConstWidget(0)->height() : 1;
}


int uiObject::iconSize()
{
    if ( iconsz_ < 0 )
    {
	const BufferString key =
	    IOPar::compKey( SettingsAccess::sKeyIcons(), "size" );
	iconsz_ = 32;
	Settings::common().get( key, iconsz_ );
    }

    return iconsz_;
}


int uiObject::baseFldSize()
{
    if ( fldsz_ < 0 )
    {
	fldsz_ = 10;
	Settings::common().get( "dTect.Field.size", fldsz_ );
    }

    return fldsz_;
}


void uiObject::updateToolTips()
{
    for ( int idx=uiobjectlist_.size()-1; idx>=0; idx-- )
	uiobjectlist_[idx]->updateToolTip();
}


void uiObject::reParent( uiParent* p )
{
    if ( !p ) return;

    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
	getWidget(idx)->setParent( p->getParentWidget() );
    }
}


bool uiObject::handleLongTabletPress()
{
    if ( !parent() || !parent()->getMainObject() || parent()->getMainObject()==this )
	return false;

    return parent()->getMainObject()->handleLongTabletPress();
}
