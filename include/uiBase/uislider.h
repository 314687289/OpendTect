#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.h,v 1.10 2004-03-02 13:29:34 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiobj.h"

class uiSliderBody;
class uiLabel;
class uiLineEdit;
template <class T> class StepInterval;

class uiSlider : public uiObject
{
public:

                        uiSlider(uiParent*,const char* nm="Slider",
				 int nrdec=0,bool log=false);

    enum 		TickSetting { NoMarks=0, Above=1, Left=Above, Below=2, 
				      Right=Below, Both=3 };
    enum		Orientation { Horizontal, Vertical };

    void		setText(const char*);
    const char*		text() const;

    void		setValue(float);
    int 		getIntValue() const;
    float 		getValue() const;

    void		setMinValue(float);
    float		minValue() const;
    void		setMaxValue(float);
    float		maxValue() const;
    void		setStep(float);
    float		step() const;
    void		setInterval(const StepInterval<float>&);
    void		getInterval(StepInterval<float>&) const;

    void		setTickMarks(TickSetting);
    TickSetting		tickMarks() const;
    void		setTickStep(int);
    int			tickStep() const;
    void		setOrientation(Orientation);
    uiSlider::Orientation getOrientation() const;

    bool		isLogScale()			{ return logscale; }

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;

private:

    mutable BufferString result;
    int			factor;
    bool		logscale;

    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*,const char*);

    void		sliderMove(CallBacker*);

    float		userValue(int) const;
    int			sliderValue(float) const;
};


class uiSliderExtra : public uiGroup
{
public:

    class Setup
    {
    public:
			Setup(const char* l=0)
			    : lbl_(l)
			    , withedit_(false)
			    , nrdec_(0)
			    , logscale_(false)
			    {}
				
	#define mSetVar(var,val) var=val; return *this
	Setup&		withedit(bool yn=true)	{ mSetVar(withedit_,yn); }
	Setup&		nrdec(int dec)		{ mSetVar(nrdec_,dec); }
	Setup&		logscale(bool yn=true)	{ mSetVar(logscale_,yn); }

	const char*	lbl_;
	bool		withedit_;
	int		nrdec_;
	bool		logscale_;
    };

                	uiSliderExtra(uiParent*,const Setup&,
				      const char* nm="SliderImpl");
			uiSliderExtra(uiParent*,const char* lbl=0,
				      const char* nm="SliderImpl");

    uiSlider*		sldr()			{ return slider; }
    uiLabel*		label()			{ return lbl; }

    void		processInput();

protected:

    uiSlider*		slider;
    uiLabel*    	lbl;
    uiLineEdit*		editfld;

    void		init(const Setup&,const char*);
    void		editRetPress(CallBacker*);
    void		sliderMove(CallBacker*);
};


#endif
