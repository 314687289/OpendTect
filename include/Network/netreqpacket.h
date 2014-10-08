#ifndef netreqpacket_h
#define netreqpacket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"
#include "bufstringset.h"

#define mRequestPacketHeaderSize		10


namespace Network
{

/*\brief Standardized packet that can be sent over a Tcp connection

  The request header contains a unique (int) request ID and a (short) sub-ID.
  The sub-ID is a flag that can be used to implement your protocol. Negative
  sub-IDs are special and cannot be freely used.

  Servers will not send Begin packets as they don't initiate requests. Replies
  are never Begin packets. Error always implies End.

  Request IDs are obtained by using setIsNewRequest().

  At the end of the class there is a public section for expert usage.

  */

mExpClass(Network) RequestPacket
{
public:
			RequestPacket(od_int32 payloadsize=-1);
			~RequestPacket();

    od_int32		requestID() const;
    bool		isNewRequest() const { return subID()==cBeginSubID(); }
    bool		isRequestEnd() const { return subID()<cMoreSubID(); }
    bool		isError() const	     { return subID()==cErrorSubID(); }
    od_int32		payloadSize() const;
    const void*		payload() const;
    void		getStringPayload(BufferString&) const;

    int			setIsNewRequest();   //!< conveniently returns reqID()
    void		setIsError()	     { setSubID( cErrorSubID() ); }
    void		setIsLast()	     { setSubID( cEndSubID() ); }
    void*		allocPayload(od_int32 size);
    void		setPayload(void*,od_int32 size); //!< buf becomes mine
    void		setStringPayload(const char*);

    void		addErrMsg(BufferString&) const;

protected:

    union Header
    {
	od_int32	int32s_[2];
	od_int16	int16s_[5]; //only number 4 is used
    };


    Header		header_;
    char*		payload_;

    static od_int16	cBeginSubID()		{ return -1; }
    static od_int16	cMoreSubID()		{ return -2; }
    static od_int16	cEndSubID()		{ return -4; }
    static od_int16	cErrorSubID()		{ return -8; }

    friend class	PacketFiller;
    friend class	PacketInterpreter;

public:

    bool		isOK() const;
			//!< checks whether the header is reasonable
    od_int16		subID() const;

    void		setRequestID(od_int32);
    void		setSubID(od_int16);

    static od_int32	headerSize() { return mRequestPacketHeaderSize; }
    static od_int32	getPayloadSize(const void*);

    void*		payload(bool takeover=false);
			    /*!< takeover: delete char[] */

    void*		getRawHeader()		{ return header_.int32s_; }
    const void*		getRawHeader() const	{ return header_.int32s_; }
};


/*\brief fills a packet's payload.

  Use the sizeFor() functions to calculate the total payload size.

*/

mExpClass(Network) PacketFiller
{
public:
			PacketFiller( RequestPacket& p, int startpos=0 )
			    : pkt_(p)
			    , curpos_(startpos)	{}

#   define		mNPFSizeFor(v) Network::PacketFiller::sizeFor(v)
#   define		mNPFSizeForArr(a,s) Network::PacketFiller::sizeFor(a,s)
    template <class T>
    static int		sizeFor(const T&);
    template <class T>
    static int		sizeFor(const T*,int nrelems);
    static int		sizeFor(const char*);
    static int		sizeFor(const OD::String&);
    static int		sizeFor(const BufferStringSet&);

    template <class T>
	void		put(const T&) const;
    template <class T>
	void		put(const T*,int nrelems) const;
    void		put(const char*) const;
    void		put(const OD::String&) const;
    void		put(const BufferStringSet&) const;

protected:

    RequestPacket&	pkt_;
    mutable int		curpos_;

};


/*\brief interprets what is in a packet.

  You need to know what should be in there. Arrays/sets are always
  preceeded by their size.

*/

mExpClass(Network) PacketInterpreter
{
public:
			PacketInterpreter( const RequestPacket& p,
					 int startpos=0 )
			    : pkt_(p)
			    , curpos_(startpos)	{}

    template <class T> void	get(T&) const;
    inline int			getInt() const;
    inline float		getFloat() const;
    inline double		getDouble() const;
    inline BufferString		getString() const;

    template <class T> void	getArr(T*,int maxsz) const;
    template <class T> void	getSet(TypeSet<T>&,int maxsz=-1) const;
    inline void			getSet(BufferStringSet&,int maxsz=-1) const;

    inline void			move( int nrb ) const	{ curpos_ += nrb; }
    inline void			moveTo( int pos ) const	{ curpos_ = pos; }

    template <class T> void	peek(T&) const;
    inline int			peekInt() const;

protected:

    const RequestPacket&	pkt_;
    mutable int			curpos_;

};


template <class T>
inline int PacketFiller::sizeFor( const T& var )
{
    return sizeof(T);
}

template <class T>
inline int PacketFiller::sizeFor( const T* arr, int nrelems )
{
    return sizeof(int) + nrelems * sizeof(T);
}

inline int PacketFiller::sizeFor( const char* str )
{
    return sizeFor( FixedString(str) );
}

inline int PacketFiller::sizeFor( const OD::String& str )
{
    return sizeof(int) + str.size();
}

inline int PacketFiller::sizeFor( const BufferStringSet& bss )
{
    int ret = sizeof(int);
    for ( int idx=0; idx<bss.size(); idx++ )
	ret += sizeFor( bss.get(idx) );
    return ret;
}


template <class T>
inline void PacketFiller::put( const T& var ) const
{
    OD::memCopy( pkt_.payload_+curpos_, &var, sizeof(T) );
    curpos_ += sizeof(T);
}

template <class T>
inline void PacketFiller::put( const T* arr, int nrelems ) const
{
    put( nrelems );
    if ( nrelems > 0 )
	OD::memCopy( pkt_.payload_+curpos_, arr, sizeof(T) );
    curpos_ += nrelems * sizeof(T);
}

inline void PacketFiller::put( const char* str ) const
{
    put( FixedString(str) );
}

inline void PacketFiller::put( const OD::String& str ) const
{
    const int sz = str.size();
    put( sz );
    OD::memCopy( pkt_.payload_+curpos_, str, sz );
    curpos_ += sz;
}

inline void PacketFiller::put( const BufferStringSet& bss ) const
{
    const int sz = bss.size();
    put( sz );
    for ( int idx=0; idx<sz; idx++ )
	put( bss.get(idx) );
}




template <class T>
inline void PacketInterpreter::peek( T& var ) const
{
    OD::memCopy( &var, pkt_.payload_+curpos_, sizeof(T) );
}


template <class T>
inline void PacketInterpreter::get( T& var ) const
{
    peek( var );
    curpos_ += sizeof(T);
}

template <>
inline void PacketInterpreter::get( BufferString& var ) const
{
    int sz; get( sz );
    if ( sz < 1 )
    {
	if ( sz < 0 )
	    { pErrMsg("Invalid string size"); }
	var.setEmpty();
	return;
    }
    var.setBufSize( sz+1 );
    char* cstr = var.getCStr();
    OD::memCopy( cstr, pkt_.payload_+curpos_, sz );
    cstr[sz] = '\0';
    curpos_ += sz;
}

inline int PacketInterpreter::peekInt() const
{ int ret = 0; peek(ret); return ret; }
inline int PacketInterpreter::getInt() const
{ int ret = 0; get(ret); return ret; }
inline float PacketInterpreter::getFloat() const
{ float ret = 0.f; get(ret); return ret; }
inline double PacketInterpreter::getDouble() const
{ double ret = 0.; get(ret); return ret; }
inline BufferString PacketInterpreter::getString() const
{ BufferString ret; get(ret); return ret; }

template <class T>
inline void PacketInterpreter::getArr( T* arr, int maxsz ) const
{
    int arrsz; get( arrsz );
    int sz = arrsz;
    if ( sz > maxsz )
	sz = maxsz;
    if ( arrsz < 1 )
	return;

    OD::memCopy( arr, pkt_.payload_+curpos_, sz*sizeof(T) );
    curpos_ += arrsz;
}

template <class T>
inline void PacketInterpreter::getSet( TypeSet<T>& ts, int maxsz) const
{
    int arrsz; get( arrsz );
    int sz = arrsz;
    if ( maxsz >= 0 && sz > maxsz )
	sz = maxsz;
    if ( arrsz < 1 )
	{ ts.setEmpty(); return; }

    ts.setSize( sz );
    OD::memCopy( ts.arr(), pkt_.payload_+curpos_, sz*sizeof(T) );
    curpos_ += arrsz;
}

inline void PacketInterpreter::getSet( BufferStringSet& bss,
						int maxsz ) const
{
    int setsz; get( setsz );
    int sz = setsz;
    if ( maxsz >= 0 && sz > maxsz )
	sz = maxsz;
    bss.setEmpty();
    if ( setsz < 1 )
	return;

    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString* bs = new BufferString;
	get( *bs );
	bss += bs;
    }

    for ( int idx=sz; idx<setsz; idx++ )
	getString();
}


}; //Namespace Network


#endif
