/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include <uiprogressbar.h>
#include <uiobjbody.h> 

#ifdef USEQT4
# include	<q3progressbar.h> 
# define	mQProgressBar	Q3ProgressBar
#else
# include	<qprogressbar.h> 
# define	mQProgressBar	QProgressBar
#endif


class uiProgressBarBody : public uiObjBodyImpl<uiProgressBar,mQProgressBar>
{
public:

                        uiProgressBarBody( uiProgressBar& handle, 
					   uiParent* parnt, const char* nm )
			    : uiObjBodyImpl<uiProgressBar,mQProgressBar>
				( handle, parnt,nm)
			    { 
				setStretch( 1, 0 );
				setHSzPol( uiObject::MedVar );
				setCenterIndicator( true );
			    }

    virtual int 	nrTxtLines() const				{ return 1; }

};

uiProgressBar::uiProgressBar( uiParent* p, const char* txt, 
			      int totalSteps, int progress )
    : uiObject(p,txt,mkbody(p,txt))
{
    setProgress( progress );
    setTotalSteps( totalSteps );
}

uiProgressBarBody& uiProgressBar::mkbody(uiParent* p, const char* txt)
{
    body_= new uiProgressBarBody(*this,p,txt);
    return *body_; 
}

void uiProgressBar::setProgress( int progr )
{ 
    body_->setProgress( progr );
} 


int uiProgressBar::Progress() const
{
    return body_->progress();
}


void uiProgressBar::setTotalSteps( int tstp )
{ 
    body_->setTotalSteps( tstp > 2 ? tstp : 2 );
} 


int uiProgressBar::totalSteps() const
{
    return body_->totalSteps();
}
