#ifndef uilabel_h
#define uilabel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "draw.h"
#include "uistring.h"

mFDQtclass(QLabel);
class uiGroup;
class uiPixmap;

mExpClass(uiBase) uiLabel : public uiSingleWidgetObject
{
public:

			uiLabel(uiParent*,const uiString&);
			uiLabel(uiParent*,const uiString&,uiObject*);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/
    virtual void	setText(const uiString&);
    const uiString&	text() const;
    void		setTextSelectable(bool yn=true);
    void		setPixmap(const uiPixmap&);

/*!
    setting an alignment only makes sense if you reserve space using
    setPrefWidthInChar();
*/
    void		setAlignment(OD::Alignment::HPos);
    OD::Alignment::HPos alignment() const;

    void		makeRequired(bool yn=true);

private:
    void		translateText();

    void		init(const uiString& txt,uiObject* buddy);
    void		updateWidth();

    QLabel*		qlabel_;
    uiString		text_;

    bool		isrequired_;
};

#endif
