#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "flatview.h"
#include "threadwork.h"

namespace FlatView
{
    class AxesDrawer;
    class uiAuxDataDisplay;
}

class BufferStringSet;
class uiBitMapDisplay;
class uiFlatViewControl;
class uiGraphicsItemGroup;
class uiGraphicsView;
class uiWorld2Ui;

/*!
\brief Fulfills the FlatView::Viewer specifications using 'ui' classes.
*/

mExpClass(uiFlatView) uiFlatViewer : public uiGroup
				   , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();

    void		setInitialSize(uiSize);
    void		setRubberBandingOn(bool);

    bool		isAnnotInInt(const char* annotnm) const;
    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    uiGraphicsView&	rgbCanvas()			{ return *view_; }

    void		setView(const uiWorldRect&);
    void		setViewToBoundingBox();
    const uiWorldRect&	curView() const			{ return wr_; }
    			/*!<May be reversed if display is reversed. */
    uiWorldRect		boundingBox() const;

    void		getWorld2Ui(uiWorld2Ui&) const;
    uiRect		getViewRect() const;
    			/*!<The rectangle onto which wr_ is projected */
    uiBorder		getAnnotBorder() const;
    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setExtraBorders( uiRect r )	{ extraborders_ = r; }
    void		setExtraFactor( float f )	{ extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    void		handleChange(unsigned int);

    FlatView::AuxData*		createAuxData(const char* nm) const;
    int				nrAuxData() const;
    FlatView::AuxData*		getAuxData(int idx);
    const FlatView::AuxData*	getAuxData(int idx) const;
    void			addAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(int idx);
    void			reGenerate(FlatView::AuxData&);

    Notifier<uiFlatViewer> 	viewChanged; //!< setView called
    Notifier<uiFlatViewer> 	dataChanged; //!< new DataPack set
    Notifier<uiFlatViewer> 	dispParsChanged;
    					//!< Triggered with each bitmap update
    Notifier<uiFlatViewer>	annotChanged; //!< Annotation changed
    Notifier<uiFlatViewer>	dispPropChanged;
    					//!< Triggered with property dlg change

    uiFlatViewControl*		control() { return control_; }

			//restrain the data ranges between the selected ranges
    void		setUseSelDataRanges(bool yn) { useseldataranges_ = yn; }
    void		setSelDataRanges(Interval<double>,Interval<double>);
    const Interval<double>& getSelDataRange(bool forx) const
			{ return forx ? xseldatarange_ : yseldatarange_; }
    static int		bitMapZVal()			{ return 0; }
    static int		auxDataZVal()			{ return 100; }
    static int		annotZVal()			{ return 200; }

protected:

    uiGraphicsView*		view_;
    FlatView::AxesDrawer& 	axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;
				/*!<May be reversed if display is reversed. */
    uiGraphicsItemGroup*	worldgroup_;
    uiRect			extraborders_;

    uiBitMapDisplay*		bitmapdisp_;

    Threads::Work		annotwork_;
    Threads::Work		bitmapwork_;
    Threads::Work		auxdatawork_;

    void			updateCB(CallBacker*);
    void			updateAnnotCB(CallBacker*);
    void			updateBitmapCB(CallBacker*);
    void			updateAuxDataCB(CallBacker*);

    uiWorldRect			getBoundingBox(bool wva) const;
    void			reSizeCB(CallBacker*);

    int				updatequeueid_;
    float			extfac_;

    void			updateTransforms();

    uiFlatViewControl*		control_;
    friend class		uiFlatViewControl;

    Interval<double>		xseldatarange_;
    Interval<double>		yseldatarange_;
    bool			useseldataranges_;

    ObjectSet<FlatView::uiAuxDataDisplay>	auxdata_;

};

#endif

