#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uigroup.h"
#include "ranges.h"

class uiSpinBoxBody;
class uiLabel;


mExpClass(uiBase) uiSpinBox : public uiObject
{
friend class		uiSpinBoxBody;

public:
                        uiSpinBox(uiParent*, int nrdecimals=0,
				  const char* nm="SpinBox");
			~uiSpinBox();

    void		setNrDecimals(int);
    void		setAlpha(bool);
    bool		isAlpha() const;

    void		setSpecialValueText(const char*); // First entry

    void		setValue(int);
    void		setValue(float);
    void		setValue(double d)	{ setValue( ((float)d) ); }
    void		setValue(const char*); 
    int			getValue() const;
    float		getFValue() const;
    const char*		text() const;

    void		setInterval( int start, int stop, int s=1 )
			{ setInterval( StepInterval<int>(start,stop,s) ); }
    void		setInterval( const Interval<int>& i, int s=1 )
			{ setInterval( StepInterval<int>(i.start,i.stop,s) ); }
    void		setInterval(const StepInterval<int>&);
    StepInterval<int>	getInterval() const;

    void		setInterval( float start, float stop, float s=1 )
			{ setInterval(StepInterval<float>(start,stop,s)); }
    void		setInterval(const StepInterval<float>&);
    StepInterval<float> getFInterval() const;

    void		setInterval( double start, double stop, double s=1 )
			{ setInterval(StepInterval<double>(start,stop,s)); }
    void		setInterval(const StepInterval<double>&);

    void		setMinValue(int);
    void		setMinValue(float);
    void		setMinValue( double d )	{ setMinValue( (float)d ); }
    int			minValue() const;
    float		minFValue() const;

    void		setMaxValue(int);
    void		setMaxValue(float);
    void		setMaxValue( double d )	{ setMaxValue( (float)d ); }
    int			maxValue() const;
    float		maxFValue() const;

    void		setStep(int,bool snap_cur_value=false);
    void		setStep(float,bool snap_cur_value=false);
    void		setStep( double d, bool scv=false )
    			{ setStep( ((float)d),scv ); }
    float		fstep() const;
    int			step() const;

    void		stepBy( int nrsteps );

    const char*		prefix() const;
    void		setPrefix(const char*);
    const char*		suffix() const;
    void		setSuffix(const char*);

    void		doSnap( bool yn )		{ dosnap_ = yn; }

    void		setKeyboardTracking(bool);
    bool		keyboardTracking() const;
    void		setFocusChangeTrigger(bool);
    bool		focusChangeTrigger() const;

    bool		handleLongTabletPress();
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

    Notifier<uiSpinBox>	valueChanged;
    Notifier<uiSpinBox>	valueChanging;

    void		notifyHandler(bool editingfinished);

private:

    float		oldvalue_;

    uiSpinBoxBody*	body_;
    uiSpinBoxBody&	mkbody(uiParent*, const char*);

    bool		dosnap_; /*!< If true, value in spinbox will be snapped 
				  to a value equal to N*step. */
    bool		focuschgtrigger_;
    void		snapToStep(CallBacker*);
};


mExpClass(uiBase) uiLabeledSpinBox : public uiGroup
{
public:
                	uiLabeledSpinBox(uiParent*,const char* txt,
					 int nrdecimals=0,const char* nm=0);

    uiSpinBox*  	box()			{ return sb; }
    const uiSpinBox*  	box() const		{ return sb; }
    uiLabel*    	label()			{ return lbl; }

protected:

    uiSpinBox*		sb;
    uiLabel*    	lbl;
};

#endif

