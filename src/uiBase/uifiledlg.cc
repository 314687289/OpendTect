/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.42 2008-09-09 10:52:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"

#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"

#include "uiparentbody.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uilabel.h"

#include <QFileDialog>


const char* uiFileDialog::filesep_ = ";";

class ODFileDialog : public QFileDialog
{
public:

ODFileDialog( const QString& dirname, const QString& filter=QString::null,
	      QWidget* parent=0, const char* caption=0, bool modal=false )
    : QFileDialog(parent,caption,dirname,filter)
{ setModal( modal ); }

ODFileDialog( QWidget* parent=0, const char* caption=0, bool modal=false )
    : QFileDialog(parent,caption)
{ setModal( modal ); }

};


QFileDialog::Mode qmodeForUiMode( uiFileDialog::Mode mode )
{
    switch( mode )
    {
    case uiFileDialog::AnyFile		: return QFileDialog::AnyFile;
    case uiFileDialog::ExistingFile	: return QFileDialog::ExistingFile;
    case uiFileDialog::Directory	: return QFileDialog::Directory;
    case uiFileDialog::DirectoryOnly	: return QFileDialog::DirectoryOnly;
    case uiFileDialog::ExistingFiles	: return QFileDialog::ExistingFiles;
    }
    return QFileDialog::AnyFile;
}

#define mCommon \
    , fname_( fname ) \
    , filter_( filter ) \
    , caption_(caption) \
    , parnt_(parnt) \
    , addallexts_(false) \
    , confirmoverwrite_(true)

uiFileDialog::uiFileDialog( uiParent* parnt, bool forread,
			    const char* fname, const char* filter,
			    const char* caption )
	: mode_( forread ? ExistingFile : AnyFile )
        , forread_( forread )
        mCommon
{
    if ( !caption || !*caption )
	caption_ = forread ? "Open" : "Save As";
}


uiFileDialog::uiFileDialog( uiParent* parnt, Mode mode,
			    const char* fname, const char* filter,
			    const char* caption )
	: mode_( mode )
        , forread_(true)
	mCommon
{}


int uiFileDialog::go()
{
    FilePath fp( fname_ );
    fname_ = fp.fullPath();
    BufferString dirname;
    if ( !File_isDirectory(fname_) )
    {
	if ( !File_isDirectory(fp.pathOnly()) )
	{
	    dirname = GetPersonalDir();
	    fname_ = "";
	}
	else if ( !File_exists(fname_) &&
		  (mode_ == ExistingFile || mode_ == ExistingFiles) )
	{
	    dirname = fp.pathOnly();
	    fname_ = "";
	}
	else
	{
	    dirname = fp.pathOnly();
	    fname_ = fp.fileName();
	}
    }
    else
    {
	dirname = fname_;
	fname_ = "";
    }

    if ( GetEnvVarYN("OD_FILE_SELECTOR_BROKEN") )
    {
	uiDialog dlg( parnt_, uiDialog::Setup("Specify file name",
			    "System file selection unavailable!",mNoHelpID) );
	uiLineEdit* le = new uiLineEdit( &dlg, "File name" );
	le->setText( dirname );
	new uiLabel( &dlg, "File name", le );
	if ( !dlg.go() ) return 0;
	fn = le->text(); filenames.add( fn );
	return 1;
    }

    QWidget* qparent = 0;
    if ( parnt_ && parnt_->pbody() )
	qparent = parnt_->pbody()->managewidg();

    BufferString flt( filter_ );
    if ( filter_.size() )
    {
	if ( addallexts_ )
	{
	    flt += ";;All files (*";
#ifdef __win__
	    flt += ".*";
#endif
	    flt += ")";
	}
    }

    ODFileDialog* fd = new ODFileDialog( QString(dirname), QString(flt),
					 qparent, "File dialog", true );
    fd->selectFile( QString(fname_) );
    fd->setAcceptMode( forread_ ? QFileDialog::AcceptOpen
	    			: QFileDialog::AcceptSave );
    fd->setMode( qmodeForUiMode(mode_) );
    fd->setCaption( QString(caption_) );
    fd->setConfirmOverwrite( confirmoverwrite_ );
    if ( !currentdir_.isEmpty() )
	fd->setDirectory( QString(currentdir_.buf()) );
    if ( selectedfilter_.size() )
	fd->selectFilter( QString(selectedfilter_) );
    
#ifdef __win__
    fd->setViewMode( QFileDialog::Detail );
#endif

    if ( fd->exec() != QDialog::Accepted )
    	return processExternalFilenames( dirname, flt );

    QStringList list = fd->selectedFiles();
    if (  list.size() )
	fn = list[0].toAscii().constData();
    else 
	fn = fd->selectedFile().toAscii().constData();

    selectedfilter_ = fd->selectedFilter().toAscii().constData();

#ifdef __win__
    replaceCharacter( fn.buf(), '/', '\\' );
#endif

    for ( int idx=0; idx<list.size(); idx++ )
    {
	BufferString* bs = new BufferString( list[idx].toAscii().constData() );

#ifdef __win__
	replaceCharacter( bs->buf(), '/', '\\' );
#endif
	filenames += bs;
    }

    return 1;
}


void uiFileDialog::getFileNames( BufferStringSet& fnms ) const
{
    deepCopy( fnms, filenames );
}


void uiFileDialog::list2String( const BufferStringSet& list,
				BufferString& string )
{
    QStringList qlist;
    for ( int idx=0; idx<list.size(); idx++ )
	qlist.append( (QString)list[idx]->buf() );

    string = qlist.join( (QString)filesep_ ).toAscii().constData();
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
{
    QStringList qlist = QStringList::split( (QString)filesep_,
	    				    (QString)string );
    for ( int idx=0; idx<qlist.size(); idx++ )
	list += new BufferString( qlist[idx].toAscii().constData() );
}


FileMultiString* uiFileDialog::externalfilenames_ = 0;
BufferString uiFileDialog::extfilenameserrmsg_ = "";


void uiFileDialog::setExternalFilenames( const FileMultiString& fms )
{
    extfilenameserrmsg_.setEmpty();

    if ( !externalfilenames_ )
	externalfilenames_ = new FileMultiString( fms );
    else
	*externalfilenames_ = fms;
}


const char* uiFileDialog::getExternalFilenamesErrMsg()
{
    return extfilenameserrmsg_.isEmpty() ? 0 : extfilenameserrmsg_.buf();
}


static bool filterIncludesExt( const char* filter, const char* ext )
{
    if ( !filter )
	return false;

    const char* fltptr = filter;
    while ( *fltptr != '\0' )
    {
	fltptr++;
	if ( *(fltptr-1) != '*' )
	    continue;
	if ( *(fltptr) != '.' )
	    return true;
	
	if ( !ext )
	    continue;
	const char* extptr = ext;
	while ( true )
	{ 
	    fltptr++;
	    if ( *extptr == '\0' )
	    {
		if ( !isalnum(*fltptr) )
		    return true;
		break;
	    }
	    if ( tolower(*extptr) != tolower(*fltptr) )
		break;
	    extptr++;
	}
    }
    return false;
}


#define mRetErrMsg( pathname, msg ) \
{ \
    if ( msg ) \
    { \
	extfilenameserrmsg_ += "Path name \""; \
	extfilenameserrmsg_ += pathname; \
	extfilenameserrmsg_ += "\" "; \
	extfilenameserrmsg_ += msg; \
    } \
    delete externalfilenames_; \
    externalfilenames_ = 0; \
    return msg ? 0 : 1; \
}

int uiFileDialog::processExternalFilenames( const char* dir, 
					    const char* filters )
{
    if ( !externalfilenames_ )
	return 0;

    deepErase( filenames );
    fn.setEmpty();

    if ( externalfilenames_->isEmpty() )
	mRetErrMsg( "", mode_==ExistingFiles ? 0 : "should not be empty" );

    if ( !dir )
	dir = currentdir_.isEmpty() ? GetPersonalDir() : currentdir_.buf();
    if ( !filters )
	filters = filter_.buf();

    BufferStringSet filterset;
    const SeparString fltsep( filters, uiFileDialog::filesep_[0] );
    for ( int fltidx=0; fltidx<fltsep.size(); fltidx++ )
	filterset.add( fltsep[fltidx] );
    
    for ( int idx=0; idx<externalfilenames_->size(); idx++ )
    {
	BufferString fname( externalfilenames_[idx] );
	FilePath fp( fname );
	if ( !fp.isAbsolute() )
	{
	    fp = dir;
	    fp.add( fname );
	}
	fname = fp.fullPath();

	if ( !idx && externalfilenames_->size()>1 && mode_!=ExistingFiles )
	    mRetErrMsg( fname, "expected to be solitary" );

	if ( File_isDirectory(fname) )
	{
	    if ( mode_!=Directory && mode_!=DirectoryOnly )
		mRetErrMsg( fname, "specifies an existing directory" );
	    if ( !forread_ && !File_isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only directory" );
	}
	else 
	{
	    if ( mode_==Directory || mode_==DirectoryOnly )
		mRetErrMsg( fname, "specifies no existing directory" );

	    if ( !File_exists(fname) )
	    {
		if ( mode_ != AnyFile ) 
		    mRetErrMsg( fname, "specifies no existing file" );
		if ( fp.nrLevels() > 1 )
		{
		    if ( !File_isDirectory(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in non-existing directory" );
		    if ( !forread_ && !File_isWritable(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in a read-only directory" );
		}
	    }
	    else if ( !forread_ && !File_isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only file" );
	}

	BufferString* bs = new BufferString( fname );
#ifdef __win__
	replaceCharacter( bs->buf(), '/', '\\' );
#endif
	filenames += bs;
	
	if ( !idx )
	    fn = *bs;

	for ( int fltidx=filterset.size()-1; fltidx>=0; fltidx-- )
	{
	    if ( !filterIncludesExt(filterset[fltidx]->buf(),fp.extension()) )
		filterset.remove( fltidx );
	}
	if ( filterset.isEmpty() )
	    mRetErrMsg( fname, "has an incompatible file extension" );
    }

    selectedfilter_ = filterset[0]->buf();
    mRetErrMsg( "", 0 );
}
