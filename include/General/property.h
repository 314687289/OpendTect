#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.5 2004-11-25 17:23:03 bert Exp $
________________________________________________________________________


-*/

#include "repos.h"
#include "enums.h"
#include "uidobj.h"
#include "bufstringset.h"

class PropertyRefRepository;
PropertyRefRepository& PrRR();


/*!\brief Ref Data for a (usually petrophysical) property */

class PropertyRef : public ::UserIDObject
{
public:

    enum StdType	{ Other, Time, Dist, Den, Vel, Son, AI, Por, Perm,
			  Sat, GR, ElPot, Res, PR, Comp, Temp, Pres };
			DeclareEnumUtils(StdType)

			PropertyRef( const char* nm=0, StdType t=Other,
				     bool h=false )
			: UserIDObject(nm)
			, stdtype_(t)
			, hcaff_(h)
			, source_(Repos::Survey)	{}

    BufferStringSet&	specialUnitsOfMeasure()		{ return units_; }
    bool		hcAffected() const		{ return hcaff_; }
    void		setHCAffected( bool yn )	{ hcaff_ = yn; }
    StdType		stdType() const			{ return stdtype_; }
    void		setStdType( StdType t ) 	{ stdtype_ = t; }
    Repos::Source	source() const			{ return source_; }
    void		setSource( Repos::Source s )	{ source_ = s; }

protected:

    StdType		stdtype_;
    BufferStringSet	units_;
    bool		hcaff_;
    Repos::Source	source_;

    friend class	PropertyRefRepository;
};


/*!\brief A (usually petrophysical) property of some object */

class Property
{
public:

    			Property(const PropertyRef* pr )
			: ref_(pr)		{}
    virtual		~Property()		{}

    virtual float	value() const		= 0;
    const PropertyRef*	ref() const		{ return ref_; }

    virtual bool	dependsOn(const Property*) const { return false; }

protected:

    const PropertyRef*	ref_;

};


/*\brief The repository of all PropertyRefs in OpendTect

  The singleton instance can be accessed through the global PrRR() function.

  The list of properties is filled from:


  */

class PropertyRefRepository
{
public:

    const PropertyRef* get(const char* nm) const;
    			//!< Can be null

    const ObjectSet<const PropertyRef>& all() const	{ return entries; }

    bool		set(const PropertyRef&);
    			//!< returns whether it has added.
    			//!< if not (i.e. pr with ref exists), it updates.
    bool		write(Repos::Source) const;

private:

    			PropertyRefRepository();

    ObjectSet<const PropertyRef> entries;

    void		addFromFile(const Repos::FileProvider&);

    friend PropertyRefRepository& PrRR();

};


#endif
