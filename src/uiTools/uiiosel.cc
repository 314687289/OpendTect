/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          start of 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiiosel.cc,v 1.50 2008-11-25 15:35:26 cvsbert Exp $";

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "iopar.h"
#include "filegen.h"
#include "keystrs.h"

IOPar& uiIOFileSelect::ixtablehistory =
			*new IOPar("IXTable selection history");
IOPar& uiIOFileSelect::devicehistory =
			*new IOPar("Device selection history");
IOPar& uiIOFileSelect::tmpstoragehistory =
			*new IOPar("Temporay storage selection history");


uiIOSelect::uiIOSelect( uiParent* p, const CallBack& butcb, const char* txt,
			bool withclear, const char* buttontxt, bool keepmytxt )
	: uiGroup(p)
	, doselcb_(butcb)
	, selectiondone(this)
	, specialitems(*new IOPar)
    	, keepmytxt_(keepmytxt)
{
    if ( withclear ) addSpecialItem( "" );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, txt, txt );
    inp_ = lcb->box(); lbl_ = lcb->label();
    inp_->setReadOnly( false );
    inp_->selectionChanged.notify( mCB(this,uiIOSelect,selDone) );
    lbl_->setAlignment( uiLabel::AlignRight );

    selbut_ = new uiPushButton( this, buttontxt, false );
    selbut_->setName( BufferString( buttontxt, " ",  txt ) );
    selbut_->activated.notify( mCB(this,uiIOSelect,doSel) );
    selbut_->attach( rightOf, lcb );

    setHAlignObj( lcb );
    setHCentreObj( lcb );
    mainObject()->finaliseStart.notify( mCB(this,uiIOSelect,doFinalise) );

}


uiIOSelect::~uiIOSelect()
{
    delete &specialitems;
    deepErase( entries_ );
}


void uiIOSelect::stretchHor( bool yn )
{
    inp_->setHSzPol( uiObject::MedMax );
}


void uiIOSelect::doFinalise()
{
    updateFromEntries();
}


void uiIOSelect::updateFromEntries()
{
    int curitnr = inp_->size() ? inp_->currentItem() : -1;
    BufferString curusrnm;
    if ( curitnr >= 0 )
	curusrnm = inp_->textOfItem( curitnr );

    if ( keepmytxt_ )
	curusrnm = inp_->text();

    inp_->empty();

    for ( int idx=0; idx<specialitems.size(); idx++ )
	inp_->addItem( specialitems.getValue(idx) );

    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const char* usrnm = userNameFromKey( *entries_[idx] );
	if ( usrnm )
	    inp_->addItem( usrnm );
	else
	{
	    delete entries_[idx];
	    entries_.remove( idx );
	    idx--;
	}
    }

    if ( curitnr >= 0 && inp_->size() )
	inp_->setCurrentItem( curusrnm );

    if ( keepmytxt_ )
	inp_->setText( curusrnm );
}


bool uiIOSelect::haveEntry( const char* key ) const
{
    if ( specialitems.find(key) ) return true;

    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( *entries_[idx] == key ) return true;
    return false;
}


void uiIOSelect::updateHistory( IOPar& iopar ) const
{
    // Find the last key already present
    int lastidx = 0;
    for ( ; ; lastidx++ )
    {
	if ( !iopar.find( IOPar::compKey(sKey::IOSelection,lastidx+1)) )
	    break;
    }

    // Add the entries
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const char* key = *entries_[idx];
	if ( iopar.findKeyFor(key) ) continue;

	lastidx++;
	iopar.set( IOPar::compKey(sKey::IOSelection,lastidx), key );
    }
}


void uiIOSelect::getHistory( const IOPar& iopar )
{
    checkState();
    bool haveold = inp_->size();
    bool havenew = false; BufferString bs;
    for ( int idx=1; ; idx++ )
    {
	if ( !iopar.get( IOPar::compKey(sKey::IOSelection,idx), bs ) )
	    break;

	const char* key = bs;
	if ( haveEntry(key) || !userNameFromKey(key) ) continue;

	havenew = true;
	entries_ += new BufferString( key );
    }

    if ( havenew )
    {
	updateFromEntries();
	if ( !haveold )
	    selDone(0);
    }
}


void uiIOSelect::addSpecialItem( const char* key, const char* value )
{
    if ( !value ) value = key;
    specialitems.set( key, value );
}


bool uiIOSelect::isEmpty() const
{
    const char* inp = getInput();
    return !inp || !*inp;
}
    


const char* uiIOSelect::getInput() const
{
    return inp_->text();
}


const char* uiIOSelect::getKey() const
{
    checkState();
    const_cast<uiIOSelect*>(this)->processInput();

    const int nrspec = specialitems.size();
    const int curit = getCurrentItem();
    if ( curit < 0 ) return "";
    if ( curit < nrspec ) return specialitems.getKey(curit);

    return entries_.size() ? (const char*)(*entries_[curit-nrspec]) : "";
}


void uiIOSelect::checkState() const
{
    if ( inp_->size() != specialitems.size() + entries_.size() )
	const_cast<uiIOSelect*>(this)->updateFromEntries();
}


void uiIOSelect::setInput( const char* key )
{
    checkState();

    if ( specialitems.find(key) )
    {
	inp_->setCurrentItem( specialitems.find(key) );
	return;
    }

    const char* usrnm = userNameFromKey( key );
    if ( !usrnm ) return;

    const int nrspec = specialitems.size();
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const int boxidx = idx + nrspec;
	if ( *entries_[idx] == key )
	{
	    inp_->setItemText( boxidx, usrnm );
	    inp_->setCurrentItem( boxidx );
	    return;
	}
    }

    entries_ += new BufferString( key );
    inp_->addItem( usrnm );
    inp_->setCurrentItem( nrspec + nrentries );
}


void uiIOSelect::setInputText( const char* txt )
{
    inp_->setText( txt );
}


int uiIOSelect::getCurrentItem() const
{
    checkState();
    if ( inp_->size() == 0 )
	return -1;

    const char* curtxt = inp_->text();
    if ( !inp_->isPresent(curtxt) )
	return -1;

    return inp_->currentItem();
}


void uiIOSelect::setCurrentItem( int idx )
{
    checkState();
    if ( idx >= 0 ) inp_->setCurrentItem( idx );
}


int uiIOSelect::nrItems() const
{
    return specialitems.size() + entries_.size();
}


const char* uiIOSelect::getItem( int idx ) const
{
    const int nrspec = specialitems.size();
    const int nrentries = entries_.size();
    return idx < nrspec
	 ? (idx < 0 ? "" : specialitems.getValue(idx))
	 : (idx < nrentries + nrspec ? (const char*)*entries_[idx-nrspec] : "");
}


void uiIOSelect::doSel( CallBacker* )
{
    processInput();
    selok_ = false;
    doselcb_.doCall( this );
    if ( selok_ )
    {
	updateFromEntries();
	selectiondone.trigger();
    }
}


void uiIOSelect::selDone( CallBacker* )
{
    objSel();
    selectiondone.trigger();
}


void uiIOSelect::empty( bool withclear )
{
    inp_->empty();

    if ( entries_.size() ) 
	entries_.erase();

    if ( withclear ) addSpecialItem( "" );
}


void uiIOSelect::setReadOnly( bool yn )
{
    inp_->setReadOnly( yn );
}


const char* uiIOSelect::labelText() const
{
    return lbl_->text();
}


void uiIOSelect::setLabelText( const char* s )
{
    lbl_->setPrefWidthInChar( strlen(s)+1 );
    return lbl_->setText( s );
}


uiIOFileSelect::uiIOFileSelect( uiParent* p, const char* txt, bool frrd,
				const char* inp, bool wclr )
	: uiIOSelect(p,mCB(this,uiIOFileSelect,doFileSel),txt,wclr)
	, forread(frrd)
	, seldir(false)
{
    if ( inp && *inp ) setInput( inp );
}


void uiIOFileSelect::doFileSel( CallBacker* c )
{
    BufferString caption( "Select " );
    caption += labelText();
    uiFileDialog fd( this, forread, getInput(),
		     filter.isEmpty() ? 0 : (const char*)filter, caption );
    if ( seldir )
	fd.setMode( uiFileDialog::DirectoryOnly );

    if ( fd.go() )
    {
	setInput( fd.fileName() );
	selDone( 0 );
    }
}


bool uiIOFileSelect::fillPar( IOPar& iopar ) const
{
    const char* res = getInput();
    iopar.set( "File name", res );
    return res && *res && File_exists(res);
}


void uiIOFileSelect::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "File name" );
    if ( res ) setInput( res );
}
