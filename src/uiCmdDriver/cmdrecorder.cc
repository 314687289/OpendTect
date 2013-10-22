/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "cmdrecorder.h"

#include "cmdcomposer.h"
#include "objectfinder.h"
#include "tablecommands.h"

#include <iostream>
#include <sstream>

#include "ascstream.h"
#include "oddirs.h"
#include "strmprov.h"
#include "timer.h"

#include "uigroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiseparator.h"
#include "uitoolbutton.h"


namespace CmdDrive
{


CmdRecorder::CmdRecorder( const uiMainWin& aw )
    	: rec_(*this)
	, applwin_(&aw)
	, outputfnm_(0)
	, outputsd_(*new StreamData)
	, recording_(false)
	, dynamicpopupmenu_(0)
	, openqdialog_(false)
	, lastobjsearched_(0)
	, ignorecmddriverwindows_(true)
	, writetailonly_(false)
	, bufstream_(*new std::ostringstream())
	, bufsize_(0)
{
    CmdComposer::initStandardComposers();
}


CmdRecorder::~CmdRecorder()
{
    if ( recording_ )
	stop();

    outputsd_.close();
    delete &outputsd_;
    delete &bufstream_;
}


void CmdRecorder::ignoreCmdDriverWindows( bool yn )
{ ignorecmddriverwindows_ = yn; }


bool CmdRecorder::mustSkip() const
{
    if ( !outputsd_.usable() )
	return true;

    if ( ignorecmddriverwindows_ && isCmdDriverWindow(winstack_.topWin()) )
	return true;

    outputcounter_++;
    return false;
}


bool CmdRecorder::start()
{
    outputsd_ = StreamProvider(outputfnm_).makeOStream( false );
    if ( !outputfnm_ || !outputsd_.usable() )
	return false;

    ascostream astrm( *outputsd_.ostrm );
    if ( !astrm.putHeader("OpendTect commands") )
	return false;

    uiBaseObject::addCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiAction::addCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiMenu::addInterceptor( mCB(this,CmdRecorder,dynamicMenuInterceptor) );
    Timer::setUserWaitFlag( false );

    dynamicpopupmenu_ = 0;
    openqdialog_ = false;

    lastobjsearched_ = 0;
    lastobjfreewins_.erase();

    winassertion_.setEmpty();
    winassertcasedep_ = false;

    popuprefnrs_.erase();
    composers_.erase();
    winstack_.synchronize();

    recording_ = true;
    nrparskipped_ = 0;
    outputcounter_ = 0;

    return true;
}


void CmdRecorder::stop( bool fatal )
{
    if ( !recording_ )
	return;

    const bool oldwritetailonly = writetailonly_;

    if ( fatal )
    {
	recording_ = false;
	flush();
	recording_ = true;
	writetailonly_ = false;

	CmdRecEvent dummy;
	for ( int idx=composers_.size()-1; idx>=0; idx-- )
	{
	    if ( !composers_[idx]->greedy() )
		composers_[idx]->accept( dummy );
	}

	dummy.srcwin_ = winstack_.topWin( ignorecmddriverwindows_ );
	dummy.openqdlg_ = openqdialog_;
	dummy.qdlgtitle_ = uiMainWin::activeModalQDlgTitle();
	insertWinAssertion( dummy );

	mRecOutStrm << std::endl << std::endl
		    << "###  OpendTect terminated unexpectedly: "
		    << Time::getDateTimeString() << std::endl << std::endl;

	const int curcounter = outputcounter_;

	for ( int idx=composers_.size()-1; idx>=0; idx-- )
	{
	    CmdComposer* comp = composers_[idx];
	    if ( comp->greedy() && comp->tryToFinish() && !comp->done() )
		comp->accept( dummy ); 
	}

	if ( curcounter != outputcounter_ )
	{
	    mRecOutStrm << std::endl << "###  Command line(s) above were "
			<< "under construction at that time!" << std::endl;
	}
    }
    else if ( ignorecmddriverwindows_ )
    {
	ignorecmddriverwindows_ = false;
	CmdRecEvent dummy;
	dummy.srcwin_ = winstack_.topWin( true );
	insertWinAssertion( dummy );
	ignorecmddriverwindows_ = true;
    }

    uiBaseObject::removeCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiAction::removeCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiMenu::removeInterceptor(
	    			mCB(this,CmdRecorder,dynamicMenuInterceptor) ); 

    recording_ = false;
    flush();
    writetailonly_ = oldwritetailonly;
}


void CmdRecorder::dynamicMenuInterceptor( CallBacker* cb )
{
    mDynamicCast( uiMenu*, dynamicpopupmenu_, cb );
    if ( dynamicpopupmenu_ )
	dynamicpopupmenu_->doIntercept( false, 0 ); 
}


static void takeSimilarObjs( ObjectSet<const CallBacker>& objects,
			     const char* srckey, bool tofront, bool toback )
{
    int nrobjstodo = objects.size();
    int offset = 0;
    int idx = 0;

    while ( nrobjstodo )
    {
	const CallBacker* newobj = objects[idx];
	const BufferString objkey = CmdComposer::factoryKey( newobj );
	bool yn = srckey && objkey==srckey;

	if ( FixedString(srckey) == "UILINEEDIT" )
	    yn = yn || objkey=="UISPINBOX" || objkey=="UICOMBOBOX";

	if ( FixedString(srckey) == "UILABEL" )
	    yn = yn || dynamic_cast<const uiLabel*>(newobj);
	if ( FixedString(srckey) == "UIGROUPOBJ" )
	    yn = yn || dynamic_cast<const uiGroupObj*>(newobj);
	if ( FixedString(srckey) == "UISEPARATOR" )
	    yn = yn || dynamic_cast<const uiSeparator*>(newobj);

	if ( tofront!=yn && toback!=yn )
	    objects.removeSingle( idx );
	else if ( toback && yn )
	    objects += objects.removeSingle( idx );
	else if ( tofront && yn )
	    objects.insertAt( objects.removeSingle(idx++), offset++ );
	else
	    idx++;

	nrobjstodo--;
    }
}


static bool deepFindKeyStr( const uiMainWin&, CmdRecEvent&, const uiObject* );

#define mFindAllNodes( objfinder, localenv, objsfound ) \
\
    ObjectSet<const CallBacker> objsfound; \
    if ( localenv ) \
	objfinder.findNodes( localenv, &objsfound ); \
    else \
	objfinder.findNodes( ObjectFinder::Everything, &objsfound ); \
    objfinder.deleteGreys( objsfound );

static bool doFindKeyStr( const uiMainWin& srcwin, CmdRecEvent& event,
			  const uiObject* localenv=0 )
{
    if ( !event.object_ )
	return false;

    bool allobjsrelative = false;
    ObjectFinder csobjfinder( srcwin, true );
    mFindAllNodes( csobjfinder, localenv, objsfound );
    const BufferString srckey = CmdComposer::factoryKey( event.object_ );
    takeSimilarObjs( objsfound, srckey, true, true );

    if ( !objsfound.isPresent(event.object_) )
	return deepFindKeyStr( srcwin, event, localenv );

    ObjectSet<const CallBacker> curobjset = objsfound;
    ObjectFinder::NodeTag curtag = ObjectFinder::UiObjNode;
    const CallBacker* curnode = event.object_;
    FileMultiString curkeystr;

    do
    {
	ObjectSet<const CallBacker> relatives;
	csobjfinder.findNodes( curtag, curnode, &relatives ); 

	for ( int idx=0; idx<curobjset.size(); idx++ )
	{
	    allobjsrelative = relatives.isPresent(curobjset[idx]);
	    if ( !allobjsrelative )
		break;
	}

	takeSimilarObjs( relatives, "UILABEL", true, false );
	takeSimilarObjs( relatives, "UISEPARATOR", false, true );
	takeSimilarObjs( relatives, srckey, false, true );
	takeSimilarObjs( relatives, "UIGROUPOBJ", false, true );

	ObjectSet<const CallBacker> minobjset;
	BufferString minkey;

	BufferStringSet aliases;
	while ( !relatives.isEmpty() || !aliases.isEmpty() )
	{
	    if ( aliases.isEmpty() )
	    {
		csobjfinder.getAliases( *relatives[0], aliases );
		relatives.removeSingle( 0 );
	    }

	    ObjectSet<const CallBacker> newobjset = curobjset;
	    FileMultiString newkeystr;
	    newkeystr += aliases[0]->buf(); 
	    aliases.removeSingle( 0 );

	    csobjfinder.selectNodes( newobjset, newkeystr );
	    if ( !newobjset.isPresent(event.object_) )
		continue;

	    if ( minobjset.isEmpty() || newobjset.size()<minobjset.size() )
	    {
		minobjset = newobjset;
		minkey = newkeystr[0];
	    }
	    if ( minobjset.size() == 1 )
		break;
	}

	if ( minobjset.size()==1 || minobjset.size()<curobjset.size() )
	{
	    curobjset = minobjset;
	    curkeystr += minkey;
	}

	csobjfinder.getAncestor( curtag, curnode );
    }
    while ( curobjset.size()>1 && !allobjsrelative );

    for ( int idx=curkeystr.size()-2; idx>=0; idx-- )
    {
	ObjectSet<const CallBacker> newobjset = objsfound;
	FileMultiString shorterkeystr;
	for ( int idy=0; idy<curkeystr.size(); idy++ )
	{
	    if ( idy != idx )
		shorterkeystr += curkeystr[idy];
	}
	csobjfinder.selectNodes( newobjset, shorterkeystr );
	if ( !newobjset.isPresent(event.object_) )
	    continue;
	if ( newobjset.size() == curobjset.size() )
	    curkeystr = shorterkeystr;
    }

    event.keystr_ = curkeystr.unescapedStr();
    if ( curobjset.size() > 1 )
    {
	event.keystr_ += "#";
	event.keystr_ += 1 + curobjset.indexOf( event.object_ );
    }
    event.similarobjs_ = objsfound.size() > 1;
    event.srcwin_ = &srcwin;

    ObjectFinder ciobjfinder( srcwin, false );
    ciobjfinder.selectNodes( objsfound, curkeystr );
    if ( objsfound.size() > curobjset.size() )
	event.casedep_ = true;

    return true;
}


static bool deepFindKeyStr( const uiMainWin& srcwin, CmdRecEvent& event,
			    const uiObject* localenv )
{
    ObjectFinder objfinder( srcwin );
    mFindAllNodes( objfinder, localenv, objsfound );

    for ( int idx=0; idx<objsfound.size(); idx++ )
    {
	mDynamicCastGet( const uiTable*, uitable, objsfound[idx] );
	if ( uitable )
	{
	    RowCol rc;
	    for ( rc.row()=0; rc.row()<uitable->nrRows(); rc.row()++ )
	    {
		for ( rc.col()=0; rc.col()<uitable->nrCols(); rc.col()++ )
		{
		    uiObject* uiobj = uitable->getCellObject( rc );
		    if ( uiobj && doFindKeyStr(srcwin, event, uiobj) )
		    {
			CmdRecEvent execdummy;
			execdummy.object_ = const_cast<uiTable*>( uitable );
			doFindKeyStr( srcwin, execdummy, localenv );
			TableCmdComposer::getExecPrefix( execdummy, rc );

			event.casedep_ = event.casedep_ || execdummy.casedep_;
			BufferString execprefixtail = event.execprefix_;
			event.execprefix_ = execdummy.execprefix_;
			event.execprefix_ += execprefixtail;
			return true;
		    }
		}
	    }
	}
    }
    return false;
}


bool CmdRecorder::findKeyString( const uiMainWin& srcwin, CmdRecEvent& event )
{
    if ( event.object_ != lastobjsearched_ )
    {
	lastobjsearched_ = event.object_;
	lastobjfreewins_.erase();
    }
    else if ( lastobjfreewins_.isPresent( &srcwin ) )
	return false;

    bool res = doFindKeyStr( srcwin, event );
    const UIEntity uientity( event.object_ );
    if ( !res && uientity.visible() && uientity.sensitive() )
	lastobjfreewins_ += &srcwin;

    return res;
}


static bool findMenuPath( const uiActionContainer& mnu,
			  const uiAction& searchitem,
			  FileMultiString& menupath, bool& casedep )
{
    bool itemfound = false;
    const uiAction* curitem = 0;
    FileMultiString pathtail;

    for ( int idx=0; idx<mnu.actions().size(); idx++ )
    {
	curitem = mnu.actions()[idx];
	if ( !curitem->isEnabled() )
	    continue;

	if ( curitem == &searchitem )
	{
	    itemfound = true;
	    casedep = false;
	    break;
	}

	if ( !curitem->getMenu() )
	    continue;

	if ( findMenuPath(*curitem->getMenu(), searchitem, pathtail, casedep) )
	{
	    itemfound = true;
	    break;
	}
    }

    if ( !itemfound )
	return false;

    mGetAmpFilteredStr( curtxt, curitem->text() );
    mDressNameString( curtxt, sMenuPath );

    int nrmatches = 0;
    int selnr = 0;

    for ( int idx=0; idx<mnu.actions().size(); idx++ )
    {
	const uiAction* mnuitm = mnu.actions()[idx];
	if ( !mnuitm->isEnabled() )
	    continue;

	mGetAmpFilteredStr( mnuitmtxt, mnuitm->text() );
	if ( SearchKey(curtxt,false).isMatching(mnuitmtxt) )
	{
	    if ( SearchKey(curtxt,true).isMatching(mnuitmtxt) )
	    {
		nrmatches++;
		if ( mnuitm == curitem )
		    selnr = nrmatches;
	    }
	    else
		casedep = true;
	}
    }

    if ( selnr && nrmatches>1 )
    {
	curtxt += "#"; curtxt += selnr;
    }

    menupath.setEmpty();
    menupath += curtxt;
    for ( int idx=0; idx<pathtail.size(); idx++ )
	menupath += pathtail[idx];

    return true;
}


static bool findMenuPath( const uiMainWin& srcwin, CmdRecEvent& ev )
{
    if ( !ev.mnuitm_ )
	return false;

    FileMultiString pathfms;
    const uiMenuBar* mnubar = const_cast<uiMainWin&>(srcwin).menuBar();
    if ( mnubar && mnubar->isSensitive() &&
	 findMenuPath(*mnubar, *ev.mnuitm_, pathfms, ev.casedep_) )
    {
	ev.srcwin_ = &srcwin;
	ev.menupath_ = pathfms.unescapedStr();
	return true;
    }

    ObjectSet<const CallBacker> objsfound;
    ObjectFinder objfinder( srcwin );
    objfinder.findNodes( ObjectFinder::Everything, &objsfound );

    for ( int idx=objsfound.size()-1; idx>=0; idx-- )
    {
	const uiMenu* menu = UIEntity(objsfound[idx]).menu();
	if ( menu && findMenuPath(*menu,*ev.mnuitm_,pathfms,ev.casedep_) )
	{
	    ev.srcwin_ = &srcwin;
	    ev.menupath_ = pathfms.unescapedStr();
	    ev.object_ = const_cast<CallBacker*>( objsfound[idx] );
	    doFindKeyStr( srcwin, ev );
	    return true;
	}
    }
    return false;
}


void CmdRecorder::handleEvent( CallBacker* cb )
{
    CmdRecEvent ev;
    mCBCapsuleUnpackWithCaller( const char*, msg, caller, cb );

    const char* msgnext = getNextWord( msg, ev.idstr_.buf() );
    mSkipBlanks ( msgnext );

    const bool iscarrieronly = isNumberString( ev.idstr_, true );
    if ( !iscarrieronly )
    {
#ifdef __lux32__
	ev.idstr_ = toString((od_uint32) caller );
#else
	ev.idstr_ = toString((od_uint64) caller );
#endif
	msgnext = msg;
    }
	
    BufferString beginendword;
    const char* msgnexxt = getNextWord( msgnext, beginendword.buf() );

    if ( mMatchCI(beginendword,"Begin") )
	ev.begin_ = true;
    else if ( mMatchCI(beginendword,"End") )
	ev.begin_ = false;
    else 
	return;

    char* msgnexxxt;
    ev.refnr_ = strtol( msgnexxt, &msgnexxxt, 0 );
    mSkipBlanks ( msgnexxxt );
    ev.msg_ = msgnexxxt;

    BufferString keyword;
    const char* msgtail = getNextWord( msgnexxxt, keyword.buf() );
    mSkipBlanks ( msgtail );

    if ( !iscarrieronly )
    {
	if ( ev.begin_ || !mMatchCI(keyword,"Close") )
	{
	    mDynamicCast( uiObject*, ev.object_, caller );
	    mDynamicCast( uiAction*, ev.mnuitm_, caller );
	    mDynamicCast( const uiMainWin*, ev.srcwin_, caller );
	}
    }

    BufferString fackey = CmdComposer::factoryKey( caller, keyword );

    if ( mMatchCI(keyword,"WinPopUp") )
    {
	if ( ev.begin_ )
	    popuprefnrs_ += ev.refnr_;
	else
	{
	    const int idx = popuprefnrs_.indexOf( ev.refnr_ );
	    if ( idx >= 0 )
		popuprefnrs_.removeSingle( idx );
	}
	return;
    }

    if ( !ev.begin_ )
    {
	for ( int idx=0; idx<composers_.size(); idx++ )
	{
	    if ( composers_[idx]->traceSrcWin(ev) )
		break;
	}
    }
    else if ( ev.object_ || ev.mnuitm_ )
    {
	ObjectSet<uiMainWin> windowlist;
	uiMainWin::getTopLevelWindows( windowlist );

	for ( int idx=windowlist.size()-1; idx>=0; idx-- )
	{
	    uiMainWin* uimw = windowlist[idx];

	    if ( ev.mnuitm_ )
		ev.object_ = ev.mnuitm_;
	    if ( findKeyString(*uimw, ev) )
	    {
		if ( ev.mnuitm_ )
		{
		    fackey = "UIBUTTON";
		    ev.mnuitm_ = 0;
		}
		break;
	    }

	    if ( ev.mnuitm_ )
	    {
		ev.object_ = 0;
		if ( findMenuPath(*uimw, ev) )
		    break;

		FileMultiString pathfms;
		if ( !idx && dynamicpopupmenu_ &&
		     findMenuPath(*dynamicpopupmenu_,
				  *ev.mnuitm_, pathfms, ev.casedep_) )
		{
		    ev.dynamicpopup_ = true;
		    ev.srcwin_ = winstack_.topWin();
		    ev.menupath_ = pathfms.unescapedStr();
		}	
	    }
	}
    }
    else if ( iscarrieronly )
    {
	ev.srcwin_ = winstack_.topWin(); 
	ev.openqdlg_ = true;
	ev.qdlgtitle_ = msgtail;
	openqdialog_ = true;
    }

    ev.stolen_ = !popuprefnrs_.isEmpty() || (openqdialog_ && !ev.openqdlg_);

    dynamicpopupmenu_ = 0;

    if ( !ev.srcwin_ ) 
	return;

    ObjectSet<CmdComposer> candidates = composers_;

    for ( int idx=candidates.size()-1; idx>=0; idx-- )
    {
	if ( candidates[idx]->greedy() )
	{
            if ( candidates[idx]->accept(ev) )
		ev.nraccepts_++;

	    candidates.removeSingle( idx );
	}
    }

    for ( int idx=candidates.size()-1; idx>=0; idx-- )
    {
	if ( candidates[idx]->accept(ev) )
	    ev.nraccepts_++;
    }

    winstack_.moveToTop( ev.srcwin_ );
		
    if ( ev.openqdlg_ && !ev.begin_ )
	openqdialog_ = false;

    if ( !ev.nraccepts_ && !ev.dynamicpopup_ )
    {
	CmdComposer* newcomp = CmdComposer::factory().create( fackey, rec_ );

	if ( newcomp )
	{
	    if ( newcomp->accept(ev) )
		composers_.insertAt( newcomp, 0 );
	    else
		delete newcomp;
	}
    }

    bool updatecomposers = false;
    for ( int idx=composers_.size()-1; idx>=0; idx-- )
    {
	if ( composers_[idx]->stateUpdateNeeded() || composers_[idx]->done() )
	    updatecomposers = true;

	if ( composers_[idx]->done() )
	    delete composers_.removeSingle( idx );
    }

    if ( updatecomposers )
	updateCmdComposers();
}


void CmdRecorder::insertWinAssertion( const CmdRecEvent& ev )
{
    if ( mustSkip() )
	return;

    BufferString winstr;

    if ( ev.openqdlg_ )
	winstr = ev.qdlgtitle_;
    else if ( ev.srcwin_ )
	winstr = windowTitle( applWin(), ev.srcwin_ );
    else
	return;

    mDressNameString( winstr, sWinAssert );
    
    ObjectSet<uiMainWin> ciwinlist, cswinlist;
    SearchKey(winstr, true).getMatchingWindows( applWin(), cswinlist ); 
    SearchKey(winstr,false).getMatchingWindows( applWin(), ciwinlist ); 

    if ( cswinlist.size() > (ev.openqdlg_ && !ev.begin_ ? 0 : 1) )
    {
	winstr += "#";
	winstr += (ev.openqdlg_ ? 1 + cswinlist.size() :
				  1 + cswinlist.indexOf(ev.srcwin_) );
    }

    const bool winstrcasedep = ciwinlist.size() > cswinlist.size();
    const bool hasbecomecasedep = !winassertcasedep_ && winstrcasedep;

    if ( winstr!=winassertion_ || hasbecomecasedep )
    {
	mRecOutStrm << std::endl;
	flush();

	if ( ciwinlist.size() > cswinlist.size() )
	    mRecOutStrm << "Case Sensitive" << std::endl;

	mRecOutStrm << "[" << winstr << "]" << std::endl;

	winassertion_ = winstr;
	winassertcasedep_ = winstrcasedep;
    }
}


void CmdRecorder::updateCmdComposers()
{
    for ( int idx=composers_.size()-1; idx>=0; idx-- )
	composers_[idx]->updateInternalState();

    CmdRecStopper::clearStopperList( this );
}


std::ostream& CmdRecorder::outputStrm() const
{ return writetailonly_ || bufsize_>99 ? bufstream_ : *outputsd_.ostrm; }


void CmdRecorder::flush()
{
    if ( &outputStrm() == outputsd_.ostrm )
	return;

    int sz = bufstr_.size();
    const int nrchars = bufstream_.str().size();
    bufstr_.setBufSize( mMAX(sz+nrchars+1, 2*bufsize_) );

    for ( int idx=0; idx<nrchars; idx++ )
	bufstr_.buf()[sz++] = bufstream_.str()[idx];

    bufstr_.buf()[sz] = '\0';

    bufstream_.str( "" );
    bufstream_.clear();

    int nrvoidchars = 0;
    while ( writetailonly_ && bufsize_<sz )
    {
	const char* ptr = bufstr_.buf() + nrvoidchars;
	while ( *ptr && (*ptr!='\n' || *(ptr+1)!='\n') )
	    ptr++;

	const int newnrvoidchars = mCast( int, ptr + 2 - bufstr_.buf() );
	if ( !*ptr || bufsize_>=sz-newnrvoidchars )
	    break;

	nrvoidchars = newnrvoidchars;
	nrparskipped_++;
    }

    if ( !recording_ && nrparskipped_ )
    {
	 *outputsd_.ostrm << std::endl << "###  < ... " << nrparskipped_
			  << " paragraph" << (nrparskipped_>1 ? "s" : "")
			  << " skipped ... >" << std::endl << std::endl;
	 nrparskipped_ = 0;
    }

    if ( !recording_ || (!writetailonly_ && bufsize_<sz) )
    {
	*outputsd_.ostrm << bufstr_.buf()+nrvoidchars;
	outputsd_.ostrm->flush();
	nrvoidchars = sz;
    }

    if ( nrvoidchars )
	memmove( bufstr_.buf(), bufstr_.buf()+nrvoidchars, sz-nrvoidchars+1 );
}


}; // namespace CmdDrive
