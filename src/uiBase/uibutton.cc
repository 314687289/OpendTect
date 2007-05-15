/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.cc,v 1.40 2007-05-15 10:28:57 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uibutton.h"
#include "i_qbutton.h"
#include "uiobjbody.h"
#include "pixmap.h"
#include "settings.h"


#include <qapplication.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>

#ifdef USEQT3
# define mTxt  ,txt
# define mQIcon QIconSet
#else
# define mTxt 
# define mQIcon QIcon
#endif 

static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User + 0);


//! Wrapper around QButtons. 
/*!
    Extends each QButton class <T> with a i_ButMessenger, which connects
    itself to the signals transmitted from Qt buttons.  Each signal is
    relayed to the notifyHandler of a uiButton handle object.
*/
template< class T > class uiButtonTemplBody : public uiButtonBody,
    public uiObjectBody, public T { public:

			uiButtonTemplBody( uiButton& handle, uiParent* parnt,
					   const char* txt )
			    : uiObjectBody( parnt, txt )
                            , T( parnt && parnt->pbody() ?
				      parnt->pbody()->managewidg() : 0 mTxt )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				this->setText(txt); 
				setHSzPol( uiObject::SmallVar );
			    }

			uiButtonTemplBody(uiButton& handle, 
				     const ioPixmap& pm,
				     uiParent* parnt, const char* txt)
			    : uiObjectBody( parnt, txt )
			    , T( mQIcon(*pm.qpixmap()),txt, 
					parnt && parnt->pbody() ?
					parnt->pbody()->managewidg() : 0 mTxt )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				this->setText(txt); 
				setHSzPol( uiObject::SmallVar );
			    }

#define mHANDLE_OBJ	uiButton
#include                "i_uiobjqtbody.h"

public:

    virtual		~uiButtonTemplBody()		{ delete &messenger_; }

    virtual mButton&    qButton() = 0;
    inline const mButton& qButton() const
                        { return ((uiButtonTemplBody*)this)->qButton(); }

    virtual int 	nrTxtLines() const		{ return 1; }

    const char*		text();

    void 		activate()
			{
			    QEvent* actevent = new QEvent( sQEventActivate );
			    QApplication::postEvent( &messenger_, actevent ); 
			}

protected:

    i_ButMessenger&     messenger_;
    int                 idInGroup;

    void		doNotify()	{ handle_.activated.trigger(handle_); }

    bool 		handleEvent( const QEvent* ev )
			{ 
			    if ( ev->type() != sQEventActivate ) return false;
			    handle_.click(); 
			    handle_.activatedone.trigger(handle_);
			    return true; 
			}

};

class uiPushButtonBody : public uiButtonTemplBody<QPushButton>
{
public:
			uiPushButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QPushButton>(handle,parnt,txt)
			    {}

			uiPushButtonBody( uiButton& handle, const ioPixmap& pm,
				          uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QPushButton>
					(handle,pm,parnt,txt)		{}

    virtual mButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


class uiRadioButtonBody : public uiButtonTemplBody<QRadioButton>
{                        
public:
			uiRadioButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QRadioButton>(handle,parnt,txt)
			    {}

    virtual mButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


class uiCheckBoxBody: public uiButtonTemplBody<QCheckBox>
{
public:

			uiCheckBoxBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QCheckBox>(handle,parnt,txt)
			    {}

    virtual mButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::toggled ) doNotify(); }
};


class uiToolButtonBody : public uiButtonTemplBody<QToolButton>
{
public:
			uiToolButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QToolButton>(handle,parnt,txt)
			    {}

    virtual mButton&    qButton()		{ return *this; }


protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


#define mqbut()         dynamic_cast<mButton*>( body() )

uiButton::uiButton( uiParent* parnt, const char* nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject( parnt, nm, b )
    , activated( this )
    , activatedone( this )
{
    if ( cb ) activated.notify(*cb);
}


void uiButton::setText( const char* txt )
{ 
    mqbut()->setText( QString( txt ) ); 
}


const char* uiButton::text()
#ifdef USEQT3
    { return mqbut()->text(); }
#else
    { return mqbut()->text().toAscii().constData(); }
#endif


void uiButton::activate()
{
    dynamic_cast<uiButtonBody*>( body() )->activate();
}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm,ia) )
    , pixmap_(0)
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm, const CallBack& cb,
			    bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm,ia) )
    , pixmap_(0)
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm,ia) )
    , pixmap_(new ioPixmap(pm))
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm,ia) )
    , pixmap_(new ioPixmap(pm))
{}


uiPushButton::~uiPushButton()
{
    delete pixmap_;
}


uiPushButtonBody& uiPushButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt, bool immact )
{
    BufferString buttxt( txt );
    if ( !immact && txt && *txt )
	buttxt += " ...";
    if ( pm )	body_ = new uiPushButtonBody(*this,*pm,parnt,buttxt.buf()); 
    else	body_ = new uiPushButtonBody(*this,parnt,buttxt.buf()); 

    return *body_; 
}


void uiPushButton::setDefault( bool yn )
{
    body_->setDefault( yn );
    setFocus();
}


void uiPushButton::setPixmap( const ioPixmap& pm )
{
    delete pixmap_;
    pixmap_ = new ioPixmap( pm );
    body_->setPixmap( *pm.qpixmap() );
}


void uiPushButton::click()			{ activated.trigger(); }


uiRadioButton::uiRadioButton( uiParent* p, const char* nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{}


uiRadioButton::uiRadioButton( uiParent* p, const char* nm, 
			      const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{}


uiRadioButtonBody& uiRadioButton::mkbody( uiParent* parnt, const char* txt )
{ 
    body_= new uiRadioButtonBody(*this,parnt,txt);
    return *body_; 
}


bool uiRadioButton::isChecked() const		{ return body_->isChecked (); }

void uiRadioButton::setChecked( bool check )	{ body_->setChecked( check ); }

void uiRadioButton::click()			{ setChecked( true ); }


uiCheckBox::uiCheckBox( uiParent* p, const char* nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{}


uiCheckBox::uiCheckBox( uiParent* p, const char* nm, const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{}


void uiCheckBox::setText( const char* txt )
{ 
    mqbut()->setText( QString( txt ) ); 
}


uiCheckBoxBody& uiCheckBox::mkbody( uiParent* parnt, const char* txt )
{ 
    body_= new uiCheckBoxBody(*this,parnt,txt);
    return *body_; 
}

bool uiCheckBox::isChecked () const		{ return body_->isChecked(); }

void uiCheckBox::setChecked ( bool check )	{ body_->setChecked( check ); }

void uiCheckBox::click()			{ setChecked( !isChecked() ); }


static int preftbsz = -1;
#define mSetDefPrefSzs() \
    if ( preftbsz < 0 ) \
    { \
	Settings::common().get( "dTect.Icons.size", preftbsz ); \
	if ( preftbsz < 10 || preftbsz > 64 ) \
	    preftbsz = 28; \
    } \
    body_->setIconSize( QSize(preftbsz,preftbsz) );

uiToolButton::uiToolButton( uiParent* parnt, const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm) )
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm, const CallBack& cb)
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm) )
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm) )
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm) )
{
    mSetDefPrefSzs();
}


uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt)
{
    body_ = new uiToolButtonBody(*this,parnt,txt); 
    if ( pm )
        body_->setIconSet( QIconSet(*pm->qpixmap()) );

    return *body_;
}

#ifdef USEQT3
# define mIsOn			isOn
# define mSetOn			setOn
# define mIsToggleButton	isToggleButton
# define mSetToggleButton	setToggleButton
#else
# define mIsOn			isChecked
# define mSetOn			setChecked
# define mIsToggleButton	isCheckable
# define mSetToggleButton	setCheckable
#endif


bool uiToolButton::isOn()		{ return body_->mIsOn(); }
void uiToolButton::setOn( bool yn)	{ body_->mSetOn(yn); }

bool uiToolButton::isToggleButton()     { return body_->mIsToggleButton(); }
void uiToolButton::setToggleButton( bool yn) { body_->mSetToggleButton(yn); }


void uiToolButton::click()
{
    if ( isToggleButton() )
	setOn( !isOn() );
    else
	activated.trigger();
}


void uiToolButton::setPixmap( const ioPixmap& pm )
{
#ifdef USEQT3
    body_->setIconSet( QIconSet(*pm.qpixmap()) );
#else
    body_->setIcon( QIcon(*pm.qpixmap()) );
#endif
}
