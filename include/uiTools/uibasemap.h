#ifndef uibasemap_h
#define uibasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id: uibasemap.h,v 1.9 2011-10-04 13:44:59 cvskris Exp $
________________________________________________________________________

-*/


#include "basemap.h"
#include "uigroup.h"


class uiGraphicsItemGroup;
class uiGraphicsView;
class uiWorld2Ui;

mClass uiBaseMapObject : public CallBacker
{
public:
    				uiBaseMapObject(BaseMapObject*);
    virtual			~uiBaseMapObject();

    void			setTransform(const uiWorld2Ui*);

    uiGraphicsItemGroup*	itemGrp()		{ return itemgrp_; }
    virtual void		update();

protected:
    friend			class uiBaseMap;

    void			changedCB(CallBacker*);

    uiGraphicsItemGroup*	itemgrp_;
    const uiWorld2Ui*		transform_;

    BaseMapObject*		bmobject_;
};


mClass uiBaseMap : public BaseMap, public uiGroup
{
public:
				uiBaseMap(uiParent*);
    virtual			~uiBaseMap();

    void			addObject(BaseMapObject*);
    				//!Owned by caller
    void			removeObject(const BaseMapObject*);

    void			addObject(uiBaseMapObject*);
    				//! object becomes mine, obviously.
protected:

    int				indexOf(const BaseMapObject*) const;

    uiGraphicsView&		view_;
    ObjectSet<uiBaseMapObject>	objects_;

    uiWorld2Ui&			w2ui_;

    void			reSizeCB(CallBacker*)		{ reDraw(); }
    virtual void		reDraw(bool deep=true);

};

#endif
