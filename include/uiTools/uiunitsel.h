#ifndef uiunitsel_h
#define uiunitsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
 RCS:           $Id: uiunitsel.h,v 1.1 2010-02-23 10:14:19 cvsraman Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "property.h"

class uiComboBox;
class UnitOfMeasure;


mClass uiUnitSel : public uiGroup
{
public:
    				uiUnitSel(uiParent*,PropertyRef::StdType,
					  const char* lbltxt=0);

    void			setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;

    void			setPropType(PropertyRef::StdType);

protected:

    PropertyRef::StdType	proptype_;
    ObjectSet<const UnitOfMeasure>	units_;

    uiComboBox*			inpfld_;

    void			update();
};

#endif
