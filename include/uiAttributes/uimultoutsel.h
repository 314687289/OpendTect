#ifndef uimultoutsel_h
#define uimultoutsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jan 2009
 RCS:           $Id: uimultoutsel.h,v 1.3 2011-01-20 10:32:17 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BufferStringSet;
class uiCheckBox;
class uiListBox;
namespace Attrib { class Desc; };

/*! \brief Dialog used to determine which outputs of an attribute will be computed at once and stored as multiple components when creating a volume output */

mClass uiMultOutSel : public uiDialog
{
public:
			uiMultOutSel(uiParent*,const Attrib::Desc&);

    void		getSelectedOutputs(TypeSet<int>&) const;
    void		getSelectedOutNames(BufferStringSet&) const;
    bool		doDisp() const;

protected:

    void		fillInAvailOutNames(Attrib::Desc*,
	                                    BufferStringSet&) const;

    void		createMultOutDlg(const BufferStringSet&);
    void		allSel(CallBacker*);

    uiListBox*		outlistfld_;
    uiCheckBox*		outallfld_;

};

#endif
