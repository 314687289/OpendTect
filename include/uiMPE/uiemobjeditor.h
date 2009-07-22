#ifndef uiemobjeditor_h
#define uiehobjeditor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemobjeditor.h,v 1.3 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uimpe.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

mClass uiEMObjectEditor : public uiEMEditor
{

public:
    			uiEMObjectEditor( uiParent*, MPE::ObjectEditor* );

    virtual void	createNodeMenus(CallBacker*);
    virtual void	handleNodeMenus(CallBacker*);

protected:
    MenuItem		snapmenuitem;

    MPE::ObjectEditor*	editor;
};

};


#endif
