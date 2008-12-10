#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.3 2008-12-10 10:05:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"
#include "ranges.h"

class IOPar;

namespace Well
{

/*!\brief Display properties of a well */

class DisplayProperties
{
public:

			DisplayProperties()		{}

    struct BasicProps
    {
			BasicProps( int sz=1 )
			    : size_(sz)			{}

	Color		color_;
	int		size_;

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	virtual const char* subjectName() const		= 0;

    protected:

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}

    };

    struct Track : public BasicProps
    {
			Track()
			    : dispabove_(true)
			    , dispbelow_(false)		{}

	virtual const char* subjectName() const		{ return "Track"; }

	bool		dispabove_;
	bool		dispbelow_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    struct Markers : public BasicProps
    {

			Markers()
			    : BasicProps(5)
			    , circular_(true)	{}

	virtual const char* subjectName() const	{ return "Markers"; }
	bool		circular_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    struct Log : public BasicProps
    {

			Log()
			    : seismicstyle_(true)	
			    , cliprate_(mUdf(float))
			    , range_(mUdf(float),mUdf(float))
		            , iscliprate_(false)
			    , logarithmic_(false)
			    , repeat_(1)
			    , repeatovlap_(mUdf(float))
			    , linecolor_(Color::White)
			    , logfill_(false)
		            , logfillcolor_(Color::White)
			    , seqname_("AI")
			    , singlfillcol_(false)
							{}

	BufferString	name_;
	bool		seismicstyle_;

	float               cliprate_;      //!< If undef, use range_
	Interval<float>     range_;         //!< If cliprate_ set, filled using it
	bool                logarithmic_;
	bool                iscliprate_;
	bool                logfill_;
	int                 repeat_;
	float               repeatovlap_;
	Color               linecolor_;
	Color               logfillcolor_;
	const char*         seqname_;
	bool                singlfillcol_;

    

	virtual const char* subjectName() const	{ return "Log"; }

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    Track		track_;
    Markers		markers_;
    Log			left_;
    Log			right_;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;


    static DisplayProperties&	defaults();
    static void			commitDefaults();

};


} // namespace

#endif
