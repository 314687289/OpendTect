#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellimpasc.h,v 1.18 2006-10-06 11:48:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "strmdata.h"
#include "bufstringset.h"

#include <iosfwd>

class UnitOfMeasure;


namespace Well
{
class Data;

class AscImporter
{
public:

			AscImporter( Data& d ) : wd(d), useconvs_(false) {}
			~AscImporter();

    const char*		getTrack(const char*,bool first_is_surface,
	    			 bool depthinfeet);
    const char*		getD2T(const char*,bool istvd,bool istwt,
	    		       bool depthinfeet);
    const char*		getMarkers(const char*,bool istvd,
	    			   bool depthinfeet);
    Data&		getWellData() 				{ return wd; }

    class LasFileInfo
    {
    public:
			LasFileInfo()
			    : zrg(mUdf(float),mUdf(float))
			    , depthcolnr(-1)
			    , undefval(-999.25)	{}
			~LasFileInfo()		{ deepErase(lognms); }

	BufferStringSet	lognms;
	Interval<float>	zrg;
	int		depthcolnr;
	float		undefval;
	BufferString	zunitstr;

	BufferString	wellnm; //!< only info; not used by getLogs
    };

    const char*		getLogInfo(const char* lasfnm,LasFileInfo&) const;
    const char*		getLogInfo(std::istream& lasstrm,LasFileInfo&) const;
    const char*		getLogs(const char* lasfnm,const LasFileInfo&,
	    			bool istvd=true);
    const char*		getLogs(std::istream& lasstrm,const LasFileInfo&,
	    			bool istvd=true);

    bool		willConvertToSI() const		{ return useconvs_; }
    			//!< Note that depth is always converted
    void		setConvertToSI( bool yn )	{ useconvs_ = yn; }
    			//!< Note that depth is always converted

protected:

    Data&		wd;

    mutable BufferStringSet	unitmeasstrs_;
    mutable ObjectSet<const UnitOfMeasure>	convs_;
    bool		useconvs_;

    void		parseHeader(char*,char*&,char*&,char*&) const;
    const char*		getLogData(std::istream&,const BoolTypeSet&,
	    			   const LasFileInfo&,bool,int,int);

};

}; // namespace Well

#endif
