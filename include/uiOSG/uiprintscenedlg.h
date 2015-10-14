#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uisaveimagedlg.h"

class uiGenInput;
class uiLabeledComboBox;
class ui3DViewer;

namespace osgViewer
{ class View; }
namespace osg
{ class Image; }

/*!
\brief Print scene dialog box.
*/

mExpClass(uiOSG) uiPrintSceneDlg : public uiSaveImageDlg
{ mODTextTranslationClass(uiPrintSceneDlg)
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<ui3DViewer>&);
protected:

    uiLabeledComboBox*	scenefld_;

    const char*		getExtension();
    void		writeToSettings();
    bool		supportPrintFormats() const	{ return false; }

    enum		{InvalidImages=0, OnlyMainViewImage, MainAndHudImages };
    void		setFldVals(CallBacker*);
    void		sceneSel(CallBacker*);
    bool		acceptOK(CallBacker*);
    osg::Image*		offScreenRenderViewToImage(osgViewer::View*,
						   unsigned char transparency);
			/*! The returned image is not referenced yet. */
    int			validateImages(const osg::Image*,const osg::Image*);
    bool		hasImageValidFormat(const osg::Image*);
    void		flipImageVertical(osg::Image*);
    bool		saveImages(const osg::Image*,const osg::Image*);

    const ObjectSet<ui3DViewer>& viewers_;
};

#endif


