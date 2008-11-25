/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimdiarea.cc,v 1.3 2008-11-25 15:35:24 cvsbert Exp $";

#include "uimdiarea.h"
#include "i_qmdiarea.h"

#include "uiobjbody.h"
#include "bufstringset.h"

#include <QIcon>
#include <QMdiSubWindow>



class uiMdiAreaBody : public uiObjBodyImplNoQtNm<uiMdiArea,QMdiArea>
{ 	
public:
    			uiMdiAreaBody(uiMdiArea&,uiParent*,const char*);

protected:
    i_MdiAreaMessenger& messenger_;
};


uiMdiAreaBody::uiMdiAreaBody( uiMdiArea& handle, uiParent* p, const char* nm )
    : uiObjBodyImplNoQtNm<uiMdiArea,QMdiArea>(handle,p,nm)
    , messenger_(*new i_MdiAreaMessenger(this,&handle))
{}



uiMdiArea::uiMdiArea( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , windowActivated(this)
{
    setStretch( 2, 2 );
}


uiMdiAreaBody& uiMdiArea::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiMdiAreaBody( *this, p, nm );
    return *body_;
}


void uiMdiArea::tile()		{ body_->tileSubWindows(); }
void uiMdiArea::cascade()	{ body_->cascadeSubWindows(); }


void uiMdiArea::tileVertical()
{
    tile();
    QList<QMdiSubWindow*> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
	if ( widget && !widget->isHidden() )
	    nrvisiblewindows++;
    }
    
    if ( nrvisiblewindows == 0 ) return;

    const int wswidth = body_->frameSize().width();
    const int wsheight = body_->frameSize().height();
    const int avgheight = wsheight / nrvisiblewindows;
    int y = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
	if ( widget->isHidden() ) continue;

	if ( widget->isMaximized() )
	{
	    widget->hide();
	    widget->showNormal();
	}

	const int prefheight = widget->minimumHeight() + 
	    			widget->parentWidget()->baseSize().height();
	const int actheight = QMAX( avgheight, prefheight );

	widget->setGeometry( 0, y, wswidth, actheight );
	y += actheight;
    }
}


void uiMdiArea::tileHorizontal()
{
    tile();
    QList<QMdiSubWindow*> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
	if ( widget && !widget->isHidden() )
	    nrvisiblewindows++;
    }
    
    if ( nrvisiblewindows == 0 ) return;

    const int wswidth = body_->frameSize().width();
    const int wsheight = body_->frameSize().height();
    const int avgwidth = wswidth / nrvisiblewindows;
    int x = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
	if ( widget->isHidden() ) continue;

	if ( widget->isMaximized() )
	{
	    widget->hide();
	    widget->showNormal();
	}

	const int prefwidth = widget->minimumWidth() + 
	    			widget->parentWidget()->baseSize().width();
	const int actwidth = QMAX( avgwidth, prefwidth );

	widget->setGeometry( x, 0, actwidth, wsheight );
	x += actwidth;
    }
}


void uiMdiArea::addGroup( uiMdiAreaGroup* grp )
{
    if ( !grp || !grp->pbody() ) return;

    body_->addSubWindow( grp->qWidget() );
    grp->closed().notify( mCB(this,uiMdiArea,grpClosed) );
    grp->changed.notify( mCB(this,uiMdiArea,grpChanged) );
    grps_ += grp;
    body_->setActiveSubWindow( grp->qWidget() );
    windowActivated.trigger();
}


void uiMdiArea::grpClosed( CallBacker* cb )
{
    mDynamicCastGet(uiObject*,uiobj,cb)
    if ( !uiobj ) return;

    uiMdiAreaGroup* grp = 0;
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( grps_[idx]->mainObject() != uiobj )
	    continue;

	grp = grps_[idx];
	break;
    }

    grp->closed().remove( mCB(this,uiMdiArea,grpClosed) );
    grp->changed.remove( mCB(this,uiMdiArea,grpChanged) );
    grps_ -= grp;
    windowActivated.trigger();
}


void uiMdiArea::grpChanged( CallBacker* )
{ windowActivated.trigger(); }


void uiMdiArea::closeAll()
{
    body_->closeAllSubWindows();
    windowActivated.trigger();
}


void uiMdiArea::setActiveWin( uiMdiAreaGroup* grp )
{
    if ( !grp ) return;
    body_->setActiveSubWindow( grp->qWidget() );
}


void uiMdiArea::setActiveWin( const char* nm )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( strcmp(nm,grps_[idx]->getTitle()) )
	    continue;

	body_->setActiveSubWindow( grps_[idx]->qWidget() );
	return;
    }
}


const char* uiMdiArea::getActiveWin() const
{
    static BufferString nm;
    QWidget* widget = body_->activeSubWindow();
    nm = widget ? widget->caption().toAscii().constData() : "";
    return nm.buf();
}


void uiMdiArea::getWindowNames( BufferStringSet& nms )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	nms.add( grps_[idx]->getTitle() );
}



uiMdiAreaGroup::uiMdiAreaGroup( const char* nm )
    : uiGroup(0,nm)
    , changed(this)
{
    qmdisubwindow_ = new QMdiSubWindow();
    qmdisubwindow_->setWidget( attachObj()->body()->qwidget() );
    setTitle( nm );
}


void uiMdiAreaGroup::setTitle( const char* nm )
{
    qmdisubwindow_->setCaption( nm );
    changed.trigger();
}


const char* uiMdiAreaGroup::getTitle() const
{
    static BufferString title;
    title = qmdisubwindow_->caption().toAscii().constData();
    return title.buf();
}


void uiMdiAreaGroup::setIcon( const char* img[] )
{
    if ( !img ) return;
    qmdisubwindow_->setWindowIcon( QIcon(img) );
}


NotifierAccess& uiMdiAreaGroup::closed()
{ return mainObject()->closed; }

QMdiSubWindow* uiMdiAreaGroup::qWidget()
{ return qmdisubwindow_; }
