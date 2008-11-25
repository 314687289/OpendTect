/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitoolbar.cc,v 1.49 2008-11-25 15:35:24 cvsbert Exp $";

#include "uitoolbar.h"

#include "uiaction.h"
#include "uibody.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uiobj.h"
#include "uiparentbody.h"

#include "bufstringset.h"
#include "filegen.h"
#include "pixmap.h"
#include "separstr.h"

#include <QToolBar>
#include "i_qtoolbut.h"


class uiToolBarBody : public uiParentBody
{
public:

			uiToolBarBody(uiToolBar&,QToolBar&);
			~uiToolBarBody();

    int 		addButton(const ioPixmap&,const CallBack&, 
				  const char*,bool);
    int 		addButton(const char*,const CallBack&,const char*,bool);
    int			addButton(const MenuItem&);
    void		addObject(uiObject*);
    void		clear();
    void		turnOn(int idx, bool yn );
    bool		isOn(int idx ) const;
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);
    bool		isSensitive() const;
    void		setToolTip(int,const char*);
    void		setShortcut(int,const char*);
    void		setPixmap(int,const char*);
    void		setPixmap(int,const ioPixmap&);
    void		setButtonMenu(int,const uiPopupMenu&);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

    void		reLoadPixMaps();

    const ObjectSet<uiObject>&	objectList() const	{ return objects_; }

protected:

    virtual const QWidget*      managewidg_() const	{ return qbar_; }
    virtual const QWidget*	qwidget_() const	{ return qbar_; }
    QToolBar*			qbar_;
    uiToolBar&			tbar_;
    int				iconsz_;

    virtual void		attachChild( constraintType tp,
					     uiObject* child,
					     uiObject* other, int margin,
					     bool reciprocal )
				{
				    pErrMsg(
				      "Cannot attach uiObjects in a uiToolBar");
				    return;
				}

private:
    BufferStringSet		pmsrcs_;
    
    ObjectSet<uiObject>		objects_; 
    TypeSet<int>		butindex_;

};


uiToolBarBody::uiToolBarBody( uiToolBar& handle, QToolBar& bar )
    : uiParentBody("ToolBar")
    , qbar_(&bar)
    , tbar_(handle)
{
}


uiToolBarBody::~uiToolBarBody()	
{ clear(); }


int uiToolBarBody::addButton( const char* fnm, const CallBack& cb,
			      const char* nm, bool toggle )
{ return addButton( ioPixmap(fnm), cb, nm, toggle ); }


int uiToolBarBody::addButton( const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
    uiToolButton* toolbarbut = new uiToolButton( &tbar_, nm, pm, cb );
    toolbarbut->setToggleButton(toggle);
    toolbarbut->setToolTip( nm );
    butindex_ += objects_.size();
    addObject( toolbarbut );
    pmsrcs_.add( pm.source() );

    return butindex_.size()-1;
}


int uiToolBarBody::addButton( const MenuItem& itm )
{
    uiAction* action = new uiAction( itm );
    qbar_->addAction( action->qaction() );
    return qbar_->actions().size()-1;
}


void uiToolBarBody::addObject( uiObject* obj )
{
    if ( obj && obj->body() )
    {
	qbar_->addWidget( obj->body()->qwidget() );
	objects_ += obj;
    }
}


void uiToolBarBody::clear()
{
    qbar_->clear();

    for ( int idx=0; idx<butindex_.size(); idx++ )
	delete objects_[butindex_[idx]];

    objects_.erase();
    butindex_.erase();
}

#define mToolBarBut(idx) dynamic_cast<uiToolButton*>(objects_[butindex_[idx]])
#define mConstToolBarBut(idx) \
	dynamic_cast<const uiToolButton*>(objects_[butindex_[idx]])

void uiToolBarBody::turnOn( int idx, bool yn )
{ mToolBarBut(idx)->setOn( yn ); }


void uiToolBarBody::reLoadPixMaps()
{
    for ( int idx=0; idx<pmsrcs_.size(); idx++ )
    {
	const BufferString& pmsrc = pmsrcs_.get( idx );
	if ( pmsrc[0] == '[' )
	    continue;

	FileMultiString fms( pmsrc );
	const int len = fms.size();
	const BufferString fnm( fms[0] );
	const ioPixmap pm( fnm.buf(), len > 1 ? fms[1] : 0 );
	if ( pm.isEmpty() )
	    continue;

	mToolBarBut(idx)->setPixmap( pm );
    }
}


bool uiToolBarBody::isOn( int idx ) const
{ return mConstToolBarBut(idx)->isOn(); }

void uiToolBarBody::setSensitive( int idx, bool yn )
{ mToolBarBut(idx)->setSensitive( yn ); }

void uiToolBarBody::setPixmap( int idx, const char* fnm )
{ setPixmap( idx, ioPixmap(fnm) ); }

void uiToolBarBody::setPixmap( int idx, const ioPixmap& pm )
{ mToolBarBut(idx)->setPixmap( pm ); }

void uiToolBarBody::setToolTip( int idx, const char* txt )
{ mToolBarBut(idx)->setToolTip( txt ); }

void uiToolBarBody::setShortcut( int idx, const char* sc )
{ mToolBarBut(idx)->setShortcut( sc ); }

void uiToolBarBody::setSensitive( bool yn )
{ if ( qwidget() ) qwidget()->setEnabled( yn ); }

bool uiToolBarBody::isSensitive() const
{ return qwidget() ? qwidget()->isEnabled() : false; }

void uiToolBarBody::setButtonMenu( int idx, const uiPopupMenu& mnu )
{ mToolBarBut(idx)->setMenu( mnu ); }


ObjectSet<uiToolBar>& uiToolBar::toolBars()
{
    static ObjectSet<uiToolBar>* ret = 0;
    if ( !ret )
    ret = new ObjectSet<uiToolBar>;
    return *ret;
}



uiToolBar::uiToolBar( uiParent* parnt, const char* nm, ToolBarArea tba,
		      bool newline )
    : uiParent(nm,0)
    , parent_(parnt)
    , tbarea_(tba)
{
    qtoolbar_ = new QToolBar( QString(nm) );
    qtoolbar_->setObjectName( nm );
    setBody( &mkbody(nm,*qtoolbar_) );

    mDynamicCastGet(uiMainWin*,uimw,parnt)
    if ( uimw )
    {
	if ( newline ) uimw->addToolBarBreak();
	uimw->addToolBar( this );
    }

    toolBars() += this;
}


uiToolBar::~uiToolBar()
{
    mDynamicCastGet(uiMainWin*,uimw,parent_)
    if ( uimw ) uimw->removeToolBar( this );

    delete body_;
    delete qtoolbar_;

    toolBars() -= this;
}


uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_ = new uiToolBarBody( *this, qtb );
    return *body_; 
}


int uiToolBar::addButton( const char* fnm, const CallBack& cb,
			  const char* nm, bool toggle )
{ return body_->addButton( fnm, cb, nm, toggle ); }


int uiToolBar::addButton( const ioPixmap& pm, const CallBack& cb,
			  const char* nm, bool toggle )
{ return body_->addButton( pm, cb, nm, toggle ); }


int uiToolBar::addButton( const MenuItem& itm )
{ return body_->addButton( itm ); }


void uiToolBar::addObject( uiObject* obj )
{ body_->addObject( obj ); };


void uiToolBar::setLabel( const char* lbl )
{
    qtoolbar_->setLabel( QString(lbl) );
    setName( lbl );
}


void uiToolBar::turnOn( int idx, bool yn )
{ body_->turnOn( idx, yn ); }

bool uiToolBar::isOn( int idx ) const
{ return body_->isOn( idx ); }

void uiToolBar::setSensitive( int idx, bool yn )
{ body_->setSensitive( idx, yn ); }

void uiToolBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }

void uiToolBar::setToolTip( int idx, const char* tip )
{ body_->setToolTip( idx, tip ); }

void uiToolBar::setShortcut( int idx, const char* sc )
{ body_->setShortcut( idx, sc ); }

void uiToolBar::setPixmap( int idx, const char* fnm )
{ body_->setPixmap( idx, fnm ); }

void uiToolBar::setPixmap( int idx, const ioPixmap& pm )
{ body_->setPixmap( idx, pm ); }

void uiToolBar::setButtonMenu( int idx, const uiPopupMenu& mnu )
{ body_->setButtonMenu( idx, mnu ); }


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar_->show();
    else
	qtoolbar_->hide();

    mDynamicCastGet(uiMainWin*,uimw,parent_)
    if ( uimw ) uimw->updateToolbarsMenu();
}


bool uiToolBar::isHidden() const
{ return qtoolbar_->isHidden(); }

bool uiToolBar::isVisible() const
{ return qtoolbar_->isVisible(); }

void uiToolBar::addSeparator()
{ qtoolbar_->addSeparator(); }

void uiToolBar::reLoadPixMaps()
{ body_->reLoadPixMaps(); }

void uiToolBar::clear()
{ body_->clear(); }


uiMainWin* uiToolBar::mainwin()
{ 
    mDynamicCastGet(uiMainWin*,uimw,parent_)
    return uimw;
}


const ObjectSet<uiObject>& uiToolBar::objectList() const
{ return body_->objectList(); }
