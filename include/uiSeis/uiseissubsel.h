#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "seisselection.h"
#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"
#include "sets.h"

class IOObj;
class HorSampling;
class CubeSampling;

class uiCompoundParSel;
class uiCheckBox;
class uiLineSel;
class uiPosSubSel;
class uiSeis2DSubSel;
class uiSelSubline;
class uiSeis2DMultiLineSel;
class uiSeis2DLineNameSel;


mExpClass(uiSeis) uiSeisSubSel : public uiGroup
{
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel()					{}

    bool		isAll() const;
    void		getSampling(CubeSampling&) const;
    void		getSampling(HorSampling&) const;
    void		getZRange(StepInterval<float>&) const;

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    virtual void	clear();
    virtual void	setInput(const IOObj&)				= 0;
    void		setInput(const HorSampling&);
    void		setInput(const MultiID&);
    void		setInput(const StepInterval<float>& zrg);
    void		setInput(const CubeSampling&);

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    virtual uiCompoundParSel*	compoundParSel();

protected:

    			uiSeisSubSel(uiParent*,const Seis::SelSetup&);

    uiPosSubSel*	selfld_;

};


mExpClass(uiSeis) uiSeis3DSubSel : public uiSeisSubSel
{
public:

    			uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
			    : uiSeisSubSel(p,ss)		{}

    void		setInput(const IOObj&);

};


mExpClass(uiSeis) uiSeis2DSubSel : public uiSeisSubSel
{
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    virtual void	clear();
    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setInput(const IOObj&);

    Notifier<uiSeis2DSubSel> lineSel;
    bool		isSingLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    void		selectedGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		selectedLines(BufferStringSet&) const;
    void		setSelectedLines(const BufferStringSet&);

    void		getSampling(CubeSampling&,int lidx=-1) const;
    StepInterval<int>	getTrcRange(int lidx=-1) const;
    StepInterval<float> getZRange(int lidx=-1) const;

protected:

    uiSeis2DMultiLineSel*	multilnmsel_;
    uiSeis2DLineNameSel*	singlelnmsel_;

    bool		multiln_;
    MultiID		inpkey_;

    void		lineChg(CallBacker*);
};

#endif

