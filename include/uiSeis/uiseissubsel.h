#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.23 2008-07-17 10:12:10 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "bufstringset.h"
#include "seisselection.h"
#include "uidialog.h"
#include "ranges.h"
class IOPar;
class IOObj;
class CtxtIOObj;
class uiGenInput;
class HorSampling;
class CubeSampling;
class uiListBox;
class uiPosSubSel;
class uiSeis2DSubSel;
class uiSeisSel;
class uiSelSubline;


class uiSeisSubSel : public uiGroup
{
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel()					{}

    bool		isAll() const;
    void		getSampling(HorSampling&) const;
    void		getZRange(StepInterval<float>&) const;

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    virtual void	clear();
    virtual void	setInput(const IOObj&)				= 0;
    void		setInput(const HorSampling&);
    void		setInput(const StepInterval<float>& zrg);
    void		setInput(const CubeSampling&);

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    virtual uiCompoundParSel*	compoundParSel();

protected:

    			uiSeisSubSel(uiParent*,const Seis::SelSetup&);

    uiPosSubSel*	selfld_;

};


class uiSeis3DSubSel : public uiSeisSubSel
{
public:

    			uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
			    : uiSeisSubSel(p,ss)		{}

    void		setInput(const IOObj&);

};


class uiSeis2DSubSel : public uiSeisSubSel
{ 	
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    virtual void	clear();
    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setInput(const IOObj&);
    void		setInputWithAttrib(const IOObj&,const char* attribnm);

    Notifier<uiSeis2DSubSel> lineSel;
    Notifier<uiSeis2DSubSel> singLineSel;
    bool		isSingLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    const BufferStringSet& curLineNames() const		{ return curlnms_; }

protected:

    uiGenInput*		lnmfld_;
    uiGenInput*		lnmsfld_;

    bool		multiln_;
    BufferStringSet&	curlnms_;

    TypeSet<StepInterval<int> >		trcrgs_;
    TypeSet<StepInterval<float> >	zrgs_;

    void		lineChg(CallBacker*);
    void		singLineChg(CallBacker*);

};


class uiSelection2DParSel : public uiCompoundParSel
{
public:
    				uiSelection2DParSel(uiParent*);
    				~uiSelection2DParSel();

    BufferString		getSummary() const;
    void			doDlg(CallBacker*);
    void			lineSetSel(CallBacker*);
    IOObj*			getIOObj();
    const BufferStringSet&	getSelLines() const	{ return sellines_; }

protected:
    int			        nroflines_;
    BufferStringSet 		sellines_;

    uiSeisSel*			linesetfld_;
    uiListBox*			lnmsfld_;
    CtxtIOObj*			lsctio_;
};

#endif
