/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Oct 2004
 RCS:		$Id: ui2dsip.cc,v 1.1 2004-10-06 16:18:41 bert Exp $
________________________________________________________________________

-*/

#include "ui2dsip.h"
#include "uidialog.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "cubesampling.h"
#include "errh.h"


class ui2DDefSurvInfoDlg : public uiDialog
{
public:

ui2DDefSurvInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey setup for 2D only",
				 "Specify survey characteristics","0.3.8"))
{
    trcdistfld = new uiGenInput( this, "Average trace distance",
	    			 FloatInpSpec() );
    DoubleInpSpec dis;
    xrgfld = new uiGenInput( this, "X-coordinate range", dis, dis );
    xrgfld->attach( alignedBelow, trcdistfld );
    yrgfld = new uiGenInput( this, "Y-coordinate range", dis, dis );
    yrgfld->attach( alignedBelow, xrgfld );
}

    uiGenInput*		trcdistfld;
    uiGenInput*		xrgfld;
    uiGenInput*		yrgfld;

};


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p )
{
    return new ui2DDefSurvInfoDlg( p );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool ui2DSurvInfoProvider::getInfo( uiDialog* din, CubeSampling& cs,
				      Coord crd[3] )
{
    if ( !din ) return false;
    mDynamicCastGet(ui2DDefSurvInfoDlg*,dlg,din)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }

    double tdist = dlg->trcdistfld->getValue();
    Coord c0( dlg->xrgfld->getValue(0), dlg->yrgfld->getValue(0) );
    Coord c1( dlg->xrgfld->getValue(1), dlg->yrgfld->getValue(1) );
    if ( tdist < 0 ) tdist = -tdist;
    if ( c0.x > c1.x ) Swap( c0.x, c1.x );
    if ( c0.y > c1.y ) Swap( c0.y, c1.y );
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    if ( tdist < 0.1 )
	mErrRet("Trace distance should be > 0.1")
    const int nrinl = (int)(d.x / tdist + 1.5);
    const int nrcrl = (int)(d.y / tdist + 1.5);
    if ( nrinl < 2 )
	mErrRet("X distance is less than one trace distance")
    if ( nrcrl < 2 )
	mErrRet("Y distance is less than one trace distance")

    cs.hrg.start.inl = cs.hrg.start.crl = 1;
    cs.hrg.step.inl = cs.hrg.step.crl = 1;
    cs.hrg.stop.inl = nrinl; cs.hrg.stop.crl = nrcrl;

    const Coord cmax( c0.x + tdist*(nrinl-1), c0.y + tdist*(nrcrl-1) );
    crd[0] = c0;
    crd[1] = cmax;
    crd[2] = Coord( c0.x, cmax.y );

    cs.zrg.start = cs.zrg.stop = cs.zrg.step = mUndefValue;
    return true;
}
