#ifndef uifontsel_H
#define uifontsel_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          25/9/2000
 RCS:           $Id: uifontsel.h,v 1.6 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class uiButton;
class uiLabeledComboBox;
class uiFont;


bool	select( uiFont&, uiParent* parnt=0, const char* nm=0 );
	/*!< \brief pops a selector box to select a new font
	     \return true if new font selected
	*/

class uiSetFonts : public uiDialog
{
public:

		uiSetFonts(uiParent*,const char*);

protected:

    ObjectSet<uiButton>	buttons;

    void	butPushed(CallBacker*);

};


class uiSelFonts : public uiDialog
{
public:

		uiSelFonts(uiParent*,const char*,const char*);
		~uiSelFonts();

    void	add(const char* str,const char* stdfontkey);

    const char*	resultFor(const char* str);

protected:

    ObjectSet<uiLabeledComboBox>	sels;
    BufferStringSet			ids;

};


#endif
