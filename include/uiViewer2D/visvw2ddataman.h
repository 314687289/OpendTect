#ifndef visvw2ddataman_h
#define visvw2ddataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddataman.h,v 1.4 2011-06-03 14:10:26 cvsbruno Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "factory.h"
#include "emposid.h"


class Vw2DDataObject;
class IOPar;

class uiFlatViewWin; 
class uiFlatViewAuxDataEditor;



mClass Vw2DDataManager : public CallBacker
{
public:
    				Vw2DDataManager();
				~Vw2DDataManager();

    void			addObject(Vw2DDataObject*);
    void			removeObject(Vw2DDataObject*);
    void                        removeAll();

    void			getObjects(ObjectSet<Vw2DDataObject>&) const;

    const Vw2DDataObject*	getObject(int id) const;
    Vw2DDataObject*		getObject(int id);

    void			setSelected(Vw2DDataObject*);

    void			usePar(const IOPar&,uiFlatViewWin*,
				    const ObjectSet<uiFlatViewAuxDataEditor>&);
    void			fillPar(IOPar&) const;

    mDefineFactory3ParamInClass(Vw2DDataObject,
		    const EM::ObjectID&,uiFlatViewWin*,
		    const ObjectSet<uiFlatViewAuxDataEditor>&,factory);

    Notifier<Vw2DDataManager>	addRemove;

protected:

    void                        deSelect(int id);

    ObjectSet<Vw2DDataObject>	objects_;
    int				selectedid_;
    int				freeid_;

    static const char* 		sKeyNrObjects() 	{ return "Nr objects"; }
};

#endif
