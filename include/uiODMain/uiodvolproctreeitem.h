#ifndef uiodvolproctreeitem_h
#define uiodvolproctreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2007
 RCS:		$Id: uiodvolproctreeitem.h,v 1.5 2012-08-03 13:01:04 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"
#include "multiid.h"

namespace VolProc
{

mClass(uiODMain) uiDataTreeItem : public uiODDataTreeItem
{
public:
   static void			initClass();
   				~uiDataTreeItem();
				
				uiDataTreeItem(const char* parenttype);

    bool			selectSetup();

    static const char*		sKeyVolumeProcessing()
				{ return "VolumeProcessing"; }

protected:
    
    bool			anyButtonClick(uiListViewItem*);
   
    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			createMenu(MenuHandler* menu ,bool istoolbar);
    void			handleMenuCB(CallBacker*);
    BufferString		createDisplayName() const;
    void			updateColumnText( int col );

    MenuItem			selmenuitem_;
    MenuItem			reloadmenuitem_;
    MenuItem			editmenuitem_;

    MultiID			mid_;

};

}; //namespace

#endif

