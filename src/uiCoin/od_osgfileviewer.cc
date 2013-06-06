/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "genc.h"
#include "bufstring.h"
#include "file.h"

# ifdef __msvc__
#  include "winmain.h"
# endif

#include <QtGui/QApplication>
#include <QtGui/QFileDialog>

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgQt/GraphicsWindowQt>

#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgManipulator/TabBoxDragger>

#include <osgDB/ReadFile>

#include <iostream>

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    BufferString file;
    if ( argc>1 )
	file = argv[1];

    QApplication app(argc, argv);    
    osgQt::initQtWindowingSystem();

    while ( !File::exists(file) )
    {
	QString newfile = QFileDialog::getOpenFileName( );
	file = newfile.toAscii().constData();

	if ( file.isEmpty() )
	{
	    std::cout << "Please select a osg file.\n" ;
	    return 1;
	}
    }

    osg::Node* root = osgDB::readNodeFile( file.buf() );

    if ( !root )
    {
	return 1;
    }
    
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setSceneData( root );
    viewer->setCameraManipulator( new osgGA::TrackballManipulator );
    osgQt::setViewer( viewer.get() );
    
    osgQt::GLWidget* glw = new osgQt::GLWidget;
    osgQt::GraphicsWindowQt* graphicswin = new osgQt::GraphicsWindowQt( glw );
    
    viewer->getCamera()->setViewport(
		    new osg::Viewport(0, 0, glw->width(), glw->height() ) );
    viewer->getCamera()->setGraphicsContext( graphicswin );

    glw->show();

    return app.exec();
}
