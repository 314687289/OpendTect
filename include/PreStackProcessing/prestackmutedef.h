#ifndef prestackmutedef_h
#define prestackmutedef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.h,v 1.5 2009-07-22 16:01:17 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "position.h"
#include "samplingdata.h"

class PointBasedMathFunction;


namespace PreStack
{

mClass MuteDef : public NamedObject
{
public:

					MuteDef(const char* nm=0);
					MuteDef(const MuteDef&);
					~MuteDef();
    MuteDef&				operator=(const MuteDef&);

    int					size() const;
    int					indexOf(const BinID&) const;
    PointBasedMathFunction&		getFn(int idx);
    BinID&				getPos(int idx);
    const PointBasedMathFunction&	getFn(int idx) const;
    const BinID&			getPos(int idx) const;

    void				add(PointBasedMathFunction*,
	    				    const BinID& pos );
    					//!<Function becomes mine.
    void				remove(int idx);
    float				value(float offs,const BinID&) const;
					//!< Interpolates between defined
					//!< positions
    bool				isChanged() const { return ischanged_; }
    void				setChanged(bool yn) { ischanged_=yn; }

protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
    bool				ischanged_;
};


}; //namespace

#endif
