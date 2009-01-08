#ifndef uicolortable_h
#define uicolortable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id: uicolortable.h,v 1.14 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "flatview.h"

class uiColorTableCanvas;
class uiComboBox;
class uiLineEdit;

namespace ColTab { class Sequence; class MapperSetup; }

mClass uiColorTable : public uiGroup
{
public:

			uiColorTable(uiParent*,const ColTab::Sequence& colseq,
				     bool vertical);
			   //!< Editable
			uiColorTable(uiParent*,const char*,bool vertical );
			   //!< Display only
			~uiColorTable();

    const ColTab::Sequence&	colTabSeq() const	{ return coltabseq_;}
    const ColTab::MapperSetup&	colTabMapperSetup() const { return mapsetup_; }

    void		setSequence(const char*,bool emitnotif=true);
    void		setSequence(const ColTab::Sequence*,bool allowedit,
	    			    bool emitnotif=true);
    			/*!If ptr is null, sequence edit will be disabled. */
    void		setMapperSetup(const ColTab::MapperSetup*,
	    			       bool emitnotif=true);
    			/*!If ptr is null, mapper edit will be disabled. */
    void		setHistogram(const TypeSet<float>*);
    void		setInterval(const Interval<float>&);

    void                setEnabManage( bool yn )	{ enabmanage_ = yn; }

    void		setDispPars(const FlatView::DataDispPars::VD&);
    void		getDispPars(FlatView::DataDispPars::VD&) const;

    Notifier<uiColorTable>	seqChanged;
    Notifier<uiColorTable>	scaleChanged;

protected:

    bool		enabmanage_;

    ColTab::MapperSetup& mapsetup_;
    ColTab::Sequence&	coltabseq_;

    TypeSet<float>	histogram_;
    uiColorTableCanvas*	canvas_;
    uiLineEdit*		minfld_;
    uiLineEdit*		maxfld_;
    uiComboBox*		selfld_;

    void		updateRgFld();
    void		canvasreDraw(CallBacker*);
    void		canvasClick(CallBacker*);
    void		tabSel(CallBacker*);
    void		tableAdded(CallBacker*);
    void		rangeEntered(CallBacker*);
    void		doManage(CallBacker*);
    void		doFlip(CallBacker*);
    void		setAsDefault(CallBacker*);
    void		editScaling(CallBacker*);
    void		makeSymmetrical(CallBacker*);
    void		colTabChgdCB(CallBacker*);
    void		colTabManChgd(CallBacker*);

    bool		isEditable() const	{ return maxfld_; }
    void		fillTabList();
};


#endif
