#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.6 2005-10-04 14:38:43 cvskris Exp $
________________________________________________________________________


-*/

#include "colortab.h"
#include "uigroup.h"
#include "uidialog.h"

namespace visBase { class VisColorTab; }
class ColorTableEditor;

/*!\brief

*/

class uiVisColTabEd : public uiGroup
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    void			setColTab(int coltabid);
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);

protected:

    void			colTabEdChangedCB(CallBacker*);
    void			colTabChangedCB(CallBacker*);
    void			delColTabCB(CallBacker*);
    void			updateEditor();
    void			enableCallBacks();
    void			disableCallBacks();

    const CallBack		coltabcb;

    visBase::VisColorTab*	coltab;
    ColorTableEditor*		coltabed;

    ColorTable			colseq;
    Interval<float>		coltabinterval;
    bool			coltabautoscale;
    float			coltabcliprate;
};


class uiColorBarDialog :  public uiDialog
{
public:
    				uiColorBarDialog( uiParent* , int coltabid,
						  const char* title);

    void			setColTab( int id );
    Notifier<uiColorBarDialog>	winClosing;

protected:
    bool			closeOK();
    uiVisColTabEd*		coltabed;
};


#endif
