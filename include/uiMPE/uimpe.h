#ifndef uimpe_h
#define uimpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uimpe.h,v 1.6 2006-05-12 09:51:14 cvsnanne Exp $
________________________________________________________________________

-*/

/*!\mainpage
uiMPE contains user interface for the MPE-engines trackers and editors. The
class uiMPEPartServer provides tracking services for higher hierarchies 
(i.e. the application manager).

There are two factories avaliable for Trackning and Editing UI from
the MPE::uiMPEEngine object, which is available through the static function
MPE::uiMPE(). */


#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "uigroup.h"

class uiParent;

namespace Attrib { class DescSet; };


namespace MPE
{

class ObjectEditor;
class SectionTracker;

/*! Interface for the ui interaction with MPE::ObjectEditor. Object is
    implemented in separate classes inheriting uiEMEditor that can be created
    by:

\code
    PtrMan<uiEMEditor> uieditor =
    	MPE::uiMPE().editorfact.create( uiparent, editor );
\endcode
*/

class uiEMEditor : public CallBackClass
{
public:
    			uiEMEditor(uiParent*);
    virtual		~uiEMEditor();
    virtual void	setActiveNode( const EM::PosID& n ) { node=n; }
    virtual void	createNodeMenus(CallBacker*) {}
    virtual void	handleNodeMenus(CallBacker*) {}
    virtual void	createInteractionLineMenus(CallBacker*) {}
    virtual void	handleInteractionLineMenus(CallBacker*) {}

protected:
    EM::PosID		node;
    uiParent*		parent;
};


/*! Factory function that can produce a MPE::uiEMEditor* given a 
    uiParent* and a MPE::ObjectEditor*. \note that the function should
    return a zero pointer if the MPE::ObjectEditor* is of the wrong kind. */


typedef uiEMEditor*(*uiEMEditorCreationFunc)(uiParent*,MPE::ObjectEditor*);

/*! Factory that is able to create MPE::uiEMEditor objects given a uiParent*
    and a MPE::ObjectEditor*. Each class that wants to be able to procuce
    instances of itself must register itself with the addFactory startup. */

class uiEMEditorFactory
{
public:
    void		addFactory( uiEMEditorCreationFunc f );
    uiEMEditor*		create( uiParent*, MPE::ObjectEditor* e ) const;
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */

protected:
    TypeSet<uiEMEditorCreationFunc>	funcs;

};


/*! Interface to track-setup groups. Implementations can be retrieved through
    MPE::uiSetupGroupFactory. */


class uiSetupGroup : public uiGroup
{
public:
				uiSetupGroup(uiParent*,const char* helpref);
    virtual void		setSectionTracker(SectionTracker*)	{}
    virtual bool		commitToTracker() const { return true; }

    BufferString		helpref_;
};
    

/*! Factory function that can produce a MPE::uiSetupGroup* given a 
    uiParent* and an Attrib::DescSet*. */

typedef uiSetupGroup*(*uiSetupGrpCreationFunc)(uiParent*,const char* typestr,	
					       const Attrib::DescSet*);

/*! Factory that is able to create MPE::uiSetupGroup* given a uiParent*,
    and an Attrib::DescSet*. Each class that wants to
    be able to procuce instances of itself must register itself with the
    addFactory startup. */

class uiSetupGroupFactory
{
public:
    void		addFactory(uiSetupGrpCreationFunc f);
    uiSetupGroup*	create(uiParent*,const char* typestr,
	    		       const Attrib::DescSet*);
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */

protected:
    TypeSet<uiSetupGrpCreationFunc>	funcs;

};


/*! Holder class for MPE ui-factories. Is normally only retrieved by
    MPE::uiMPE(). */


class uiMPEEngine 
{
public:
    uiEMEditorFactory		editorfact;
    uiSetupGroupFactory		setupgrpfact;
};



/*! Access function for an instance (and normally the only instance) of
  MPE::uiMPEEngine. */
uiMPEEngine& uiMPE();

};


#endif
