#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.17 2011-11-14 07:39:14 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "bufstringset.h"
#include "ioobj.h"

#include "uidialog.h"

class DataPointSet;
class DataPointSetDisplayMgr;
class uiPosProvider;
class uiPosFilterSetSel;
class uiSeis2DLineNameSel;
class uiListBox;
namespace Attrib { class DescSet; class SelInfo; }


mClass uiAttribCrossPlot : public uiDialog
{
public:
					uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
					~uiAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    void				setDisplayMgr(
					    DataPointSetDisplayMgr* dispmgr )
					{ dpsdispmgr_ = dispmgr; }

    const DataPointSet&			getDPS() const;

protected:

    const Attrib::DescSet& 		ads_;
    Attrib::SelInfo*			attrinfo_;
    TypeSet<int>			selidxs_;
    TypeSet<MultiID>			selids_;
    TypeSet<BufferStringSet>		linenmsset_;

    uiListBox*				attrsfld_;
    uiPosProvider*			posprovfld_;
    uiPosFilterSetSel*			posfiltfld_;
    uiListBox*				lnmfld_;
    DataPointSet*			curdps_;
    DataPointSetDisplayMgr*		dpsdispmgr_;

    void				adsChg();
    MultiID				getSelectedID() const;
    void				initWin(CallBacker*);
    void				lineChecked(CallBacker*);
    void				attrChecked(CallBacker*);
    void				attrChanged(CallBacker*);

    bool				acceptOK(CallBacker*);
};


#endif
