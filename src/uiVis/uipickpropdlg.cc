/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.cc,v 1.4 2007-07-05 06:35:27 cvsraman Exp $
________________________________________________________________________

-*/

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set, bool drawline )
    : uiMarkerStyleDlg(p,"Pick properties"), set_(set)
{
    linefld_ = new uiCheckBox( this, "Draw line" ); 
    linefld_->setChecked( drawline );
    linefld_->attach( rightTo, typefld );
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    sliderfld->sldr()->setValue( set_.disp_.pixsize_ );
    colselfld->setColor( set_.disp_.color_ );
    typefld->setValue( set_.disp_.markertype_ + 1 );
}


void uiPickPropDlg::sliderMove( CallBacker* )
{
    const float sldrval = sliderfld->sldr()->getValue();
    set_.disp_.pixsize_ = mNINT(sldrval);
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    set_.disp_.markertype_ = typefld->getIntValue() - 1;
    if ( typefld->getIntValue()==0 ) linefld_->setChecked( true );
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* )
{
    set_.disp_.color_ = colselfld->color();
    Pick::Mgr().reportDispChange( this, set_ );
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    drawline_ = linefld_->isChecked();
    return true;
}
