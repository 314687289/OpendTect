/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink/A.H. Bril
 Date:          Aug 2000/Oct 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitaskrunner.cc,v 1.15 2009-03-03 21:02:17 cvskris Exp $";

#include "uitaskrunner.h"

#include "mousecursor.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"

#include "timer.h"
#include "uimsg.h"
#include "thread.h"
#include "timefun.h"

#include <math.h>


uiTaskRunner::uiTaskRunner( uiParent* p ) 
    : uiDialog( p, uiDialog::Setup("Executing",mNoDlgTitle,mNoHelpID)
	.nrstatusflds( -1 )
	.oktext("Pause")
	.canceltext("Abort") )
    , task_( 0 )
    , tim_(*new Timer("") )
    , execnm_("")
    , statemutex_( *new Threads::Mutex )
    , dispmsgonerr_( true )
{
    progbar_ = new uiProgressBar( this, "ProgressBar", 0, 0 );
    progbar_->setPrefWidthInChar( 50 );

    tim_.tick.notify( mCB( this, uiTaskRunner, timerTick ) );
    finaliseStart.notify( mCB( this, uiTaskRunner, onFinalise ) );

    statusBar()->addMsgFld( "Current activity", OD::AlignLeft, 2 );
    statusBar()->addMsgFld( "Counted items", OD::AlignRight, 2 );
    statusBar()->addMsgFld( "Number done", OD::AlignLeft, 1 );
}


uiTaskRunner::~uiTaskRunner()
{
    delete &statemutex_;
    delete &tim_;
}


bool uiTaskRunner::execute( Task& t )
{
    if ( task_ )
	return (execres_ = t.execute());

    MouseCursorChanger mousecursor( MouseCursor::Arrow );

    task_ = &t;
    state_ = 1;
    prevtotalnr_ = -1;
    prevnrdone_ = -1;
    prevpercentage_ = -1;
    prevmessage_ = "";
    prevnrdonetext_ = prevmessage_;
    execnm_ = task_->name();

    updateFields();
    return (execres_ = go());
}


void uiTaskRunner::onFinalise( CallBacker* )
{
    const int totalnr = task_->totalNr();

    tim_.start( 100, true );

    Threads::MutexLocker lock( uitaskrunnerthreadmutex_ );
    thread_ = new Threads::Thread( mCB(this,uiTaskRunner,doWork) );
}


void uiTaskRunner::doWork( CallBacker* )
{
    task_->enableNrDoneCounting( true );
    task_->enableWorkContol( true );
    bool res = task_->execute();

    statemutex_.lock();
    state_ = res ? 0 : -1;
    statemutex_.unLock();

    thread_->threadExit();
}


void uiTaskRunner::updateFields()
{
    uiStatusBar& sb = *statusBar();

    dispinfomutex_.lock();
    const int totalnr = task_->totalNr();
    const int nrdone = task_->nrDone();
    const BufferString nrdonetext = task_->nrDoneText();
    const BufferString message = task_->message();


    if ( prevmessage_!=message )
    {
	sb.message( message.buf(), 0 );
	prevmessage_ = message;
    }
    if ( prevnrdonetext_!=nrdonetext )
    {
	sb.message( nrdonetext.buf(), 1 );
	prevnrdonetext_ = nrdonetext;
    }

    const bool nrdonechg = prevnrdone_ != nrdone;
    if ( nrdonechg )
    {
	prevnrdone_ = nrdone;
	BufferString str; str += nrdone;
	sb.message( str, 2 );
    }

    if ( totalnr > 0 )
    {
	if ( prevtotalnr_ <= 0 )
	    progbar_->display( true );

	if ( totalnr != prevtotalnr_ )
	{
	    progbar_->setTotalSteps( totalnr );
	    prevtotalnr_ = totalnr;
	}

	if ( nrdonechg )
	    progbar_->setProgress( nrdone );

	const float fpercentage = 100. * ((float)nrdone) / totalnr;
	int percentage = (int)fpercentage;
	if ( percentage > 100 ) percentage = 100;
	if ( percentage!=prevpercentage_ )
	{
	    BufferString capt( "[" );
	    capt += percentage; capt += "%] "; capt += execnm_;
	    setCaption( capt.buf() );

	    prevpercentage_ = percentage;
	}
    }
    else if ( prevtotalnr_ > 0 )
	progbar_->display( false );

    dispinfomutex_.unLock();
}


bool uiTaskRunner::acceptOK( CallBacker* )
{
    uitaskrunnerthreadmutex_.lock();
    Task::Control state;
    if ( task_ )
	state = task_->getState();
    else
    {
	uitaskrunnerthreadmutex_.unLock();
	return false;
    }

    uitaskrunnerthreadmutex_.unLock();

    if ( state==Task::Pause )
    {
	task_->controlWork( Task::Run );
	setOkText("Pause" );
    }
    else if ( state==Task::Run )
    {
	task_->controlWork( Task::Pause );
	setOkText("Resume" );
    }

    return false;
}


void uiTaskRunner::timerTick( CallBacker* )
{
    statemutex_.lock();
    const int state = state_;
    statemutex_.unLock();

    if ( state<1 )
    {
	BufferString message;
	if ( uitaskrunnerthreadmutex_.tryLock() )
	{
	    message = finalizeTask();
	    uitaskrunnerthreadmutex_.unLock();
	}

	if ( state<0 && dispmsgonerr_ )
	    uiMSG().error( message );

	done( state<0 ? 0 : 1 );
	return;
    }

    updateFields();
    tim_.start( 250, true );
}


BufferString uiTaskRunner::finalizeTask()
{
    BufferString message;
    if ( thread_ )
    {
	thread_->stop();
	thread_ = 0;
    }

    if ( task_ ) message = task_->message();
    task_ = 0;

    uitaskrunnerthreadmutex_.unLock();

    return message;
}


bool uiTaskRunner::rejectOK( CallBacker* )
{
    uitaskrunnerthreadmutex_.lock();
    if ( task_ ) task_->controlWork( Task::Stop );
    finalizeTask();
    uitaskrunnerthreadmutex_.unLock();

    execres_ = false;
    return true;
}
