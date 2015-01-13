#ifndef uistrings_h
#define uistrings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "fixedstring.h"
#include "uistring.h"

//Common strings. Use these and extend when needed

mClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings);
public:
    static uiString s2D(bool immediate);
    static uiString s3D(bool immediate);
    static uiString sAbort()		{ return tr("Abort"); }
    static uiString sAction()		{ return tr("Action"); }
    static uiString sAdd(bool immediate);
    static uiString sAdvanced()		{ return tr("Advanced"); }
    static uiString sAll()		{ return tr("All"); }
    static uiString sAnalysis()		{ return tr("Analysis"); }
    static uiString sApply()		{ return tr("Apply"); }
    static uiString sApply(bool)	{ return sApply(); }
    static uiString sASCII(bool immediate);
    static uiString sAttribute()	{ return tr("Attribute"); }
    static uiString sAttributes(bool immediate);
    static uiString sBadConnection()	{ 
					  return tr("Internal error: "
						    "bad connection"); 
					}
    static uiString sBottom()		{ return tr("Bottom"); }
    static uiString sCalculate()	{ return tr("Calculate"); }
    static uiString sCancel()		{ return tr("Cancel"); }
    static uiString sCancel(bool)	{ return sCancel(); }
    static uiString sCantCreateHor()	{ return tr("Cannot create horizon"); }
    static uiString sCantFindAttrName()	{ 
					  return tr("Cannot find "
						    "attribute name"); 
					}
    static uiString sCantFindODB()	{
					  return tr("Cannot find object "
						    "in data base");
					}
    static uiString sCantFindSurf()	{ return tr("Cannot find surface"); }
    static uiString sCantLoadHor()	{ return tr("Cannot load horizon"); }
    static uiString sCantOpenInpFile()	{ return tr("Cannot open input file"); }
    static uiString sCantOpenOutpFile() { 
					  return tr("Cannot open "
						    "output file"); 
					}
    static uiString sCantReadInpData()	{ 
					  return tr("Cannot read "
						    "input data");
					}
    static uiString sClose()		{ return tr("Close"); }
    static uiString sColor()		{ return tr("Color"); }
    static uiString sColorTable(bool immediate);
    static uiString sContinue()		{ return tr("Continue"); }
    static uiString sCreate(bool immediate);
    static uiString sCrossline()        { return tr("Crossline"); }
    static uiString sDepth()	        { return tr("Depth"); }
    static uiString sDisplay()		{ return tr("Display"); }
    static uiString sDown()		{ return tr("Down"); }
    static uiString sEdit(bool immediate);
    static uiString sEmptyString()	{ return uiString(""); }
    static uiString sExamine()		{ return tr("Examine"); }
    static uiString sExamine(bool)	{ return sExamine(); }
    static uiString sExport()		{ return tr("Export"); }
    static uiString sFaults(bool immediate);
    static uiString sFile()	        { return tr("File"); }
    static uiString sFileDoesntExist()	{ return tr("File does not exist"); }
    static uiString sGo()	        { return tr("Go"); }
    static uiString sHelp()		{ return tr("Help"); }
    static uiString sHelp(bool)		{ return sHelp(); }
    static uiString sHide()		{ return tr("Hide"); }
    static uiString sHistogram(bool immediate);
    static uiString sHorizon(bool immediate);
    static uiString sHorizons(bool immediate);
    static uiString sHorizontal()	{ return tr("Horizontal"); }
    static uiString sImport()		{ return tr("Import"); }
    static uiString sInline()		{ return tr("Inline"); }
    static uiString sInput()		{ return tr("Input"); }
    static uiString sInputData()	{ return tr("Input Data"); }
    static uiString sLoad(bool immediate);
    static uiString sLock()		{ return tr("Lock"); }
    static uiString sLogs(bool immediate);
    static uiString sManual()		{ return tr("Manual"); }
    static uiString sMarkers(bool immediate);
    static uiString sName()		{ return tr("Name"); }
    static uiString sNew(bool immediate);
    static uiString sNext()		{ return tr("Next >"); }
    static uiString sNo()		{ return tr("No"); }
    static uiString sNone()		{ return tr("None"); }
    static uiString sNoObjStoreSetDB()	{ 
					  return tr("No object to store "
						    "set in data base"); 
					}
    static uiString sOk()		{ return tr("OK"); }
    static uiString sOk(bool)		{ return sOk(); }
    static uiString sOpen(bool immediate);
    static uiString sOperator()		{ return tr("Operator"); }
    static uiString sOptions()		{ return tr("Options"); }
    static uiString sOptions(bool)	{ return sOptions(); }
    static uiString sOutput()           { return tr("Output"); }
    static uiString sOverwrite()        { return tr("Overwrite"); }
    static uiString sPause()            { return tr("Pause"); }
    static uiString sPickSet()		{ return tr("Pickset"); }
    static uiString sPolygon()		{ return tr("Polygon"); }
    static uiString sProcessing()	{ return tr("Processing"); }
    static uiString sProperties(bool immediate);
    static uiString sRectangle()	{ return tr("Rectangle"); }
    static uiString sRedo()		{ return tr("Redo"); }
    static uiString sReload()		{ return tr("Reload"); }
    static uiString sRemove(bool immediate);
    static uiString sSave(bool immediate);
    static uiString sSaveAs(bool immediate);
    static uiString sSaveAsDefault()    { return tr("Save as default"); }
    static uiString sScenes()		{ return tr("Scenes"); }
    static uiString sSeedData()		{
					  return tr("Which one is "
						    "your seed data.");
					}
    static uiString sSEGY()		{ return tr("SEG-Y"); }
    static uiString sSeismics(bool immediate);
    static uiString sSelect(bool immediate);
    static uiString sSettings(bool immediate);
    static uiString sSetup()		{ return tr("Setup"); }
    static uiString sShift(bool immediate);
    static uiString sShow()             { return tr("Show"); }
    static uiString sSteering()		{ return tr("Steering"); }
    static uiString sStep()		{ return tr("Step"); }
    static uiString sStop()		{ return tr("Stop"); }
    static uiString sStored(bool immediate);
    static uiString sStratigraphy(bool immediate);
    static uiString sSurvey()		{ return tr("Survey"); }
    static uiString sTile()		{ return tr("Tile"); }
    static uiString sTime()		{ return tr("Time"); }
    static uiString sTools()		{ return tr("Tools"); }
    static uiString sTrack(bool immediate);
    static uiString sTransparency()     { return tr("Transparency"); }
    static uiString sType()             { return tr("Type"); }
    static uiString sUndo()		{ return tr("Undo"); }
    static uiString sUp()		{ return tr("Up"); }
    static uiString sUse()		{ return tr("Use"); }
    static uiString sUtilities()	{ return tr("Utilities"); }
    static uiString sValue()		{ return tr("Value"); }
    static uiString sVertical()		{ return tr("Vertical"); }
    static uiString sView()		{ return tr("View"); }
    static uiString sWells(bool immediate);
    static uiString sWiggle()		{ return tr("Wiggle"); }
    static uiString sYes()		{ return tr("Yes"); }
};

// Implementations uiStrings

inline uiString uiStrings::s2D( bool immediate )
{ return immediate ? tr("2D") : tr("2D ..."); }

inline uiString uiStrings::s3D( bool immediate )
{ return immediate ? tr("3D") : tr("3D ..."); }

inline uiString uiStrings::sAdd( bool immediate )
{ return immediate ? tr("Add") : tr("Add ..."); }

inline uiString uiStrings::sASCII( bool immediate )
{ return immediate ? tr("ASCII") : tr("ASCII ..."); }

inline uiString uiStrings::sAttributes( bool immediate )
{ return immediate ? tr("Attributes") : tr("Attributes ..."); }

inline uiString uiStrings::sColorTable( bool immediate )
{ return immediate ? tr("ColorTable") : tr("ColorTable ..."); }

inline uiString uiStrings::sCreate( bool immediate )
{ return immediate ? tr("Create") : tr("Create ..."); }

inline uiString uiStrings::sEdit( bool immediate )
{ return immediate ? tr("Edit") : tr("Edit ..."); }

inline uiString uiStrings::sFaults( bool immediate )
{ return immediate ? tr("Faults") : tr("Faults ..."); }

inline uiString uiStrings::sHistogram( bool immediate )
{ return immediate ? tr("Histogram") : tr("Histogram ..."); }

inline uiString uiStrings::sHorizon( bool immediate )
{ return immediate ? tr("Horizon") : tr("Horizon ..."); }

inline uiString uiStrings::sHorizons( bool immediate )
{ return immediate ? tr("Horizons") : tr("Horizons ..."); }

inline uiString uiStrings::sLoad( bool immediate )
{ return immediate ? tr("Load") : tr("Load ..."); }

inline uiString uiStrings::sLogs( bool immediate )
{ return immediate ? tr("Logs") : tr("Logs ..."); }

inline uiString uiStrings::sMarkers( bool immediate )
{ return immediate ? tr("Markers") : tr("Markers ..."); }

inline uiString uiStrings::sNew( bool immediate )
{ return immediate ? tr("New") : tr("New ..."); }

inline uiString uiStrings::sOpen( bool immediate )
{ return immediate ? tr("Open") : tr("Open ..."); }

inline uiString uiStrings::sProperties( bool immediate )
{ return immediate ? tr("Properties") : tr("Properties ..."); }

inline uiString uiStrings::sRemove( bool immediate )
{ return immediate ? tr("Remove") : tr("Remove ..."); }

inline uiString uiStrings::sSave( bool immediate )
{ return immediate ? tr("Save") : tr("Save ..."); }

inline uiString uiStrings::sSaveAs( bool immediate )
{ return immediate ? tr("Save as") : tr("Save as ..."); }

inline uiString uiStrings::sSeismics( bool immediate )
{ return immediate ? tr("Seismics") : tr("Seismics ..."); }

inline uiString uiStrings::sSelect(bool immediate)
{ return immediate ? tr("Select") : tr("Select ..."); }

inline uiString uiStrings::sSettings( bool immediate )
{ return immediate ? tr("Settings") : tr("Settings ..."); }

inline uiString uiStrings::sShift(bool immediate)
{ return immediate ? tr("Shift") : tr("Shift ..."); }

inline uiString uiStrings::sStored( bool immediate )
{ return immediate ? tr("Stored") : tr("Stored ..."); }

inline uiString uiStrings::sStratigraphy( bool immediate )
{ return immediate ? tr("Stratigraphy") : tr("Stratigraphy ..."); }

inline uiString uiStrings::sTrack( bool immediate )
{ return immediate ? tr("Track") : tr("Track ..."); }

inline uiString uiStrings::sWells( bool immediate )
{ return immediate ? tr("Wells") : tr("Wells ..."); }

#endif

