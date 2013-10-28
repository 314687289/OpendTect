#ifndef ui2dmultilinesel_h
#define ui2dmultilinesel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Sep 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "bufstring.h"
#include "bufstringset.h"
#include "objectset.h"
#include "posinfo2dsurv.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uiiomod.h"

class iopar;
class uiComboBox;
class uiCheckBox;
class uiLabeledListBox;
class uiListBox;
class uiSelLineSet;
class uiSelNrRange;
class uiSelZRange;

class LineInfo;
class ui2DMultiLineSelDlg;
class uiLineSetDlg;
class uiLineSetFld;

mExpClass(uiIo) LineInfo
{
public:
				LineInfo(const PosInfo::Line2DKey& id,
					 const StepInterval<int>& trcrange,
					 const StepInterval<float>& zrange) 
				: geomid_(id)
				, trcrange_(trcrange)
				, zrange_(zrange)
				, issel_(false)			{}
    
    bool			issel_;
    PosInfo::Line2DKey		geomid_;
    StepInterval<float>		zrange_;
    StepInterval<int>		trcrange_;
    StepInterval<float>		selzrange_;
    StepInterval<int>		seltrcrange_;

};


mExpClass(uiIo) ui2DMultiLineSel : public uiCompoundParSel
{
public:

    struct Setup
    {

				Setup( const char* txt=0,bool withls=true,
					bool withtrcrg=true,
					bool allowmultils=false, 
					bool allowmultiline=false,
					bool withzrg=true )
				: lbltxt_(txt)
				, withlinesetsel_(withls)
				, withtrcrg_(withtrcrg)
				, withz_(withzrg)
				, allowmultilineset_(allowmultils)
				, allowmultiline_(allowmultiline)				{}

	mDefSetupMemb(BufferString,lbltxt)
	mDefSetupMemb(bool,withlinesetsel)
	mDefSetupMemb(bool,withtrcrg)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(bool,allowmultilineset)
	mDefSetupMemb(bool,allowmultiline)

    };


				ui2DMultiLineSel(uiParent*, Setup);
				~ui2DMultiLineSel();

    

    BufferString		getSummary() const;
    BufferStringSet		getSelLines() const;
    const char*			getLineSet();

    bool			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected:
    TypeSet<PosInfo::Line2DKey>	geomids_;
    ObjectSet<LineInfo>		lineinfo_;
    ui2DMultiLineSelDlg*	dlg_;
    Setup			setup_;

    void			openDlg(CallBacker*);
    
};


mExpClass(uiIo) ui2DMultiLineSelDlg : public uiDialog
{
public:

				ui2DMultiLineSelDlg(uiParent*,
						const ui2DMultiLineSel::Setup&);
				~ui2DMultiLineSelDlg();

    void			createDlg();
    bool			fillDlg();
    BufferString		getSummary() const;
    ObjectSet<LineInfo>&	getLineInfo() { return lineinfo_; }
    TypeSet<PosInfo::Line2DKey>&	getSelLinesGeomIds() { return selgeomids_; }

    bool			isZRangeForAll() const;

    bool			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected:

    void			getSelLines(TypeSet<PosInfo::Line2DKey>&) const;
    int				lineID(const PosInfo::Line2DKey&)const;
    void			fillLineInfo();

    void			fillUIForMultiLine();
    void			fillUIForSingleLine();
    void			fillUIForMultiLineSet();
    void			fillUIForSingleLineSet();
    void			fillUIFortraceRange();
    void			fillUIForzRange();

    const ui2DMultiLineSel::Setup&  setup_;

    BufferString		previouslnm_;
    BufferString		previouslsnm_;
    TypeSet<PosInfo::Line2DKey>	selgeomids_;

    uiLabeledListBox*		llb_;
    uiLabeledListBox*		llb2_;
    uiListBox*			lnmsfld_;
    uiComboBox*			lnmfld_;
    uiComboBox*			lsnmfld_;
    uiCheckBox*			zrange_;
    uiListBox*			lsnmsfld_;

    uiSelNrRange*		trcrgfld_;
    uiSelZRange*		zrgfld_;

    ObjectSet<LineInfo>		lineinfo_;

    bool			acceptOK(CallBacker*);

    void			lineSel(CallBacker*);
    void			multiLineSel(CallBacker*);
    void			multiLineSetCheck(CallBacker*);
    void			multiLineSetSel(CallBacker*);
    void			lineSetSel(CallBacker*);
    void			trcRgZrgChanged();
};


#endif
