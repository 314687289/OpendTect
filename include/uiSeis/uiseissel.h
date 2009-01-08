#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.34 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "uicompoundparsel.h"
#include "seistype.h"

class uiSeisIOObjInfo;
class uiListBox;

mClass uiSeisSel : public uiIOObjSel
{
public:

    struct Setup
    {
			Setup( Seis::GeomType gt )
			    : geom_(gt)
			    , selattr_(gt==Seis::Line)
			    , withclear_(false)
			    , seltxt_(0)
			    , datatype_(0)
			    , allowcnstrsabsent_(false)
			    , include_(true)	{}
			Setup( bool is2d, bool isps )
			    : geom_(Seis::geomTypeOf(is2d,isps))
			    , selattr_(is2d && !isps)
			    , withclear_(false)
			    , seltxt_(0)
			    , datatype_(0)
			    , allowcnstrsabsent_(false)
			    , include_(true)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,selattr)
	mDefSetupMemb(bool,withclear)
	mDefSetupMemb(const char*,seltxt)
	mDefSetupMemb(const char*,datatype)
	mDefSetupMemb(bool,allowcnstrsabsent)
	mDefSetupMemb(bool,include)
    };

			uiSeisSel(uiParent*,CtxtIOObj&,const Setup&);
			~uiSeisSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    inline Seis::GeomType geomType() const { return setup_.geom_; }
    inline bool		is2D() const	{ return Seis::is2D(setup_.geom_); }
    inline bool		isPS() const	{ return Seis::isPS(setup_.geom_); }

    void		setAttrNm(const char*);
    const char*		attrNm() const	{ return attrnm.buf(); }
    virtual void	processInput();
    virtual bool	existingTyped() const;
    virtual void	updateInput();

    static CtxtIOObj*	mkCtxtIOObj(Seis::GeomType);
    				//!< returns new default CtxtIOObj

protected:

    Setup		setup_;
    BufferString	orgkeyvals;
    BufferString	attrnm;
    mutable BufferString curusrnm;
    IOPar&		dlgiopar;

    virtual void	newSelection(uiIOObjRetDlg*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();
};


mClass uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const uiSeisSel::Setup&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(Seis::GeomType,bool);

protected:

    uiListBox*		attrlistfld_;
    uiGenInput*		attrfld_;
    bool		allowcnstrsabsent_;	//2D only
    const char*		datatype_;		//2D only
    bool		include_;		//2D only, datatype_ companion

    void		entrySel(CallBacker*);
    void 		attrNmSel(CallBacker*);
    const char*		getDataType();
};


mClass uiSeisLineSel : public uiCompoundParSel
{
public:

    			uiSeisLineSel(uiParent*,const char* lsnm=0);

    const char*		lineName() const	{ return lnm_; }
    const char*		lineSetName() const	{ return lsnm_; }

protected:

    BufferString	lnm_;
    BufferString	lsnm_;
    bool		fixedlsname_;

    BufferString	getSummary() const;

    void		selPush(CallBacker*);
};


#endif
