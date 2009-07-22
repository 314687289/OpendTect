#ifndef prestackeventtransl_h
#define prestackeventtransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id: prestackeventtransl.h,v 1.4 2009-07-22 16:01:17 cvsbert Exp $
________________________________________________________________________


-*/

#include "transl.h"

class Executor;
class IOObj;
class BinIDValueSet;
class HorSampling;

namespace PreStack { class EventManager; }


mClass PSEventTranslatorGroup : public TranslatorGroup
{ isTranslatorGroup(PSEvent);
public:
    				mDefEmptyTranslatorGroupConstructor(PSEvent);
    const char*			defExtension() const { return sDefExtension(); }
    static const char*		sDefExtension()	     { return "psevent"; }
    static const char*		sKeyword()	     { return "PreStack Event";}
};


mClass PSEventTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PSEvent);
    virtual Executor*	createReader(PreStack::EventManager&,
	    			     const BinIDValueSet*,
				     const HorSampling*,IOObj*,
				     bool trigger)	= 0;
    virtual Executor*	createWriter(PreStack::EventManager&,IOObj*) = 0;
    virtual Executor*	createSaveAs(PreStack::EventManager&,IOObj*)	= 0;
    virtual Executor*	createOptimizer(IOObj*)				= 0;

    static Executor*	reader(PreStack::EventManager&, const BinIDValueSet*,
				     const HorSampling*, IOObj*, bool trigger );
    static Executor*	writer(PreStack::EventManager&,IOObj*);
    static Executor*	writeAs(PreStack::EventManager&,IOObj*);
};


mClass dgbPSEventTranslator : public PSEventTranslator
{ isTranslator(dgb,PSEvent)
public:
    		mDefEmptyTranslatorConstructor(dgb,PSEvent);
    Executor*	createReader(PreStack::EventManager&,
	    			     const BinIDValueSet*,
				     const HorSampling*,IOObj*,bool);
    Executor*	createWriter(PreStack::EventManager&,IOObj*);
    Executor*	createSaveAs(PreStack::EventManager&,IOObj*);
    Executor*	createOptimizer(IOObj*) { return 0; }

    bool	implRemove(const IOObj*);
};


#endif
