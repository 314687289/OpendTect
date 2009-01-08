#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.15 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "seistype.h"
class IOObj;
class Scaler;
class uiScaler;
class uiGenInput;
class uiSeisFmtScaleComp;


mClass uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*,Seis::GeomType,
				       bool forexport=true);
    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    int			getFormat() const;
    			//!< returns (int)DataCharacteristics::UserType
    bool		horOptim() const;
    bool		extendTrcToSI() const;
    void		updateIOObj(IOObj*,bool commit=true) const;

    bool		isSteering() const	{ return issteer_; }
    void		setSteering(bool);

protected:

    uiSeisFmtScaleComp*	compfld_;
    uiScaler*		scalefld_;

    Seis::GeomType	gt_;
    bool		issteer_;

    void		updSteer(CallBacker*);
    friend class	uiSeisFmtScaleComp;

};


#endif
