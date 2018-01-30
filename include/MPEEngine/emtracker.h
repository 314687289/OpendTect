#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "factory.h"
#include "emposid.h"
#include "refcount.h"
#include "sets.h"
#include "trckeyzsampling.h"
#include "trckeyvalue.h"
#include "uistring.h"

class Executor;

namespace Geometry { class Element; }
namespace EM { class EMObject; }
namespace Attrib { class SelSpec; }

namespace MPE
{

class EMSeedPicker;
class SectionTracker;

/*!
\brief Tracks EM objects.
*/

mExpClass(MPEEngine) EMTracker : public RefCount::Referenced
{ mODTextTranslationClass(EMTracker);
public:
				EMTracker(EM::EMObject*);

    BufferString		objectName() const;
    EM::EMObject*		emObject()		{ return emobject_; }
    DBKey			objectID() const;

    virtual bool		is2D() const		{ return false; }

    void			setTypeStr(const char* type)
				{ type_ = type; }
    const char*			getTypeStr() const	{ return type_; }

    virtual bool		isEnabled() const	{ return isenabled_; }
    virtual void		enable(bool yn)		{ isenabled_=yn; }

    virtual bool		snapPositions(const TypeSet<TrcKey>&);

    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;
    void			getNeededAttribs(
					TypeSet<Attrib::SelSpec>&) const;

    virtual SectionTracker*	createSectionTracker() = 0;
    SectionTracker*		cloneSectionTracker();
    SectionTracker*		getSectionTracker(bool create=false);
    virtual EMSeedPicker*	getSeedPicker(bool create=true)
				{ return 0; }

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
				~EMTracker();

    bool			isenabled_;
    SectionTracker*		sectiontracker_;
    BufferString		errmsg_;
    const char*			type_;

    void			setEMObject(EM::EMObject*);

    static const char*		setupidStr()	{ return "SetupID"; }
    static const char*		sectionidStr()	{ return "SectionID"; }

private:
    EM::EMObject*		emobject_;
};


mDefineFactory1Param( MPEEngine, EMTracker, EM::EMObject*, TrackerFactory );


} // namespace MPE
