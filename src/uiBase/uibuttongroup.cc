/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.cc,v 1.11 2005-11-18 16:21:30 cvsarend Exp $
________________________________________________________________________

-*/

#include "uibuttongroup.h"
#include "uiobjbody.h"

#ifdef USEQT4
# define mQButtonGroup	Q3ButtonGroup
# define mButton	QAbstractButton
# define mHorizontal	Qt::Horizontal
# define mVertical	Qt::Vertical
# include <q3buttongroup.h>
# include <qabstractbutton.h>
#else
# define mQButtonGroup	QButtonGroup
# define mButton	QButton
# define mHorizontal	Horizontal
# define mVertical	Vertical
# include <qbuttongroup.h>
# include <qbutton.h>
#endif

#include "errh.h"

class uiButtonGroupObjBody;
class uiButtonGroupParentBody;


class uiButtonGroupObjBody : public uiObjectBody  , public mQButtonGroup
{
public:
			uiButtonGroupObjBody(uiButtonGroupObj& handle, 
					uiParent* parnt, const char* txt, 
                                        bool vertical, int strips )
			: uiObjectBody( parnt, txt )
			, mQButtonGroup( strips, 

// Qt seems to have a different notion of "Horizontal" then I have....
					vertical ? mHorizontal: mVertical,

					txt,
					parnt && parnt->pbody() ?
					parnt->pbody()->managewidg() : 0, txt )
			, handle_( handle )	{}

    virtual		~uiButtonGroupObjBody()	{}

#define mHANDLE_OBJ	uiButtonGroupObj
#define mQWIDGET_BASE	QWidget
#define mQWIDGET_BODY	mQButtonGroup
#include		"i_uiobjqtbody.h"

public:

    void		setPrntBody (uiButtonGroupParentBody* pb)
			{ prntbody_ = pb; }

protected:

    uiButtonGroupParentBody*	prntbody_;

};


class uiButtonGroupParentBody : public uiParentBody
{ 	
    friend class	uiButtonGroupObjBody;

public:
			uiButtonGroupParentBody( uiButtonGroup& handle, 
					 uiButtonGroupObjBody& objbdy,
					 uiParent* parnt=0,
					 const char* nm="uiButtonGroupObjBody") 
                            : uiParentBody(nm)
                            , handle_(handle)
			    , objbody_(objbdy) {}

    virtual void	attachChild( constraintType tp,
				     uiObject* child,
				     uiObject* other, int margin,
				     bool reciprocal )
			{ pErrMsg("Cannot do attachments in uiButtonGroups"); }

protected:

    uiButtonGroup&		handle_;

    virtual const QWidget* 	qwidget_() const    
    				{ return objbody_.qwidget(); }
    virtual const QWidget* 	managewidg_() const 
    				{ return objbody_.qwidget();}

private:

    uiButtonGroupObjBody&	objbody_;

};


uiButtonGroupObj::uiButtonGroupObj( uiParent* p, const char* nm,
                                    bool vertical, int strips )
    : uiObject(p,nm)
{
    body_ = new uiButtonGroupObjBody( *this, p, nm, vertical, strips );
    setBody( body_ );
}


uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm,
			      bool vertical, int strips )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ = new uiButtonGroupObj( p, nm, vertical, strips );
    uiButtonGroupObjBody* grpbdy = 
	    dynamic_cast<uiButtonGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiButtonGroupParentBody( *this, *grpbdy, p, nm);
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );

    p->manageChld( *grpobj_, *grpobj_->body_ );
}


void uiButtonGroup::selectButton( int id )
{
    grpobj_->body_->setButton( id );
}


int uiButtonGroup::selectedId() const
{
    mButton* selbut = grpobj_->body_->selected();
    return grpobj_->body_->id( selbut );
}


int uiButtonGroup::nrButtons() const
{
    return grpobj_->body_->count();
}


void uiButtonGroup::setSensitive( int id, bool yn )
{
    mButton* but = grpobj_->body_->find( id );
    if ( but ) but->setEnabled( yn );
}


void uiButtonGroup::displayFrame( bool yn )
{ grpobj_->body_->setFlat( !yn ); }


bool uiButtonGroup::isFrameDisplayed() const
{ return grpobj_->body_->isFlat(); }


void uiButtonGroup::setRadioButtonExclusive( bool yn )
{ grpobj_->body_->setRadioButtonExclusive( yn ); }


bool uiButtonGroup::isRadioButtonExclusive() const
{ return grpobj_->body_->isRadioButtonExclusive(); }
