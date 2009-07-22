#ifndef visfaceset_h
#define visfaceset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaceset.h,v 1.4 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________


-*/

#include "visshape.h"

namespace visBase
{

/*!\brief
Implementation of an Indexed face set. 
A shape is formed by setting the coord index to a sequence of positions, ended
by a -1. The shape will automaticly connect the first and the last position
in the sequence. Multiple face sets can be set by adding new sequences after
the first one.
*/


mClass FaceSet : public IndexedShape
{
public:
    static FaceSet*	create()
			mCreateDataObj( FaceSet );
};

};

#endif
