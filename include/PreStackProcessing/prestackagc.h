#ifndef prestackagc_h
#define prestackagc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "ranges.h"
#include "prestackprocessor.h"

namespace PreStack
{

/*!
\brief Processor for PreStack %AGC
*/

mExpClass(PreStackProcessing) AGC : public Processor
{
public:
    				mDefaultFactoryInstantiation(
					Processor, AGC,
					"AGC", sFactoryKeyword() )

				AGC();
    bool			prepareWork();

    void			setWindow(const Interval<float>&);
    const Interval<float>&	getWindow() const;
    void			getWindowUnit(BufferString&,
	    				      bool withparens) const;

    void			setLowEnergyMute(float fraction);
    float			getLowEnergyMute() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    static const char*		sKeyWindow()		{ return "Window"; }
    static const char*		sKeyMuteFraction()	{return "Mutefraction";}

protected:
    bool			doWork(od_int64,od_int64,int);
    od_int64			totalNr() const { return totalnr_; }

    Interval<float>		window_;
    Interval<int>		samplewindow_;
    float			mutefraction_;
    int				totalnr_;
};

} // namespace PreStack

#endif
