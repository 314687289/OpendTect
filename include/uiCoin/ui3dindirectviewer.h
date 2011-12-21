#ifndef ui3dindirectviewer_h
#define ui3dindirectviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id: ui3dindirectviewer.h,v 1.2 2011-12-21 12:36:20 cvsjaap Exp $
________________________________________________________________________

-*/

#include "ui3dviewerbody.h"


class GraphicsWindowIndirect;

//Class used by ui3DViewer to render things indirectly

class ui3DIndirectViewBody : public ui3DViewerBody
{
public:
				ui3DIndirectViewBody(ui3DViewer&,uiParent*);
				~ui3DIndirectViewBody();

    const QWidget*              qwidget_() const;

protected:
    void			updateActModeCursor();
    osgGA::GUIActionAdapter&    getActionAdapter();
    osg::GraphicsContext*       getGraphicsContext();

						
    GraphicsWindowIndirect*	graphicswin_;    
};

#endif
