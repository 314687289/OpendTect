/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowgrabber.cc,v 1.11 2009-08-31 05:12:52 cvsnanne Exp $";

#include "uiwindowgrabber.h"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uislider.h"

#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "settings.h"
#include "timefun.h"

BufferString uiWindowGrabDlg::dirname_ = "";

static const char* imageformats[] =
{ "jpg", "png", "bmp", "ppm", "xpm", "xbm", 0 };

static const char* filters[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "Bitmap (*.bmp)",
    "PPM (*.ppm)",
    "XPM (*.xpm)",
    "XBM (*.xbm)",
    0
};



uiWindowGrabDlg::uiWindowGrabDlg( uiParent* p, bool desktop )
    : uiDialog(p,uiDialog::Setup( (desktop ? "Desktop grabber" : 
					     "Window grabber"),
				  (desktop ? "Specify image file" :
					     "Specify window and image file"),
				  desktop ? "50.0.11" : "50.0.10") )
    , windowfld_(0)
{
    getTopLevelWindows( windowlist_ );
    if ( !desktop && !windowlist_.isEmpty() )
    {
	BufferStringSet windownms;
	for ( int idx=0; idx<windowlist_.size(); idx++ )
	    windownms.add( windowlist_[idx]->name() );

	windowfld_ = new uiLabeledComboBox( this, windownms, "Grab window" );
    }

    if ( dirname_.isEmpty() )
	dirname_ = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld_ = new uiFileInput( this, "Filename",
				    uiFileInput::Setup(uiFileDialog::Gen)
				    .forread(false)
				    .defseldir(dirname_)
				    .directories(false)
	   			    .allowallextensions(false) );
    fileinputfld_->valuechanged.notify( mCB(this,uiWindowGrabDlg,fileSel) );
    if ( windowfld_ )
	fileinputfld_->attach( alignedBelow, windowfld_ ); 
    updateFilter();

    qualityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Image quality").withedit(true),
	    "Quality slider" );
    qualityfld_->attach( alignedBelow, fileinputfld_ );
    qualityfld_->sldr()->setInterval( StepInterval<float>(0,100,1) );
    qualityfld_->sldr()->setValue(50);
    qualityfld_->attach( alignedBelow, fileinputfld_ );

    infofld_ = new uiLabel( this, "Arrange your windows before confirming" );
    infofld_->attach( alignedBelow, qualityfld_ );
    
    IOM().afterSurveyChange.notify( mCB(this,uiWindowGrabDlg,surveyChanged) );
}


void uiWindowGrabDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiWindowGrabDlg::updateFilter()
{
    Settings& settings = Settings::fetch("snapshot");
    PtrMan<IOPar> iopar = settings.subselect( "3D" );
    BufferString deftype = imageformats[0];
    if ( iopar ) iopar->get( "File type", deftype );

    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( deftype != imageformats[idx] )
	{
	    idx++;
	    continue;
	}

	fileinputfld_->setSelectedFilter( filters[idx] );
	break;
    }

    BufferString filter;
    idx = 0;
    while ( imageformats[idx] )
    {
	if ( !filter.isEmpty() ) filter += ";;";
	filter += filters[idx];
	idx++;
    }

    fileinputfld_->setFilter( filter );
}


void uiWindowGrabDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld_->fileName();
    addFileExtension( filename );
    fileinputfld_->setFileName( filename );
}


void uiWindowGrabDlg::addFileExtension( BufferString& filename )
{
    FilePath fp( filename.buf() );
    fp.setExtension( getExtension() );
    filename = fp.fullPath();
}


bool uiWindowGrabDlg::filenameOK() const
{
    BufferString filename = fileinputfld_->fileName();
    if ( filename.isEmpty() )
    {
	uiMSG().error( "Please select filename" );
	return false;
    }

    if ( File_exists(filename) )
    {
	BufferString msg = "The file "; msg += filename; 
	if ( !File_isWritable(filename) )
	{
	    msg += " is not writable";
	    uiMSG().error(msg);
	    return false;
	}
    }

    return true;
}


bool uiWindowGrabDlg::acceptOK( CallBacker* )
{
    if ( !filenameOK() )
       	return false;
    
    FilePath filepath( fileinputfld_->fileName() );
    dirname_ = filepath.pathOnly();
    filename_ = filepath.fullPath();
    return true;
}


const char* uiWindowGrabDlg::getExtension() const
{
    int ifmt = -1;
    FilePath fp( fileinputfld_->fileName() );
    const BufferString ext( fp.extension() );
    for ( int idx=0; imageformats[idx]; idx++ )
    {
	if ( ext == imageformats[idx] )
	    { ifmt = idx; break; }
    }

    if ( ifmt < 0 )
    {
	const char* selectedfilter = fileinputfld_->selectedFilter();
	for ( ifmt=0; filters[ifmt]; ifmt++ )
	{
	    if ( !strcmp(selectedfilter,filters[ifmt]) )
		break;
	}
    }

    return imageformats[ifmt] ? imageformats[ifmt] : imageformats[0];
}


uiMainWin* uiWindowGrabDlg::getWindow() const
{
    const int idx = windowfld_ ? windowfld_->box()->currentItem() : 0;
    return const_cast<uiMainWin*>( windowlist_[idx] );
};


int uiWindowGrabDlg::getQuality() const
{ return qualityfld_->sldr()->getIntValue(); }


uiWindowGrabber::uiWindowGrabber( uiParent* p )
    : parent_(p)
    , desktop_(false)
    , grabwin_(0)
    , quality_(50)
    , execthr_( 0 )
{}


bool uiWindowGrabber::go()
{
    uiWindowGrabDlg dlg( parent_, desktop_ );
    if ( !dlg.go() )
	return false;

    grabwin_ = dlg.getWindow();
    filename_ = dlg.getFilename();
    quality_ = dlg.getQuality();

    execthr_ = new Threads::Thread( mCB(this,uiWindowGrabber,mkThread) );

    return true;
}


uiWindowGrabber::~uiWindowGrabber()
{
    if ( execthr_ )
    {
	execthr_->stop();
	delete execthr_;
    }
}



void uiWindowGrabber::mkThread( CallBacker* )
{
    // give window manager chance to remove the uiWindowGrabDlg from screen
    Time_sleep(0.1); 

    const int zoom = desktop_ ? 0 : 1;
    grabwin_->activateGrab( filename_, zoom, 0, quality_ );
}

