#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;
class SoAction;
class SoCallback;

namespace osg { class Group; }

namespace visBase
{
    class SelectionManager;
    class EventCatcher;
    class PolygonOffset;
    class Light;

/*!\brief
    Scene manages all DataObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

mClass(visBase) Scene : public DataObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj(Scene);

    void		addObject(DataObject*);
    
    void		setAmbientLight(float);
    float		ambientLight() const;

    Light*		getLight() const;

    PolygonOffset*	getPolygonOffset()	{ return polygonoffset_; }
    bool		saveCurrentOffsetAsDefault() const;

    bool		blockMouseSelection(bool yn);
    			/*!<\returns previous status. */

    EventCatcher&	eventCatcher();

    void		setName(const char*);

    Notifier<Scene>	nameChanged;

    static const char*	sKeyOffset()	{ return "Polygon offset"; }
    static const char*	sKeyFactor()	{ return "Factor"; }
    static const char*	sKeyUnits()	{ return "Units"; }
    static const char*	sKeyLight()	{ return "Light"; }


protected:
    virtual		~Scene();
    EventCatcher&	events_;

    void		fillPar(IOPar&,TypeSet<int>&) const;
    int			usePar(const IOPar&);

    void		fillOffsetPar( IOPar& ) const;

    virtual SoNode*	gtInvntrNode();
    virtual osg::Node*	gtOsgNode();

    static void		firstRender(void*,SoAction*);

private:
    int			mousedownid_;

    void		mousePickCB(CallBacker*);
    void		removeCallback();

    SoEnvironment*	environment_;
    PolygonOffset*	polygonoffset_;
    Light*		light_;
    SoGroup*		selroot_;
    SoCallback*		callback_;
    bool		blockmousesel_;
    osg::Group*		osgsceneroot_;
};

}

#endif

