#ifndef useraction_h
#define useraction_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2007
 RCS:           $Id: useraction.h,v 1.1 2007-12-27 15:59:46 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "callback.h"
#include "bufstring.h"

class UserAction : public CallBacker
{
public:
    class Setup
    {
    public:
		    Setup();

		    mDefSetupMemb(bool,checkable);
		    mDefSetupMemb(bool,checked);
		    mDefSetupMemb(bool,enabled);
		    mDefSetupMemb(bool,visible);
		    mDefSetupMemb(BufferString,icon);
		    mDefSetupMemb(BufferString,icontxt);
		    mDefSetupMemb(BufferString,text);
		    mDefSetupMemb(BufferString,tooltip);
    };
    		UserAction( const UserAction::Setup& );
		UserAction( const UserAction& );

    Setup			setup_;

    int				id_;

    Notifier<UserAction>	change_;
};


class UserActionGroup
{
public:
    virtual		~UserActionGroup();
    virtual void	addAction(UserAction*,bool manage=false);
    virtual bool	canAddGroup() const;
    virtual void	addGroup(UserActionGroup*,bool manage=false);

    virtual void	removeAll();

protected:
    ObjectSet<UserAction>	actions_;
    BoolTypeSet			actionsownership_;

    ObjectSet<UserActionGroup>	groups_;
    BoolTypeSet			groupsowership_;
};


class UserActionHandler : public UserActionGroup
{
public:
    int					visID() const;

    Notifier<UserActionHandler>		createnotifier;
    CNotifier<UserActionHandler,int>	handlenotifier;
    bool				isHandled() const;
					/*!<Should be called as the first thing
					    from callbacks that is triggered
					    from handlenotifier. If isHandled()
					    returns true, the callback should
					    return immediately. */
    void				setHandled(bool);
					/*!<Should be called from callbacks that
					    are triggered from handlenotifier
					    if they have found the menu id they
					    are looking for.  */

protected:
    				UserActionHandler(int visid);
    int				getFreeID();

    int				freeid_;
    int				visid_;
    bool			ishandled_;
};

#endif
