#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.26 2010-09-30 10:03:34 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiobjfileman.h"

class BufferStringSet;
namespace EM { class IOObjInfo; }

class uiButton;
class uiListBox;
class uiStratLevelSel;
class uiTextEdit;
class uiToolButton;

mClass uiSurfaceMan : public uiObjFileMan
{
public:
			uiSurfaceMan(uiParent*,const char* typ);
			~uiSurfaceMan();

    static Notifier<uiSurfaceMan>* fieldsCreated();
    void		addTool(uiButton*);

protected:

    uiButton*		lastexternal_;
    uiListBox*		attribfld_;

    bool		isCur2D() const;
    bool		isCurFault() const;

    uiToolButton*	man2dbut_;
    void		copyCB(CallBacker*);
    void		man2d(CallBacker*);
    void		setRelations(CallBacker*);
    void		stratSel(CallBacker*);

    void		removeAttribCB(CallBacker*);
    void		renameAttribCB(CallBacker*);

    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
    double		getFileSize(const char*,int&) const;

    const char*		getDefKey() const;
};


mClass uiSurface2DMan : public uiDialog
{
public:
    			uiSurface2DMan(uiParent*,const EM::IOObjInfo&);
protected:
    void		lineSel(CallBacker*);

    uiListBox*		linelist_;
    uiTextEdit*		infofld_;
    const EM::IOObjInfo& eminfo_;
};


#endif
