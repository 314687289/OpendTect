#ifndef uicoltabman_h
#define uicoltabman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          February 2008
 RCS:           $Id: uicoltabman.h,v 1.7 2008-11-26 06:59:15 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "bufstringset.h"

class uiColorTableCanvas;
class uiColTabMarkerCanvas;
class uiFunctionDisplay;
class IOPar;
class uiCanvas;
class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiListView;
class uiPushButton;
class uiSpinBox;
class uiWorld2Ui;


class uiColorTableMan : public uiDialog
{
public:
				uiColorTableMan(uiParent*,ColTab::Sequence&);
				~uiColorTableMan();

    const ColTab::Sequence&	currentColTab()	const	{ return ctab_; }

    void			setHistogram(const TypeSet<float>&);

    Notifier<uiColorTableMan> 	tableAddRem;
    Notifier<uiColorTableMan> 	tableChanged;

protected:

    uiFunctionDisplay*		cttranscanvas_;
    uiColorTableCanvas*		ctabcanvas_;
    uiColTabMarkerCanvas*	markercanvas_;
    uiListView*			coltablistfld_;
    uiPushButton*       	removebut_;
    uiPushButton*       	importbut_;
    uiColorInput*       	undefcolfld_;
    uiCheckBox*			segmentfld_;
    uiSpinBox*			nrsegbox_;
    uiWorld2Ui*			w2uictabcanvas_;

    BufferString		selstatus_;
    ColTab::Sequence&         	ctab_;
    ColTab::Sequence*         	orgctab_;

    bool			issaved_;
    int				selidx_;

    void			doFinalise(CallBacker*);
    void			reDraw(CallBacker*);
    void			markerChgd(CallBacker*);
    void			selChg(CallBacker*);
    void			removeCB(CallBacker*);
    void			saveCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);


    void			refreshColTabList(const char*);

    bool			saveColTab(bool);

    void			segmentSel(CallBacker*);
    void			nrSegmentsCB(CallBacker*);
    void			updateSegmentFields();

    void			undefColSel(CallBacker*);
    void			rightClick(CallBacker*);
    void			doSegmentize();
    void			importColTab(CallBacker*);
    void			transptSel(CallBacker*);
    void			transptChg(CallBacker*);
    void			sequenceChange(CallBacker*);
    void			markerChange(CallBacker*);
};

#endif
