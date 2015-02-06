#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uistring.h"

class uiButtonBody;
class uiCheckBoxBody;
class uiPushButtonBody;
class uiRadioButtonBody;
mFDQtclass(QAbstractButton)

class uiMenu;
class uiPixmap;
mFDQtclass(QEvent)
mFDQtclass(QMenu)


//!\brief is the base class for all buttons.

mExpClass(uiBase) uiButton : public uiObject
{
public:
			uiButton(uiParent*,const uiString&,const CallBack*,
				 uiObjectBody&);
    virtual		~uiButton()		{}

    virtual void	setText(const uiString&);
    const uiString&	text() const			{ return text_; }
    void		setIcon(const char* icon_identifier);
    void		setPixmap( const uiPixmap& pm ) { setPM(pm); }
    void		setIconScale(float val); /*!< val between [0-1] */
    virtual void	updateIconSize()	{}

    virtual void	click()		= 0;

    Notifier<uiButton>	activated;

			//! Not for casual use
    mQtclass(QAbstractButton*)	qButton();

    enum StdType	{ Select, Apply, Save, SaveAs, Edit, Examine,
			  Options, Settings, Properties,
			  Help, Ok, Cancel };
    static uiButton*	getStd(uiParent*,StdType,const CallBack&,bool immediate,
				const char* nonstd_text=0);
    static uiButton*	getStd(uiParent*,StdType,const CallBack&,bool immediate,
				const uiString& nonstd_text);
    static bool		haveCommonPBIcons()	{ return havecommonpbics_; }
    static void		setHaveCommonPBIcons( bool yn=true )
    						{ havecommonpbics_ = yn; }

protected:

    uiString		text_;
    float		iconscale_;
    static bool		havecommonpbics_;

    virtual void	translateText();
    virtual void	setPM(const uiPixmap&);

};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
*/

mExpClass(uiBase) uiPushButton : public uiButton
{
public:
			uiPushButton(uiParent*,const uiString& txt,
				     bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const CallBack&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,const CallBack&,
				     bool immediate);

    void		setDefault(bool yn=true);
    void		click();
    void		setMenu(uiMenu*);

private:

    void		translateText();
    void		updateText();
    void		updateIconSize();

    bool		immediate_;
    uiPushButtonBody*	pbbody_;
    uiPushButtonBody&	mkbody(uiParent*,const uiString&);
};


mExpClass(uiBase) uiRadioButton : public uiButton
{
public:
			uiRadioButton(uiParent*,const uiString&);
			uiRadioButton(uiParent*,const uiString&,
				      const CallBack&);

    bool		isChecked() const;
    virtual void	setChecked(bool yn=true);

    void		click();

private:

    uiRadioButtonBody*	rbbody_;
    uiRadioButtonBody&	mkbody(uiParent*,const uiString&);

};


mExpClass(uiBase) uiCheckBox: public uiButton
{
public:

			uiCheckBox(uiParent*,const uiString&);
			uiCheckBox(uiParent*,const uiString&,
				   const CallBack&);

    bool		isChecked() const;
    void		setChecked(bool yn=true);

    void		click();

private:

    uiCheckBoxBody*	cbbody_;
    uiCheckBoxBody&	mkbody(uiParent*,const uiString&);

};


//! Button Abstract Base class

mExpClass(uiBase) uiButtonMessenger
{
    friend class        i_ButMessenger;

public:

			uiButtonMessenger()		{}
    virtual		~uiButtonMessenger()		{}

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled };

protected:

    //! Handler called from Qt.
    virtual void	notifyHandler(notifyTp)		= 0;

};

#endif
