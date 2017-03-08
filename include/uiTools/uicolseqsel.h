#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uitoolbar.h"
#include "coltabsequence.h"

class uiLabel;
class uiCheckBox;
class uiColSeqMan;
class uiColSeqDisp;
class uiColSeqUseModeCompactSel;


/* Color sequence selection object. */

mExpClass(uiTools) uiColSeqSelTool : public CallBacker
{ mODTextTranslationClass(uiColSeqSelTool);
public:

    typedef ColTab::Sequence	Sequence;

    virtual			~uiColSeqSelTool();

    ConstRefMan<Sequence>	sequence() const;
    void			setSequence(const Sequence&);
    const char*			seqName() const;
    void			setSeqName(const char*);
    ColTab::SeqUseMode		seqUseMode() const;
    void			setSeqUseMode(ColTab::SeqUseMode);
    void			setNonSeisDefault();

    uiColSeqDisp*		seqDisp()		{ return disp_; }

    Notifier<uiColSeqSelTool>	seqChanged;
    Notifier<uiColSeqSelTool>	newManDlg;
    Notifier<uiColSeqSelTool>	seqModified;
				//!< no change details, if needed, start
				//!< watching the current sequence for yourself
    Notifier<uiColSeqSelTool>	refreshReq;
				//!< Only to be picked up by viewers that do
				//!< no automatic updating
    Notifier<uiColSeqSelTool>	seqMenuReq;
				//!< CallBacker* is the uiMenu about to pop up

    void			setCurrentAsDefault();
    void			showManageDlg();

    OD::Orientation		orientation() const;
    virtual bool		isGroup() const			{ return false;}
    virtual uiParent*		asParent()			= 0;
    virtual bool		isFinalised() const		= 0;
    virtual void		addObjectsToToolBar(uiToolBar&);
    virtual void		orientationChanged();

    static int			maxElemLongDimSize();
				//!< max size in pixels of each element

protected:

				uiColSeqSelTool();

    uiColSeqDisp*		disp_;
    uiColSeqMan*		mandlg_;

    void			initDisp(CallBacker*);
    void			seqModifCB(CallBacker*);
    void			selectCB(CallBacker*);
    void			menuCB(CallBacker*);
    void			colSeqModifiedCB(CallBacker*);
    void			upCB(CallBacker*)	{ nextColSeq(true); }
    void			downCB(CallBacker*)	{ nextColSeq(false); }
    void			setAsDefaultCB(CallBacker*);
    void			manDlgSeqSelCB(CallBacker*);
    void			manageCB(CallBacker*)	{ showManageDlg(); }
    void			manDlgCloseCB(CallBacker*) { mandlg_ = 0; }

    void			initialise(OD::Orientation);
    virtual void		setIsVertical(bool);
    void			nextColSeq(bool prev);
    void			setToolTip();
    uiParent*			getParent()
				{ return isGroup() ? asParent() : 0; }

};


#define mImpluiColSeqSelGroup() \
    virtual uiParent*		asParent()		{ return this; } \
    virtual bool		isGroup() const		{ return true; } \
    virtual bool		isFinalised() const	{ return finalised(); }\
    void			setLabelText(const uiString&); \
    void			setOrientation( OD::Orientation orient ) \
				{ setIsVertical(orient==OD::Vertical); } \
protected: \
    uiLabel*			lbl_

/* uiGroup for color sequence selection */

mExpClass(uiTools) uiColSeqSel : public uiGroup
				, public uiColSeqSelTool
{
public:

			uiColSeqSel(uiParent*,OD::Orientation,
				uiString lbl=uiString::emptyString());

			mImpluiColSeqSelGroup();

};


#define mImpluiColSeqSelToolBar(seltoolclssnm) \
\
    seltoolclssnm&	selTool()			{ return seltool_; } \
    operator		seltoolclssnm&()		{ return seltool_; } \
\
protected: \
\
    seltoolclssnm&	seltool_;



/* uiToolBar for color sequence selection */

mExpClass(uiTools) uiColSeqToolBar : public uiToolBar
{ mODTextTranslationClass(uiColSeqToolBar);
public:

			uiColSeqToolBar(uiParent*);

			mImpluiColSeqSelToolBar(uiColSeqSelTool)

};


/* uiToolBar color sequence use mode selection */

mExpClass(uiTools) uiColSeqUseModeSel : public uiGroup
{ mODTextTranslationClass(uiColSeqUseModeSel);
public:

			uiColSeqUseModeSel(uiParent*,bool compact,
					   uiString lbltxt=tr("Use table"));
			~uiColSeqUseModeSel();

    ColTab::SeqUseMode	mode() const;
    void		setMode(ColTab::SeqUseMode);
    void		addObjectsToToolBar(uiToolBar&);

    Notifier<uiColSeqUseModeSel>    modeChange;

protected:

    uiCheckBox*		flippedbox_;
    uiCheckBox*		cyclicbox_;
    uiColSeqUseModeCompactSel* compactsel_;

    void		modeChgCB( CallBacker* )	{ modeChange.trigger();}
    void		canvasMouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

};


/* Defines toolbar tool class in .cc file. */

#define mImpluiColSeqSelToolBarTool(tbarclssnm,baseclssnm) \
\
class tbarclssnm##Tool : public baseclssnm \
{ \
public: \
\
tbarclssnm##Tool( tbarclssnm* tb ) \
    : tbar_(tb) \
{ \
    initialise( tbar_->getOrientation() ); \
    addObjectsToToolBar( *tbar_ ); \
    mAttachCB( tbar_->orientationChanged, tbarclssnm##Tool::orChgCB ); \
} \
\
void orChgCB( CallBacker* ) \
{ \
    setIsVertical( tbar_->getOrientation() == OD::Vertical ); \
    orientationChanged(); \
} \
\
virtual uiParent* asParent() \
{ \
    return tbar_; \
} \
\
virtual bool isFinalised() const \
{ \
    return tbar_->finalised(); \
} \
\
    tbarclssnm*	tbar_; \
\
};
