/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uihellopi.cc,v 1.10 2007-02-05 18:19:47 cvsbert Exp $";

#include "uimsg.h"

#ifdef PLAN_A

extern "C" const char* InituiHelloPlugin( int, char** )
{
    uiMSG().message( "Hello world" );
    return 0; // All OK - no error messages
}


#else /* PLAN_A is not defined */


#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "plugins.h"

extern "C" int GetuiHelloPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiHelloPluginInfo()
{
    static PluginInfo retpi = {
	"uiHello plugin - plan B",
	"Bert",
	"1.1.1",
	"This is the more extensive variant of the uiHello example.\n"
   	"See the plugin manual for details." };
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiHelloMgr :  public CallBacker
{
public:

			uiHelloMgr(uiODMain&);

    uiODMain&		appl;
    void		dispMsg(CallBacker*);
};


uiHelloMgr::uiHelloMgr( uiODMain& a )
	: appl(a)
{
    uiMenuItem* newitem = new uiMenuItem( "&Diplay Hello Message ...",
	    				  mCB(this,uiHelloMgr,dispMsg) );
    appl.menuMgr().utilMnu()->insertItem( newitem );
}


class uiHelloMsgBringer : public uiDialog
{
public:

uiHelloMsgBringer( uiParent* p )
    : uiDialog(p,Setup("Hello Message Window","Specify hello message",0))
{
    txtfld = new uiGenInput( this, "Hello message",
	    			StringInpSpec("Hello world") );
    typfld = new uiGenInput( this, "Message type",
	    			BoolInpSpec(true,"Info","Warning") );
    typfld->attach( alignedBelow, txtfld );
}

bool acceptOK( CallBacker* )
{
    const char* typedtxt = txtfld->text();
    if ( ! *typedtxt )
    {
	uiMSG().error( "Please type a message text" );
	return false;
    }
    if ( typfld->getBoolValue() )
	uiMSG().message( typedtxt );
    else
	uiMSG().warning( typedtxt );
    return true;
}

    uiGenInput*	txtfld;
    uiGenInput*	typfld;

};


void uiHelloMgr::dispMsg( CallBacker* )
{
    uiHelloMsgBringer dlg( &appl );
    dlg.go();
}


extern "C" const char* InituiHelloPlugin( int, char** )
{
    (void)new uiHelloMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}


#endif /* ifdef PLAN_A */
