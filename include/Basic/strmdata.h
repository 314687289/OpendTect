#ifndef strmdata_h
#define strmdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.15 2012-08-03 13:00:15 cvskris Exp $
________________________________________________________________________

-*/
 
#include "basicmod.h"
#include "gendefs.h"
#include <stdio.h>
#include <iosfwd>


/*!\brief holds data to use and close an iostream.

Usualyy created by StreamProvider.
Need to find out what to do with the pipe in windows.

*/

mClass(Basic) StreamData
{
public:

		StreamData() : fname_(0)	{ initStrms(); }
		~StreamData()			{ delete [] fname_; }
		StreamData( const StreamData& sd )
		: fname_(0)			{ copyFrom( sd ); }
    StreamData&	operator =(const StreamData&);
    void	transferTo(StreamData&);	//!< retains fileName()

    void	close();
    bool	usable() const;

    void	setFileName(const char*);
    const char*	fileName() const	{ return fname_; }
    						//!< Beware: may be NULL

    FILE*	filePtr() const		{ return const_cast<FILE*>(fp_); }

    std::istream* istrm;
    std::ostream* ostrm;

protected:

    FILE*	fp_;
    bool	ispipe_;
    char*	fname_;
    void	copyFrom(const StreamData&);

private:

    inline void	initStrms() { istrm = 0; ostrm = 0; fp_ = 0; ispipe_ = false; }
    friend class StreamProvider;

};


#endif

