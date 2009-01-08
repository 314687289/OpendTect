#ifndef uivalseriestracker_h
#define uivalseriestracker_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2005
 RCS:           $Id: uivalseriestracker.h,v 1.5 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "iopar.h"
#include "samplingdata.h"

class EventTracker;
class uiGenInput;

/*!User interface for EventTracker. */


mClass uiEventTracker : public uiDlgGroup
{
public:
    			uiEventTracker(uiParent*,EventTracker&,
				bool hideeventtype=false,
				bool immediateupdate=false);

    bool		acceptOK();
    bool		rejectOK();
protected:
    void			selEventType(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			selUseSimilarity(CallBacker*);
    void			changeCB(CallBacker*);
    bool			updateTracker(bool domsg);

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiGenInput*			alloweddifffld_;

    uiGenInput*			usesimifld_;
    uiGenInput*			simithresholdfld_;
    uiGenInput*			compwinfld_;

    bool			immediateupdate_;

    EventTracker&		tracker_;
    IOPar			restorepars_;
};
			       
#endif
