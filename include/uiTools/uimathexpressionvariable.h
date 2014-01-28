#ifndef uimathexpressionvariable_h
#define uimathexpressionvariable_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Huck
 Date:		Mar 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
class BufferStringSet;
class MathExpression;
class uiGenInput;
class uiLabeledComboBox;
class UnitOfMeasure;


mExpClass(uiTools) uiMathExpressionVariable : public uiGroup
{
public:

				uiMathExpressionVariable(uiGroup*,
						const BufferStringSet&,
						int curselidx=0,
						bool displayuom=true);

    virtual void		use(const MathExpression*);
    bool                        hasVarName(const char*) const;
    BufferString		getVarName() const;

    const char*			getInput() const;
    virtual void		setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;
    float			getCstVal() const;
    bool			isCst() const;
    void			setCurSelIdx(int);

protected:

    void			selChg(CallBacker*);

    const int			idx_;			//needed?
    uiLabeledComboBox*	inpfld_;
    uiLabeledComboBox*	unfld_;
    uiGenInput* cstvalfld_;
    const BufferStringSet&      posinpnms_;
    BufferString		varnm_;
};


#endif

