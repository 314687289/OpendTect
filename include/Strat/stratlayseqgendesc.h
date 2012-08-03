#ifndef stratlayseqgendesc_h
#define stratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratlayseqgendesc.h,v 1.17 2012-08-03 13:00:43 cvskris Exp $
________________________________________________________________________


-*/

#include "stratmod.h"
#include "objectset.h"
#include "multiid.h"
#include "propertyref.h"
class IOPar;

namespace Strat
{
class RefTree;
class LayerSequence;
class LayerGenerator;


/*!\brief Collection of LayerGenerator's that can generate a full LayerSequence.

  The 'modpos' that generate() wants needs to be  0 for the first, and 1 for
  the last model to be generated (linear in between). For one model only,
  specify 0.5.

 */

mClass(Strat) LayerSequenceGenDesc : public ObjectSet<LayerGenerator>
{
public:
			LayerSequenceGenDesc(const RefTree&);
			~LayerSequenceGenDesc();

    const PropertyRefSelection& propSelection() const	{ return propsel_; }
    void		setPropSelection(const PropertyRefSelection&);

    const MultiID& 	elasticPropSel() const;
    void		setElasticPropSel(const MultiID&);

    bool		getFrom(std::istream&);
    bool		putTo(std::ostream&) const;

    bool		prepareGenerate() const;
    bool		generate(LayerSequence&,float modpos) const;

    const char*		errMsg() const			{ return errmsg_; }

    const char*		userIdentification(int) const;
    int			indexFromUserIdentification(const char*) const;

    const RefTree&	refTree() const			{ return rt_; }

protected:

    const RefTree&		rt_;
    PropertyRefSelection	propsel_;
    MultiID			elasticpropselmid_;

    mutable BufferString	errmsg_;

};


}; // namespace Strat

#endif

