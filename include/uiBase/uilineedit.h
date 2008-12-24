#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.19 2008-12-24 05:52:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "userinputobj.h"

class uiLineEditBody;

class uiIntValidator
{
public:
    		uiIntValidator()
		    : bottom_(-mUdf(int)), top_(mUdf(int))	{}
    		uiIntValidator( int bot, int top )
		    : bottom_(bot), top_(top)			{}

    int		bottom_;
    int		top_;
};


class uiFloatValidator
{
public:
    		uiFloatValidator()
		    : bottom_(-mUdf(float)), top_(mUdf(float))
		    , nrdecimals_(1000), scnotation_(true)	{}
    		uiFloatValidator( float bot, float top )
		    : bottom_(bot), top_(top)
		    , nrdecimals_(1000), scnotation_(true)	{}

    float	bottom_;
    float	top_;
    int		nrdecimals_;
    bool	scnotation_;	// If true, ScientificNotation is used
};


class uiLineEdit : public UserInputObjImpl<const char*>, public uiObject
{
public:
			//! pref_empty : return empty string/ null value
			//  insted of undefined value, when line edit is empty.
                        uiLineEdit(uiParent*,const char* nm);
                        uiLineEdit(uiParent*,const DataInpSpec&,const char* nm);

    void		setEdited(bool=true);
    bool		isEdited() const;

    virtual void	setReadOnly(bool=true);
    virtual bool	isReadOnly() const;
    virtual bool	update_(const DataInpSpec&);

    void		setPasswordMode();
    void		setValidator(const uiIntValidator&);
    void		setValidator(const uiFloatValidator&);

    void		setMaxLength(int);
    int			maxLength() const;
    void		setNrDecimals( int nrdec )	{ nrdecimals_ = nrdec; }

			//! Moves the text cursor to the beginning of the line. 
    void		home();
			//! Moves the text cursor to the end of the line. 
    void		end();

    Notifier<uiLineEdit> editingFinished;	
    Notifier<uiLineEdit> returnPressed;	
    Notifier<uiLineEdit> textChanged;	

    			//! Force activation in GUI thread
    void                activate(const char* txt=0,bool enter=true);
    Notifier<uiLineEdit> activatedone;

    virtual const char*	getvalue_() const;
    virtual void	setvalue_( const char* );

protected:
    
    virtual bool	notifyValueChanging_( const CallBack& cb )
			    { textChanged.notify(cb); return true;}
    virtual bool	notifyValueChanged_( const CallBack& cb ) 
			    { editingFinished.notify(cb); return true;}

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*);

    mutable BufferString result_;
    int			nrdecimals_;
};
#endif
