#ifndef uimenuhandler_h
#define uimenuhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.h,v 1.7 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "menuhandler.h"
#include "position.h"

class uiPopupMenu;
class uiMenuItem;
/*
Implementation of MenuHandler for the dGB-based userinterface.
*/


mClass uiMenuHandler : public MenuHandler
{
public:
    				uiMenuHandler( uiParent*, int id );

    uiParent*			getParent() const { return uiparent_; }

    bool			executeMenu();
    bool			executeMenu(int menutype,
	    				    const TypeSet<int>* path=0 );
    				/*!<\param menutype is an integer that specifies
				   	   what type of menu should be
					   generated. Two numbers are reserved,
					   and the user of the class may use his
					   own codes for other circumstances.
					   The two defined values are:
					   - menutype==fromTree  menu generated
					   	from (a right-click on) the
						treeitem.
					   - menutype==fromScene menu generated
					   	from the scene.
				    \param path If menutype==fromScene the path
				     	   of selection (i.e. a list of the
					   ids of the paht, from scene to picked
					   object).

				*/
    int				getMenuType() const { return menutype_; }
    				/*!<\returns the \a menutype specified in
					  uiMenuHandler::executeMenu.
				    \note does only give a valid
				          answer if called from a callback,
					  notified by
					  uiMenuHandler::createnotifier
					  or uiMenuHandler::handlenotifier.  */
    const TypeSet<int>*		getPath() const { return path_; }
    				/*!<\returns The path of selection (i.e. a list
				  	   of the ids of the paht, from scene
					   to picked object). */
    const Coord3&		getPickedPos() const { return positionxyz_; }
    void			setPickedPos(const Coord3& pickedpos)
					{ positionxyz_=pickedpos; }
    const Geom::Point2D<double>& get2DPickedPos() const { return positionxy_; }
    void			set2DPickedPos(const Geom::Point2D<double>& pos)
					{ positionxy_=pos; }
    
    static const int		fromTree();
    static const int		fromScene();

protected:
    bool			executeMenuInternal();
    uiPopupMenu*		createMenu( const ObjectSet<MenuItem>&,
	    				    const MenuItem* =0);
    uiParent*			uiparent_;
    int				menutype_;
    const TypeSet<int>*		path_;
    Coord3			positionxyz_;
    Geom::Point2D<double>	positionxy_;
    				~uiMenuHandler() {}
};


#endif
