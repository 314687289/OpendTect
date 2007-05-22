#ifndef horizon3dseedpicker_h
#define horizon3dseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dseedpicker.h,v 1.19 2007-05-22 04:08:58 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace MPE
{

class Horizon3DSeedPicker : public EMSeedPicker
{
public:
    			Horizon3DSeedPicker(MPE::EMTracker&);

    bool		canSetSectionID() const		{ return true; }
    bool		setSectionID(const EM::SectionID&);
    EM::SectionID	getSectionID() const		{ return sectionid_; }

    bool		startSeedPick();
    bool		stopSeedPick(bool iscancel=false);

    bool		addSeed(const Coord3&,bool drop);
    bool		removeSeed(const EM::PosID&,
	    			   bool environment,
	    			   bool retrack);
    bool		canAddSeed() const		{ return true; }
    bool		canRemoveSeed() const		{ return true; }
    bool		reTrack();
    int			nrSeeds() const;
    int			minSeedsToLeaveInitStage() const;

    NotifierAccess*	aboutToAddRmSeedNotifier()	{ return &addrmseed_; }
    NotifierAccess*	madeSurfChangeNotifier()	{ return &surfchange_; }
    
    enum SeedModeOrder	{ TrackFromSeeds, 
			  TrackBetweenSeeds, 
			  DrawBetweenSeeds   };

    static int		nrSeedConnectModes()		{ return 3; }
    static int		defaultSeedConMode()		{return TrackFromSeeds;}
    int			defaultSeedConMode(
				    bool gotsetup) const;
    static const char*	seedConModeText(int mode, 
				    bool abbrev=false); 

    int			getSeedConnectMode() const	{ return seedconmode_; }
    void		setSeedConnectMode(int scm)	{ seedconmode_ = scm; }
    void		blockSeedPick(bool yn)		{ blockpicking_ = yn; }
    bool		isSeedPickBlocked() const	{ return blockpicking_;}
    bool		doesModeUseVolume() const;
    bool		doesModeUseSetup() const;

protected:
    bool		retrackOnActiveLine( const BinID& startbid, 
					     bool startwasdefined,
					     bool eraseonly=false);
    bool		retrackFromSeedList();
    void		processJunctions();
    bool		lineTrackDirection(BinID& dir,
					   bool perptotrackdir=false) const;
    int 		nrLateralNeighbors(const EM::PosID& pid) const;
    int 		nrLineNeighbors(const EM::PosID& pid,
	    				bool perptotrackdir=false) const;

    bool		interpolateSeeds();
    CubeSampling	getTrackBox() const;

    TypeSet<EM::PosID>	propagatelist_;
    TypeSet<EM::PosID>	seedlist_;
    TypeSet<Coord3>	seedpos_;
    TypeSet<BinID>	trackbounds_;
    TypeSet<EM::PosID>	junctions_;
    TypeSet<EM::PosID>	eraselist_;

    bool		didchecksupport_;
    EM::SectionID	sectionid_;
    MPE::EMTracker&	tracker_;

    int			seedconmode_;
    bool		blockpicking_;

    Notifier<Horizon3DSeedPicker>	addrmseed_;
    Notifier<Horizon3DSeedPicker>	surfchange_;
private: 
    void		extendSeedListEraseInBetween( 
			    bool wholeline,const BinID& startbid,
			    bool startwasdefined,const BinID& dir );
};


};

#endif
