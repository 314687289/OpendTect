#ifndef uiflatviewcoltabed_h
#define uiflatviewcoltabed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		May 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiflatviewmod.h"
#include "callback.h"
#include "flatview.h"

namespace ColTab { class Sequence; }
class uiColorTable;


/*!
\brief FlatView color table editor.
*/

mExpClass(uiFlatView) uiFlatViewColTabEd : public CallBacker
{
public:
			uiFlatViewColTabEd(uiColorTable&,FlatView::Viewer&);
			~uiFlatViewColTabEd();

    void		setColTab( const FlatView::Viewer& );
    void		setColTab(const FlatView::DataDispPars::VD&);
    void		setSensitive(bool yn);

    FlatView::DataDispPars::VD& getDisplayPars()
				{ return vdpars_; }
    Notifier<uiFlatViewColTabEd> colTabChgd;

protected:

    FlatView::DataDispPars&	ddpars_;
    FlatView::DataDispPars::VD	vdpars_;
    ColTab::Sequence&		colseq_;
    uiColorTable&		uicoltab_;
    FlatView::Viewer*		vwr_;

    void			colTabChanged(CallBacker*);
};

#endif

