#ifndef uihorizonrelations_h
#define uihorizonrelations_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uihorizonrelations.h,v 1.7 2012-08-03 13:00:56 cvskris Exp $
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "multiid.h"

class uiLabeledListBox;
class uiPushButton;
class BufferStringSet;

mClass(uiEarthModel) uiHorizonRelationsDlg : public uiDialog
{
public:
			uiHorizonRelationsDlg(uiParent*,bool is2d);

protected:
    uiLabeledListBox*	relationfld_;
    uiPushButton*	crossbut_;
    uiPushButton*	waterbut_;

    BufferStringSet	hornames_;
    TypeSet<MultiID>	horids_;
    bool		is2d_;

    void		fillRelationField(const BufferStringSet&);

    void		readHorizonCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
};

#endif

