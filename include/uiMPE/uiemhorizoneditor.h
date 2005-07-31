#ifndef uiemhorizoneditor_h
#define uiehorizonmeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemhorizoneditor.h,v 1.3 2005-07-31 06:17:44 cvskris Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uiemobjeditor.h"


class uiMPEPartServer;

namespace EM
{
    class	EdgeLineSet;
    class	EdgeLineSegment;
};


namespace MPE
{
class HorizonEditor;

class uiEMHorizonEditor : public uiEMObjectEditor
{

public:
    static void		initClass();
    static uiEMEditor*	create(uiParent*, MPE::ObjectEditor* );

    			uiEMHorizonEditor( uiParent*, MPE::HorizonEditor* );

    void		createNodeMenus(CallBacker*);
    void		handleNodeMenus(CallBacker*);
    void		createInteractionLineMenus(CallBacker*);
    void		handleInteractionLineMenus(CallBacker*);

    MPE::HorizonEditor*	getEditor();

protected:
    bool		canMakeStopLine( const EM::EdgeLineSet& lineset,
				     const EM::EdgeLineSegment& interactionline,
				     int& linenr, bool& forward ) const;

    MenuItem		editsettingsmnuitem;

    MenuItem		splitsectionmnuitem;
    MenuItem		makestoplinemnuitem;
};

};


#endif
