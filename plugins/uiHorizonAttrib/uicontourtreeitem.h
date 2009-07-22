#ifndef uicontourtreeitem_h
#define uicontourtreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
 RCS:           $Id: uicontourtreeitem.h,v 1.4 2009-07-22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"

#include "cubesampling.h"
#include "color.h"
#include "externalattrib.h"

class BinID;
class visContourLabels;

template <class T> class Array2DImpl;

namespace Attrib { class SelSpec; }
namespace visSurvey { class SurveyObject; }
namespace visBase
{
    class DrawStyle;
    class IndexedPolyLine;
    class Material;
    class Text2;
}

/*!\brief Tree item for Contour display on 3D horizons */

class uiContourTreeItem : public uiODDataTreeItem
{
public:
    static void			initClass();
				uiContourTreeItem(const char* parenttype);
				~uiContourTreeItem();

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			setupChangeCB(CallBacker*);

protected:

    bool			hasTransparencyMenu() const { return false; }
    void			prepareForShutdown();
    bool			init();

    void			removeAll();
    void			removeLabels();
    void			checkCB(CallBacker*);
    void			propChangeCB(CallBacker*);
    void			visClosingCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    void			computeContours();
    void			updateColumnText(int);
    void			createLines();
    void			addText(const Coord3&,const char*);

    Array2DImpl<int>*		arr_;
    TypeSet<BinID>		bids_;
    Interval<float>		rg_;
    SamplingData<int>		contoursampling_;

    visBase::IndexedPolyLine*	lines_;
    visBase::DrawStyle*		drawstyle_;
    visBase::Material*		material_;
    ObjectSet<visBase::Text2>	labels_;
    visContourLabels*		labelgrp_;
    
    Color			color_;
    int				linewidth_;
    MenuItem			optionsmenuitem_;

    BufferString		createDisplayName() const;
};

#endif
