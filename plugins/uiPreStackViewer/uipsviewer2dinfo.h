#ifndef uipsviewer2dinfo_h
#define uipsviewer2dinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: uipsviewer2dinfo.h,v 1.1 2011-05-04 15:20:02 cvsbruno Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "uigroup.h"

class uiLabel;
class BinID;

namespace PreStackView
{

mClass uiGatherDisplayInfoHeader : public uiGroup
{
public:
    				uiGatherDisplayInfoHeader(uiParent*);
    				~uiGatherDisplayInfoHeader() {}

    void			setData(const BinID&,bool inl,const char* data);
    void			setOffsetRange(const Interval<float>&);

protected:
    uiLabel*			datalbl_;
    uiLabel*			poslbl_;
};


}; //namespace

#endif
