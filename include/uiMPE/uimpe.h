#ifndef uimpe_h
#define uimpe_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "draw.h"
#include "emposid.h"
#include "emseedpicker.h"
#include "randcolor.h"

#include "uigroup.h"

class uiParent;

namespace Attrib { class DescSet; }


namespace MPE
{

class SectionTracker;

/*! Interface to track-setup groups. Implementations can be retrieved through
    MPE::uiSetupGroupFactory. */


mExpClass(uiMPE) uiSetupGroup : public uiGroup
{
public:
			uiSetupGroup(uiParent*,const char* helpref);
    virtual void	setSectionTracker(SectionTracker*)	{}
    virtual void	setAttribSet(const Attrib::DescSet*)	{}
    virtual void	setMode(const EMSeedPicker::SeedModeOrder) {}
    virtual int		getMode()				=0;
    virtual void	setColor(const Color&)			{}
    virtual const Color& getColor()				=0;
    virtual void	setMarkerStyle(const MarkerStyle3D&)	{}
    virtual const MarkerStyle3D& getMarkerStyle()		=0;
    virtual void	setAttribSelSpec(const Attrib::SelSpec*) {}
    virtual bool	isSameSelSpec(const Attrib::SelSpec*) const
			{ return true; }

    virtual NotifierAccess*	modeChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	propertyChangeNotifier()	{ return 0; }
    virtual NotifierAccess*	eventChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	similartyChangeNotifier()	{ return 0; }

    virtual bool	commitToTracker(bool& fieldchg) const   { return true; }
    virtual bool	commitToTracker() const;

    virtual void	showGroupOnTop(const char* grpnm)	{}

    BufferString	helpref_;
};


/*! Factory function that can produce a MPE::uiSetupGroup* given a
    uiParent* and an Attrib::DescSet*. */

typedef uiSetupGroup*(*uiSetupGrpCreationFunc)(uiParent*,const char* typestr,
					       const Attrib::DescSet*);

/*! Factory that is able to create MPE::uiSetupGroup* given a uiParent*,
    and an Attrib::DescSet*. Each class that wants to
    be able to procuce instances of itself must register itself with the
    addFactory startup. */

mExpClass(uiMPE) uiSetupGroupFactory
{
public:
    void		addFactory(uiSetupGrpCreationFunc f, const char* name);
    uiSetupGroup*	create(const char* nm,uiParent*,const char* typestr,
	    		       const Attrib::DescSet*);
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */

protected:
    BufferStringSet			names_;
    TypeSet<uiSetupGrpCreationFunc>	funcs;

};


/*!
\brief Holder class for MPE ui-factories.
  Is normally only retrieved by MPE::uiMPE().
*/


mExpClass(uiMPE) uiMPEEngine
{
public:
    uiSetupGroupFactory		setupgrpfact;
};



/*!
\brief Access function for an instance (and normally the only instance) of
  MPE::uiMPEEngine.
*/
mGlobal(uiMPE) uiMPEEngine& uiMPE();

} // namespace MPE

#endif

