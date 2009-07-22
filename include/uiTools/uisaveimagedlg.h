#ifndef uisaveimagedlg_h
#define uisaveimagedlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          February 2009
 RCS:           $Id: uisaveimagedlg.h,v 1.4 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
#include "geometry.h"
#include "ptrman.h"

class IOPar;
class Settings;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledSpinBox;

mClass uiSaveImageDlg : public uiDialog
{
public:
			uiSaveImageDlg(uiParent*);

    Notifier<uiSaveImageDlg>	sizesChanged;

    void		sPixels2Inch(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&,float);
    void		sInch2Pixels(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&,float);
    void		sCm2Inch(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&);
    void		sInch2Cm(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&);
    void		createGeomInpFlds(uiParent*);
    void                fillPar(IOPar&,bool is2d);
    bool                usePar(const IOPar&);

protected:

    uiLabeledSpinBox*	heightfld_;
    uiLabeledSpinBox*	widthfld_;
    uiGenInput*		unitfld_;
    uiCheckBox*		lockfld_;
    uiGenInput*		dpifld_;
    uiGenInput*		useparsfld_;
    uiFileInput*	fileinputfld_;
    
    BufferString	filters_;
    BufferString	selfilter_;
    Settings&		settings_;

    void		getSettingsPar(PtrMan<IOPar>&,BufferString);
    void		setSizeInPix(int width, int height);
    void		updateFilter();
    virtual void	getSupportedFormats(const char** imgfrmt,
	    				    const char** frmtdesc,
					    BufferString& filter)	=0;
    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);
    void		surveyChanged(CallBacker*);
    virtual void	setFldVals(CallBacker*)			{}


    Geom::Size2D<float> sizepix_;
    Geom::Size2D<float> sizeinch_;
    Geom::Size2D<float> sizecm_;
    float		aspectratio_;	// width / height
    float		screendpi_;

    static BufferString	dirname_;

    void		updateSizes();
    virtual const char*	getExtension();
    virtual void	writeToSettings()		{}

    static const char*  sKeyType()	{ return "Type"; }
    static const char*  sKeyHeight()    { return "Height"; }
    static const char*  sKeyWidth()     { return "Width"; }
    static const char*  sKeyUnit()      { return "Unit"; }
    static const char*  sKeyRes()       { return "Resolution"; }
    static const char*  sKeyFileType()  { return "File type"; }
};

#endif
