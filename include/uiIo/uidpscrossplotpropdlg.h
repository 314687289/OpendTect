#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.8 2011-07-05 09:44:30 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
class uiDataPointSetCrossPlotter;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSUserDefTab;
class uiDPSCPBackdropTab;
class uiDPSCPDisplayPropTab;
class uiDPSDensPlotSetTab;

		     
mClass uiDataPointSetCrossPlotterPropDlg : public uiTabStackDlg
{
public:
					uiDataPointSetCrossPlotterPropDlg(
					    uiDataPointSetCrossPlotter*);
    uiDataPointSetCrossPlotter&		plotter()	{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&		plotter_;
    uiDPSCPScalingTab*			scaletab_;
    uiDPSCPStatsTab*			statstab_;
    uiDPSUserDefTab*			userdeftab_;
    uiDPSCPDisplayPropTab* 		dispproptab_;
    uiDPSDensPlotSetTab* 		densplottab_;
    uiDPSCPBackdropTab*			bdroptab_;

    void				doApply(CallBacker*);
    bool				acceptOK(CallBacker*);

};

#endif
