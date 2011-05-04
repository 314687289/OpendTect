#ifndef uipsviewer2dposdlg_h
#define uipsviewer2dposdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2011
 RCS:           $Id: uipsviewer2dposdlg.h,v 1.4 2011-05-04 15:20:02 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"

class uiCheckBox;
class uiListBox;
class uiToolButton;

namespace PreStackView
{

mClass uiGatherPosSliceSel : public uiSliceSel
{
public:
				uiGatherPosSliceSel(uiParent*,uiSliceSel::Type);

    const CubeSampling&		cubeSampling(); 
    void			setNrViewers(int);
    int				nrViewers() const;
    void 			setStep(int);	
    int 			step() const;	

    void			enableDynamicRange(bool);
    void			enableZDisplay(bool);
    bool			isDynamicRange() const;

protected:
    uiLabeledSpinBox* 		stepfld_;
    uiLabeledSpinBox*		nrviewersfld_;
    uiCheckBox*			dynamicrgbox_;

    void			dynamicRangeChged(CallBacker*);
    void			posChged(CallBacker*);
    void			nrViewersChged(CallBacker*);
    void			applyPushed(CallBacker*);
};

mClass uiViewer2DPosDlg : public uiDialog
{
public:
				uiViewer2DPosDlg(uiParent*,const CubeSampling&);

    void 			setCubeSampling(const CubeSampling&);
    void 			getCubeSampling(CubeSampling&);

    void                        setNrViewers(int nrv)
				    { sliceselfld_->setNrViewers(nrv); }
    int				nrViewers() const
				    { return sliceselfld_->nrViewers(); }
    bool			isDynamicRange() const
				    { return sliceselfld_->isDynamicRange(); }

    void                        enableDynamicRange(bool yn)
				    { sliceselfld_->enableDynamicRange(yn); }
    void                        enableZDisplay(bool yn)
				    { sliceselfld_->enableZDisplay(yn); }

    Notifier<uiViewer2DPosDlg> okpushed_;

protected:

    uiGatherPosSliceSel*	sliceselfld_;
    bool			acceptOK(CallBacker*);
};


mClass uiViewer2DSelDataDlg : public uiDialog
{
public: 	
			    uiViewer2DSelDataDlg(uiParent*,
				    const BufferStringSet&,BufferStringSet&);
protected:

    uiListBox*			allgatherfld_;
    uiListBox*			selgatherfld_;
    uiToolButton*		toselect_;	
    uiToolButton*		fromselect_;

    BufferStringSet&		selgathers_;	

    void 			selButPush(CallBacker*);
    bool 			acceptOK(CallBacker*);
};

}; //namespace

#endif
