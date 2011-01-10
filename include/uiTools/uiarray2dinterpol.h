#ifndef uiarray2dinterpol_h
#define uiarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id: uiarray2dinterpol.h,v 1.10 2011-01-10 10:20:57 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "factory.h"

class Array2DInterpol;
class InverseDistanceArray2DInterpol;
class uiCheckBox;
class uiGenInput;
class uiArray2DInterpol;


mClass uiArray2DInterpolSel : public uiDlgGroup
{
public:
    mDefineFactory1ParamInClass(uiArray2DInterpol,uiParent*,factory);
    				uiArray2DInterpolSel(uiParent*,bool filltype,
				bool holesz, bool withclassification, 
				const Array2DInterpol* oldvals);

    bool			acceptOK();
    Array2DInterpol*		getResult();
    				//!<\note Becomes caller's
				
    void			setDistanceUnit(const char*);
    				//!<A unitstring in [] that tells what the
				//!<unit is for going from one cell to another
    
    const char*			helpID() const;
    void			fillPar(IOPar&) const;

protected:
					~uiArray2DInterpolSel();
    uiParent*				getTopObject();
    void				selChangeCB(CallBacker*);

    Array2DInterpol*			result_;

    uiGenInput*				filltypefld_;
    uiGenInput*				maxholeszfld_;
    uiGenInput*				methodsel_;
    uiGenInput*				isclassificationfld_;

    ObjectSet<uiArray2DInterpol>	params_;
};


mClass uiArray2DInterpol : public uiDlgGroup
{
public:
    virtual void	setValuesFrom(const Array2DInterpol&)		{}
    			//*!Dose only work if provided object is of 'your' type.

    bool		acceptOK()					= 0;
    virtual void	setDistanceUnit(const char*)			{}

    Array2DInterpol*	getResult();
			//!<Becomes caller's


protected:
    			uiArray2DInterpol(uiParent*,const char* nm);
    			~uiArray2DInterpol();
    Array2DInterpol*	result_;
};


mClass uiInverseDistanceArray2DInterpol : public uiArray2DInterpol
{
public:

    				uiInverseDistanceArray2DInterpol(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			setValuesFrom(const Array2DInterpol&);
    bool			acceptOK();
    void			setDistanceUnit(const char*);

    const char*			helpID() const { return "104.0.13"; }

protected:

    uiGenInput*			radiusfld_;
    uiButton*			parbut_;

    bool			cornersfirst_;
    int				stepsz_;
    int				nrsteps_;

    void			useRadiusCB(CallBacker*);
    void			doParamDlg(CallBacker*);

    friend class		uiInvDistA2DInterpolPars;

};


mClass uiTriangulationArray2DInterpol : public uiArray2DInterpol
{
public:

    				uiTriangulationArray2DInterpol(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    bool			acceptOK();
    void			setDistanceUnit(const char*);

    const char*			helpID() const { return "104.0.13"; }

protected:

    void			intCB(CallBacker*);
    uiGenInput*			maxdistfld_;
    uiCheckBox*			interpolatefld_;
};


mClass uiArray2DInterpolExtension : public uiArray2DInterpol
{
public:

    				uiArray2DInterpolExtension(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    bool			acceptOK();

    const char*			helpID() const { return "104.0.13"; }

protected:

    uiGenInput*                 nrstepsfld_;
};
#endif
