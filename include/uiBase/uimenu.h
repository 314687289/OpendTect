#ifndef uimenu_h
#define uimenu_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.24 2005-09-28 12:10:34 cvsarend Exp $
________________________________________________________________________

-*/

#include "uihandle.h"

class uiParent;

class uiMenuItem;
class uiPopupMenu;
class uiPopupItem;

class uiMenuItemContainerBody;
class i_MenuMessenger;


class QMenuBar;
class QPopupMenu;

template<class T> class ObjectSet;


class uiMenuItemContainer : public uiObjHandle
{
friend class			uiMenuItemContainerBody;
protected:
				uiMenuItemContainer( const char* nm,
						uiMenuItemContainerBody* b );
public:
				~uiMenuItemContainer();

    int				nrItems() const;
    const ObjectSet<uiMenuItem>& items() const;
	
    int				insertItem(uiMenuItem*,int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    int				insertItem(const char* text, const CallBack& cb,
					   int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    int				insertItem(uiPopupMenu*,int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    void			insertSeparator(int idx=-1);

    int				idAt(int idx) const;
    int				indexOf(int id) const;

protected:

    void			setMenuBody(uiMenuItemContainerBody* b);
    uiMenuItemContainerBody*	body_;

};

/*! 

    Stores the id of the item in Qt and has a 
    messenger, so Qt's signals can be relayed.

*/
class uiMenuItem : public UserIDObject
{
friend class			uiMenuItemContainerBody;

public:
				uiMenuItem( const char* nm );
				uiMenuItem( const char* nm, const CallBack& cb);
				~uiMenuItem();

				//! sets a new text 2b displayed
    void			setText(const char*);

    bool			isEnabled() const;
    void			setEnabled(bool);
    bool			isChecked() const;
    void 			setChecked(bool);

    Notifier<uiMenuItem>	activated;

    int				id() const			{ return id_; }
    int				index() const;

protected:

    void 			setId(int i)			{ id_ = i; }
    void			setMenu(uiMenuItemContainerBody* m){ menu_ = m;}

    i_MenuMessenger*		messenger()		{ return &messenger_; }
    uiMenuItemContainerBody*	menu_;

private:

    i_MenuMessenger&            messenger_;

    int                         id_;
    bool			checked;
    bool			enabled;

};


class uiPopupItem : public uiMenuItem
{
friend class uiPopupMenu;
protected:
                                uiPopupItem( uiPopupMenu& , const char* nm);
public:

    bool                        isCheckable() const;
    void                        setCheckable(bool);

    uiPopupMenu&		menu()		{ return *popmenu_; }
    const uiPopupMenu&		menu() const	{ return *popmenu_; }
    
protected:

    uiPopupMenu*		popmenu_;

};


class QPixmap;

class uiMenuBar : public uiMenuItemContainer
{

    friend class		uiMainWinBody;
    friend class		uiDialogBody;

public:

    void			setIcon(const QPixmap&);
    void			setSensitive(bool yn);
    				/*!< Works on complete menubar */

protected:
				uiMenuBar( uiParent* parnt, const char* nm );
				uiMenuBar( uiParent* parnt, const char* nm, 
					   QMenuBar& );

    void 			reDraw(bool deep=true);

};

class uiPopupMenu : public uiMenuItemContainer
{

public:                        
				uiPopupMenu(uiParent* parnt,
					    const char* nm="uiPopupMenu");
				uiPopupMenu(uiParent* parnt,
					    QPopupMenu* qmnu,
					    const char* nm="uiPopupMenu");

				~uiPopupMenu();

    bool			isCheckable() const;
    void			setCheckable(bool);

    bool			isEnabled() const;
    void			setEnabled(bool);

    bool			isChecked() const;
    void			setChecked(bool);

				/*! \brief pops-up at mouse position

				    The return code is the id of the selected 
				    item in either the popup menu or one of 
				    its submenus, or -1 if no item is selected
				    (normally because the user presses Escape). 
				*/
    int				exec(); 

    uiPopupItem&		item()			{ return item_; }
    const uiPopupItem&		item() const		{ return item_; }

private:

    uiPopupItem&		item_;
};

#endif
