#ifndef uisegyreadstarter_h
#define uisegyreadstarter_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"
#include "segyuiscandata.h"
#include "uisegyimptype.h"

class Timer;
class DataClipSampler;
class uiLabel;
class uiButton;
class uiSpinBox;
class uiCheckBox;
class uiFileInput;
class uiSurveyMap;
class uiHistogramDisplay;
class uiSEGYImpType;
class uiSEGYReadStartInfo;
namespace SEGY { class ImpType; }


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

			uiSEGYReadStarter(uiParent*,bool forsurvsetup,
					  const SEGY::ImpType* fixedtype=0);
			~uiSEGYReadStarter();

    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }
    FullSpec		fullSpec() const;

    const SEGY::ImpType& impType() const;
    void		setImpTypIdx(int);

protected:

    SEGY::FileSpec	filespec_;
    FilePars		filepars_;
    FileReadOpts*	filereadopts_;

    uiSEGYImpType*	typfld_;
    uiFileInput*	inpfld_;
    uiSEGYReadStartInfo* infofld_;
    uiHistogramDisplay*	ampldisp_;
    uiSurveyMap*	survmap_;
    uiButton*		examinebut_;
    uiButton*		fullscanbut_;
    uiButton*		editbut_;
    uiSpinBox*		examinenrtrcsfld_;
    uiSpinBox*		clipfld_;
    uiCheckBox*		inc0sbox_;
    uiLabel*		nrfileslbl_;
    Timer*		filenamepopuptimer_;

    BufferString	userfilename_;
    SEGY::LoadDef	loaddef_;
    ObjectSet<SEGY::ScanInfo> scaninfo_;
    DataClipSampler&	clipsampler_;
    bool		infeet_;
    bool		veryfirstscan_;
    SEGY::ImpType	fixedimptype_;
    bool		survsetup_;

    bool		getExistingFileName(BufferString& fnm,bool werr=true);
    bool		getFileSpec();
    void		execNewScan(bool,bool full=false);
    void		scanInput();
    bool		scanFile(const char*,bool,bool);
    bool		obtainScanInfo(SEGY::ScanInfo&,od_istream&,bool,bool);
    bool		completeFileInfo(od_istream&,SEGY::BasicFileInfo&,bool);
    void		completeLoadDef(od_istream&);
    void		handleNewInputSpec(bool);

    void		createTools();
    void		createHist();
    void		createSurvMap();
    void		clearDisplay();
    void		setButtonStatuses();
    void		displayScanResults();
    void		updateSurvMap();

    void		initWin(CallBacker*);
    void		firstSel(CallBacker*);
    void		typChg(CallBacker*);
    void		inpChg(CallBacker*);
    void		editFile(CallBacker*);
    void		fullScanReq(CallBacker*);
    void		defChg( CallBacker* )		{ execNewScan(true); }
    void		examineCB(CallBacker*);
    void		updateAmplDisplay(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		commit();

};


#endif
