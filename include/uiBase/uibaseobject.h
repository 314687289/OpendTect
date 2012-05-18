#ifndef uibaseobject_h
#define uibaseobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uibaseobject.h,v 1.8 2012-05-18 12:13:43 cvskris Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

class uiBody;
class QWidget;

mClass uiBaseObject : public NamedObject
{
public:
				uiBaseObject(const char* nm, uiBody* = 0);
    virtual			~uiBaseObject();

				// implementation: uiobj.cc
    void			finalise();
    bool			finalised() const;
    void			clear();

    inline const uiBody*	body() const		{ return body_; }
    inline uiBody*		body()			{ return body_; }

    static void			setCmdRecorder(const CallBack&);
    static void			unsetCmdRecorder();

    int	 /* refnr */		beginCmdRecEvent(const char* msg=0);
    void			endCmdRecEvent(int refnr,const char* msg=0);

    int	 /* refnr */		beginCmdRecEvent(od_uint64 id,
	    					 const char* msg=0);
    void			endCmdRecEvent(od_uint64 id,int refnr,
					       const char* msg=0);

    Notifier<uiBaseObject>	tobeDeleted;
				//!< triggered in destructor

    virtual Notifier<uiBaseObject>& preFinalise()
				{ return finaliseStart; }
    virtual Notifier<uiBaseObject>& postFinalise()
				{ return finaliseDone; }
    
    
    virtual QWidget*		getWidget() { return 0; }
    const QWidget*		getWidget() const;

protected:

    void			setBody( uiBody* b )	{ body_ = b; }

    Notifier<uiBaseObject>	finaliseStart;
				//!< triggered when about to start finalising
    Notifier<uiBaseObject>	finaliseDone;
    				//!< triggered when finalising finished

private:
    static CallBack*		cmdrecorder_;
    int				cmdrecrefnr_;
    uiBody*			body_;
};


/*
CmdRecorder annotation to distinguish real user actions from actions 
performed by program code. Should be used at start of each (non-const)
uiObject function that calls any uiBody/Qt function that may trigger a
signal received by the corresponding Messenger class (see i_q****.h).
Apart from a few notify handler functions, it will do no harm when 
using this annotation unnecessarily.
*/

#define mBlockCmdRec		CmdRecStopper cmdrecstopper(this);

mClass CmdRecStopper
{
public:
    				CmdRecStopper(const uiBaseObject*);
				~CmdRecStopper();
};


#endif
