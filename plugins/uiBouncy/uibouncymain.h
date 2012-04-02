#ifndef bouncymain_h
#define bouncymain_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
 * ID       : $Id: uibouncymain.h,v 1.3 2012-04-02 14:38:53 cvskris Exp $
-*/

#include "uidialog.h"

class uiGenInput;

namespace uiBouncy
{
class uiBouncySettingsDlg;

mClass uiBouncyMain : public uiDialog
{
public:

				uiBouncyMain(uiParent*, uiBouncySettingsDlg**); 
				~uiBouncyMain();

    BufferString		getPlayerName() const;
    void			setPlayerName(const BufferString&);

protected:

    uiGenInput*			namefld_;
    uiBouncySettingsDlg**	bsdlg_;

    bool			acceptOK(CallBacker*);

};
};

#endif
