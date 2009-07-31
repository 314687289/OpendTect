#ifndef winstreambuf_h
#define winstreambuf_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id: winstreambuf.h,v 1.4 2009-07-31 08:06:27 cvsnanne Exp $
________________________________________________________________________

*/

#define private protected
#include <fstream>
#undef private

namespace std
{

/*!\brief Adapter to seek in filebuffers on win64.

  Usage like:

  std::winfilebuf fb;
  std::istream strm( &fb );

  After C:\Program Files (x86)\Microsoft Visual Studio 8\VC\include\fstream
  Only change is: feek -> _fseeki64

*/


class winfilebuf : public filebuf
{
public:
winfilebuf( const char* fnm, ios_base::openmode om )
{
    isok_ = open( fnm, om );
}

bool isOK() const	{ return isok_; }

protected:

#define defom (ios_base::openmode)(ios_base::in | ios_base::out)

virtual pos_type seekoff( off_type _Off, ios_base::seekdir _Way,
			  ios_base::openmode = defom )
{
    fpos_t _Fileposition;

    if ( _Mysb::gptr() == &_Mychar && _Way == ios_base::cur && _Pcvt == 0 )
	_Off -= (off_type)sizeof (char);

    if ( _Myfile==0 || !_Endwrite() || (_Off!=0 || _Way!=ios_base::cur) &&
	 _fseeki64(_Myfile,_Off,_Way) != 0 ||
	 fgetpos(_Myfile,&_Fileposition) != 0 )
	return (pos_type(_BADOFF));

    if ( _Mysb::gptr() == &_Mychar )
	_Mysb::setg( &_Mychar, &_Mychar+1, &_Mychar+1 );

    return (_POS_TYPE_FROM_STATE(pos_type,_State,_Fileposition));
}


virtual pos_type seekpos( pos_type _Pos, ios_base::openmode = defom )

{
    fpos_t _Fileposition = _POS_TYPE_TO_FPOS_T(_Pos);
    off_type _Off = (off_type)_Pos - _FPOSOFF(_Fileposition);

    if ( _Myfile == 0 || !_Endwrite()
	 || fsetpos(_Myfile, &_Fileposition) != 0
	 || _Off != 0 && _fseeki64(_Myfile, _Off, SEEK_CUR) != 0
	 || fgetpos(_Myfile, &_Fileposition) != 0)
	return (pos_type(_BADOFF));

    _State = _POS_TYPE_TO_STATE(_Pos);

    if ( _Mysb::gptr() == &_Mychar)
	 _Mysb::setg( &_Mychar, &_Mychar+1, &_Mychar+1 );

    return (_POS_TYPE_FROM_STATE(pos_type,_State,_Fileposition));
}

bool	isok_;

};


class winifstream : public istream
{
public:

winifstream( const char* fnm, ios_base::openmode om )
    : istream(0)
{
    fb_ = new winfilebuf( fnm, om );
    rdbuf( fb_ );

    if ( fb_->isOK() )
	clear();
    else
	setstate( ios_base::failbit );
}

~winifstream()
{
    if ( !fb_->close() )
	setstate( ios_base::failbit );
}

bool is_open()
{ return fb_->is_open(); }

    winfilebuf*	fb_;
};


class winofstream : public ostream
{
public:

winofstream( const char* fnm, ios_base::openmode om )
    : ostream(0)
{
    fb_ = new winfilebuf( fnm, om );
    rdbuf( fb_ );

    if ( fb_->isOK() )
	clear();
    else
	setstate( ios_base::failbit );
}

~winofstream()
{
    if ( !fb_->close() )
	setstate( ios_base::failbit );
}

bool is_open()
{ return fb_->is_open(); }

    winfilebuf*	fb_;
};


} // namespace std


#endif
