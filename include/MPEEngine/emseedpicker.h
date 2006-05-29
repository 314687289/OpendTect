#ifndef emseedpicker_h
#define emseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emseedpicker.h,v 1.19 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "emtracker.h"
#include "position.h"
#include "sets.h"

namespace MPE
{

/*!
handles adding of seeds and retracking of events based on new seeds.

An instance of the class is usually avaiable from the each EMTracker.
*/

class EMSeedPicker: public CallBacker
{
public:
    virtual		~EMSeedPicker() {}


    virtual bool	canSetSectionID() const			{ return false;}
    virtual bool	setSectionID(const EM::SectionID&)	{ return false;}
				
    virtual EM::SectionID getSectionID() const			{ return -1; }

    virtual bool	startSeedPick()				{ return false;}
    			/*!<Should be set when seedpicking
			    is about to start. */
    virtual bool	stopSeedPick(bool iscancel=false)	{ return true; }

    virtual bool	canAddSeed() const			{ return false;}
    virtual bool	addSeed(const Coord3&)			{ return false;}
    virtual bool	canRemoveSeed() const			{ return false;}
    virtual bool	removeSeed(const EM::PosID&,
	    			   bool retrack=true)		{ return false;}
    virtual bool	reTrack()				{ return false;}
    virtual int		nrSeeds() const				{ return 0; }
    virtual int		minSeedsToLeaveInitStage() const	{ return 1; }

    virtual NotifierAccess* seedChangeNotifier()		{ return 0; }
    
    virtual void	setSeedConnectMode(int)			{ return; }
    virtual int		getSeedConnectMode() const		{ return -1; }
    virtual void	blockSeedPick(bool)			{ return; }
    virtual bool	isSeedPickBlocked() const		{ return false;}
    virtual bool        doesModeUseVolume() const		{ return true; }
    virtual bool	doesModeUseSetup() const		{ return true;}

    virtual const char*	errMsg() const				{ return 0; }

};


};

#endif
