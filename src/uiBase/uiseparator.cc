/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:		$Id: uiseparator.cc,v 1.7 2007-04-11 16:32:52 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseparator.h"
#include "uiobjbody.h"

#ifdef USEQT3
# include <qframe.h>
#else
# include <QFrame>
#endif


class uiSeparatorBody : public uiObjBodyImpl<uiSeparator,QFrame>
{
public:

uiSeparatorBody( uiSeparator& handle, uiParent* p, const char* nm,
		 bool hor, bool raised)
    : uiObjBodyImpl<uiSeparator,QFrame>(handle,p,nm)
{
    setFrameStyle( (hor ? QFrame::HLine : QFrame::VLine)
		 | (raised ? QFrame::Raised : QFrame::Sunken) );
    setLineWidth( 1 ); setMidLineWidth( 0 );
}

};


uiSeparator::uiSeparator( uiParent* p, const char* txt, bool hor, bool raised )
    : uiObject(p,txt, mkbody(p,txt,hor,raised) )
{
}

uiSeparatorBody& uiSeparator::mkbody( uiParent* p, const char* txt, 
				      bool hor, bool raised )
{ 
    body_= new uiSeparatorBody(*this,p,txt,hor,raised);
    return *body_; 
}


void uiSeparator::setRaised( bool yn )
{ 
    body_->setFrameShadow( yn ? QFrame::Raised : QFrame::Sunken );
}
