#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.h,v 1.27 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiButtonBody;
class uiCheckBoxBody;
class uiPushButtonBody;
class uiRadioButtonBody;
class uiToolButtonBody;

class uiPopupMenu;
class ioPixmap;
class QEvent;


//!\brief Button Abstract Base class
mClass uiButton : public uiObject
{
public:
			uiButton(uiParent*,const char*,const CallBack*,
				 uiObjectBody&);
    virtual		~uiButton()		{}

    virtual void	setText(const char*);
    const char*		text();

    virtual void	click()			{}

    Notifier<uiButton>	activated;

    			//! Force activation in GUI thread
    void		activate();
    Notifier<uiButton>	activatedone;
};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
  */

mClass uiPushButton : public uiButton
{
public:
				uiPushButton(uiParent*,const char* nm,
					     bool immediate);
				uiPushButton(uiParent*,const char* nm,
					     const CallBack&,
					     bool immediate); 
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&,
					     bool immediate);
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&,const CallBack&,
					     bool immediate);
				~uiPushButton();

    void			setDefault(bool yn=true);
    void			setPixmap(const ioPixmap&);
    				//! Size of pixmap is 1/2 the size of button

    void			click();

private:

    uiPushButtonBody*		body_;
    uiPushButtonBody&		mkbody(uiParent*,const ioPixmap*,const char*,
	    				bool);
};


mClass uiRadioButton : public uiButton
{                        
public:
				uiRadioButton(uiParent*,const char*);
				uiRadioButton(uiParent*,const char*,
					      const CallBack&);

    bool			isChecked() const;
    virtual void		setChecked(bool yn=true);

    void			click();

private:

    uiRadioButtonBody*		body_;
    uiRadioButtonBody&		mkbody(uiParent*,const char*);

};


mClass uiCheckBox: public uiButton
{
public:

				uiCheckBox(uiParent*,const char*);
				uiCheckBox(uiParent*,const char*,
					   const CallBack&);

    bool			isChecked() const;
    void			setChecked(bool yn=true);

    void			click();

    virtual void		setText(const char*);

private:

    uiCheckBoxBody*		body_;
    uiCheckBoxBody&		mkbody(uiParent*,const char*);

};


mClass uiToolButton : public uiButton
{
public:
				uiToolButton(uiParent*,const char*);
				uiToolButton(uiParent*,const char*,
					     const CallBack&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&,const CallBack&);

    enum ArrowType		{ NoArrow, UpArrow, DownArrow,
				  LeftArrow, RightArrow };

    bool			isOn() const;
    void			setOn(bool yn=true);

    void			setToggleButton(bool yn=true);
    bool			isToggleButton() const;

    void			setPixmap(const ioPixmap&);
    void			setArrowType(ArrowType);

    void			setShortcut(const char*);

    void			setMenu(const uiPopupMenu&);
    
    void			click();

private:

    uiToolButtonBody*		body_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap*, const char*); 

};


//! Button Abstract Base class
mClass uiButtonBody
{
    friend class        i_ButMessenger;

public:
			uiButtonBody()				{}
    virtual		~uiButtonBody()				{}

    virtual void	activate()				=0;

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled };
    
protected:
    virtual bool	handleEvent(const QEvent*)		=0;

    //! Handler called from Qt.
    virtual void        notifyHandler(notifyTp)			=0;
};




#endif
