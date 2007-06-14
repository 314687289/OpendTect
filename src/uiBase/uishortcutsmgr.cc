/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
________________________________________________________________________

-*/


#include "uishortcutsmgr.h"
#include "ptrman.h"
#include "oddirs.h"
#include "settings.h"
#include "keyenum.h"
#include "keystrs.h"
#include "strmprov.h"
#include "ascstream.h"
#include "errh.h"
#include <qevent.h>

uiShortcutsMgr& SCMgr()
{
    static uiShortcutsMgr* scmgr = 0;
    if ( !scmgr ) scmgr = new uiShortcutsMgr;
    return *scmgr;
}


const char** uiKeyDesc::sKeyKeyStrs()
{
    static const char* strs[] =
{ "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S",
  "T","U","V","W","X","Y","Z","Plus","Minus","Asterisk","Slash","Up","Down",
  "Left","Right","Delete","PageUp","PageDown", 0 };
    return strs;
}


const int speckeystransbl[] =
{ 0x2b, 0x2d, 0x2a, 0x2f,
  0x1013, 0x1015, 0x1012, 0x1014, 0x1007, 0x1016, 0x1017 };


#define mQtbaseforA 0x41
#define mQtbasefor0 0x30


uiKeyDesc::uiKeyDesc( QKeyEvent* ev )
    : key_(ev->key())
    , state_((OD::ButtonState)ev->state())
{
    if ( state_ >= OD::Keypad )
	state_ = (OD::ButtonState)((int)state_ - (int)OD::Keypad);
}


uiKeyDesc::uiKeyDesc( const char* str1, const char* str2 )
    : state_(OD::NoButton)
    , key_(0)
{
    set( str1, str2 );
}


bool uiKeyDesc::set( const char* statestr, const char* keystr )
{
    if ( !statestr )
	statestr = "";

    if ( *statestr == 'S' || *statestr == 's' )
	state_ = OD::ShiftButton;
    if ( *statestr == 'C' || *statestr == 'c' )
	state_ = OD::ControlButton;
    // Not considering 'Alt' button because that's usually OS-related

    if ( !keystr || *keystr == '\0' )
	return false;

    if ( keystr[1] != '\0' )
    {
	const char** strs = sKeyKeyStrs();
	for ( int idx=26; strs[idx]; idx++ )
	{
	    if ( !strcmp( keystr, strs[idx] ) )
		{ key_ = speckeystransbl[idx-26]; break; }
	}
    }
    else
    {
	char charfound = *keystr;
	if ( (charfound >= 'a' && charfound <= 'z') )
	    key_ = mQtbaseforA + charfound - 'a';
	else if ( (charfound >= 'A' && charfound <= 'Z') )
	    key_ = mQtbaseforA + charfound - 'A';
	else if ( charfound >= '0' && charfound <= '9' )
	    key_ = mQtbasefor0 + charfound - '0';
    }

    return key_ != 0;
}


const char* uiKeyDesc::stateStr() const
{
    return OD::nameOf( state_ );
}


const char* uiKeyDesc::keyStr() const
{
    const char** strs = sKeyKeyStrs();

    if ( key_ >= mQtbaseforA && key_ < mQtbaseforA + 26 )
	return strs[ key_ - mQtbaseforA ];
    if ( key_ >= mQtbasefor0 && key_ < mQtbasefor0 + 10 )
	return strs[ key_ - mQtbasefor0 ];

    for ( int idx=26; ; idx++ )
    {
	if ( speckeystransbl[idx-26] == key_ )
	    return strs[idx];
    }
    return 0;
}

#define mDefQKeyEv \
    QKeyEvent qke( QEvent::KeyPress, key_, 0, (int)state_ )


bool uiKeyDesc::isSimpleAscii() const
{
    mDefQKeyEv;
    return state_== 0 && isascii( *(qke.text()) );
}


char uiKeyDesc::asciiChar() const
{
    if ( !isascii(key_) ) return 0;
    mDefQKeyEv;
    return qke.ascii();
}


uiShortcutsList::uiShortcutsList( const char* selkey )
	: selkey_(selkey)
{
    PtrMan<IOPar> pars = SCMgr().getStored( selkey );
    if ( !pars ) return;
    
    int index = 0;
    while ( true )
    {
	BufferString name;
	if ( !getSCNames( *pars, index, name ) )
	    return;
	
	BufferString val1, val2;
	if ( !getKeyValues( *pars, index, val1, val2 ) )
	    return;

	names_.add( name );
	keydescs_ += uiKeyDesc( val1, val2 );
	index++;
    }
}


uiShortcutsList& uiShortcutsList::operator =( const uiShortcutsList& scl )
{
    if ( this == &scl ) return *this;
    empty();
    selkey_ = scl.selkey_;
    keydescs_ = scl.keydescs_;
    names_ = scl.names_;
    return *this;
}


void uiShortcutsList::empty()
{
    keydescs_.erase(); names_.deepErase();
}


bool uiShortcutsList::write( bool usr ) const
{
    return SCMgr().setList( *this, usr );
}


void uiShortcutsList::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	BufferString basekey = IOPar::compKey(selkey_,idx);
	iop.set( IOPar::compKey(basekey,sKey::Name), names_.get(idx) );
	iop.set( IOPar::compKey(basekey,sKey::Keys),
			keydescs_[idx].stateStr(), keydescs_[idx].keyStr() );
    }
}


bool uiShortcutsList::getKeyValues( const IOPar& par, int scutidx,
				    BufferString& val1,
				    BufferString& val2 ) const
{
    BufferString scutidxstr = scutidx;
    BufferString key = IOPar::compKey( scutidxstr.buf(), sKey::Keys );
    return par.get( key.buf(), val1, val2 );
}


uiKeyDesc uiShortcutsList::keyDescOf( const char* nm ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( names_.get(idx) == nm )
	    return keydescs_[idx];
    }
    return uiKeyDesc(0,0);
}


const char* uiShortcutsList::nameOf( const uiKeyDesc& kd ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( keydescs_[idx] == kd )
	    return names_.get( idx );
    }
    return 0;
}


bool uiShortcutsList::getSCNames( const IOPar& par, int scutidx,
				  BufferString& name ) const
{
    BufferString scutidxstr = scutidx;
    BufferString key = IOPar::compKey( scutidxstr.buf(), sKey::Name );
    return par.get( key.buf(), name );
}


static const char* sFileKey = "shortcuts";

IOPar* uiShortcutsMgr::getStored( const char* subsel )
{
    Settings& setts = Settings::fetch( sFileKey );
    IOPar* ret = setts.subselect( subsel );
    if ( ret && ret->size() )
	return ret;
    delete ret;

    StreamData sd = StreamProvider(
	    		mGetSetupFileName("ShortCuts")).makeIStream();
    if ( !sd.usable() )
	{ sd.close(); return 0; }

    ascistream astrm( *sd.istrm );
    IOPar* iop = new IOPar( astrm );
    sd.close();
    if ( iop && iop->isEmpty() )
	{ delete iop; return 0; }

    ret = iop->subselect( subsel );
    delete iop;
    if ( ret && ret->isEmpty() )
	{ delete ret; return 0; }

    return ret;
}


const uiShortcutsList& uiShortcutsMgr::getList( const char* key ) const
{
    const int idx = keys_.indexOf( key );
    if ( idx >= 0 ) return *lists_[idx];

    uiShortcutsMgr& self = *(const_cast<uiShortcutsMgr*>(this));
    uiShortcutsList* newlist = new uiShortcutsList( key );
    self.keys_.add( key );
    self.lists_ += newlist;
    return *newlist;
}


bool uiShortcutsMgr::setList( const uiShortcutsList& scl, bool usr )
{
    if ( !usr )
    {
	pErrMsg( "Needs implementation" );
	return false;
    }

    IOPar iop; scl.fillPar( iop );
    Settings& setts = Settings::fetch( sFileKey );
    setts.merge( iop );
    if ( !setts.write(false) )
	return false;

    uiShortcutsList& myscl = const_cast<uiShortcutsList&>(getList(scl.selkey_));
    myscl = scl;
    return true;
}
