#ifndef uiaction_h
#define uiaction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: uiaction.h,v 1.6 2011-03-28 07:55:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class QAction;
class ioPixmap;
class MenuItem;

mClass uiAction : public CallBacker
{
friend class		i_ActionMessenger;
public:
                        uiAction(const char*);
                        uiAction(const char*,const CallBack&);
                        uiAction(const char*,const CallBack&,const ioPixmap&);
			uiAction(const MenuItem&);
			uiAction(QAction*);
			~uiAction();

    void		setText(const char*);
    const char*		text() const;
    void		setIconText(const char*);
    const char*		iconText() const;
    void		setToolTip(const char*);
    const char*		toolTip() const;

    void		setPixmap(const ioPixmap&);

    void		setCheckable(bool);
    bool		isCheckable() const;
    void		setChecked(bool);
    bool		isChecked() const;
    void		setEnabled(bool);
    bool		isEnabled() const;
    void		setVisible(bool);
    bool		isVisible() const;

    QAction*		qaction()		{ return qaction_; }

    Notifier<uiAction>	toggled;
    Notifier<uiAction>	triggered;

private:

    i_ActionMessenger*	msgr_;
    QAction*		qaction_;
    CallBack		cb_;
    bool		checked_;

    void		init(const char*);
};

#endif
