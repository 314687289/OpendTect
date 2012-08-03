#ifndef uiviscoltabed_h
#define uiviscoltabed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.29 2012-08-03 13:01:18 cvskris Exp $
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uidialog.h"

namespace visSurvey { class SurveyObject; }
namespace ColTab { class Sequence; class MapperSetup; }
class uiColorTable;
class uiGroup;
class IOPar;


mClass(uiVis) uiVisColTabEd : public CallBacker
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    void			setColTab(const ColTab::Sequence*,
	    				  bool editseq,
	    				  const ColTab::MapperSetup*,
					  bool edittrans);
    void			setColTab(visSurvey::SurveyObject*,int ch,
	    				  int version);
    const ColTab::Sequence&	getColTabSequence() const;
    const ColTab::MapperSetup&	getColTabMapperSetup() const;

    int				getChannel() const { return channel_; }
    const visSurvey::SurveyObject* getSurvObj() const { return survobj_; }

    NotifierAccess&		seqChange();
    NotifierAccess&		mapperChange();
    
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);
    void			setPrefWidth(int);
    uiGroup*			colTabGrp()	{ return (uiGroup*)uicoltab_; }
    uiColorTable*		colTab()	{ return uicoltab_; }

    bool			usePar(const IOPar&);
    void                        fillPar(IOPar&);
    void			setDefaultColTab();

    static const char*          sKeyColorSeq();
    static const char*          sKeyScaleFactor();
    static const char*          sKeyClipRate();
    static const char*          sKeyAutoScale();
    static const char*          sKeySymmetry();
    static const char*          sKeySymMidval();
    void			colseqChanged(CallBacker*);
    void			colorTabChgdCB(CallBacker*);
    void			clipperChanged(CallBacker*);

protected:

    void			mapperChangeCB(CallBacker*);
    void			removeAllVisCB(CallBacker*);


    uiColorTable*		uicoltab_;
    int				channel_;
    int				version_;
    visSurvey::SurveyObject*	survobj_;
};


mClass(uiVis) uiColorBarDialog :  public uiDialog
{
public:
    				uiColorBarDialog(uiParent*,const char* title);

    uiVisColTabEd&		editor()	{ return *coltabed_; }

    Notifier<uiColorBarDialog>	winClosing;

protected:
    bool			closeOK();
    uiVisColTabEd*		coltabed_;
};


#endif

