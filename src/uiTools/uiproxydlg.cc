/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiproxydlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "odnetworkaccess.h"
#include "settings.h"
#include "od_helpids.h"

uiProxyDlg::uiProxyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Connection Settings"),mNoDlgTitle,
    mODHelpKey(mProxyDlgHelpID) ))
{
    useproxyfld_ = new uiGenInput( this, tr("Use proxy"), BoolInpSpec(true) );
    useproxyfld_->valuechanged.notify( mCB(this,uiProxyDlg,useProxyCB) );

    hostfld_ = new uiGenInput( this, tr("HTTP Proxy"), StringInpSpec() );
    hostfld_->attach( alignedBelow, useproxyfld_ );

    portfld_ = new uiLabeledSpinBox( this, tr("Port") );
    portfld_->attach( rightTo, hostfld_ );
    portfld_->box()->setInterval( 1, 65535 );

    authenticationfld_ =
	new uiCheckBox( this, tr("Use authentication") );
    authenticationfld_->activated.notify(
				    mCB(this,uiProxyDlg,useProxyCB) );
    authenticationfld_->attach( alignedBelow, hostfld_ );

    usernamefld_ = new uiGenInput( this, tr("User name"), StringInpSpec() );
    usernamefld_->attach( alignedBelow, authenticationfld_ );
    pwdfld_ = new uiLineEdit( this, "Password" );
    pwdfld_->setToolTip( tr("Password is case sensitive") );
    pwdfld_->attach( alignedBelow, usernamefld_ );
    pwdfld_->setPrefWidthInChar( 23 );
    pwdfld_->setPasswordMode();
    pwdlabel_ = new uiLabel( this, tr("Password") );
    pwdlabel_->attach( leftOf, pwdfld_ );

    initFromSettings();
    useProxyCB(0);
}


uiProxyDlg::~uiProxyDlg()
{}


void uiProxyDlg::initFromSettings()
{
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( Network::sKeyUseProxy(), useproxy );
    useproxyfld_->setValue( useproxy );

    BufferString host;
    setts.get( Network::sKeyProxyHost(), host );
    hostfld_->setText( host );

    int port = 1;
    setts.get( Network::sKeyProxyPort(), port );
    portfld_->box()->setValue( port );

    bool needauth = false;
    setts.getYN( Network::sKeyUseAuthentication(), needauth );
    authenticationfld_->setChecked( needauth );

    BufferString username;
    setts.get( Network::sKeyProxyUserName(), username );
    usernamefld_->setText( username );

    BufferString password;
    if ( setts.get(Network::sKeyCryptProxyPassword(),password) )
    {
	uiString str;
	str.setFromHexEncoded( password );
	password = str.getFullString();
    }
    else
	setts.get( Network::sKeyProxyPassword(), password );

    pwdfld_->setText( password );
}


bool uiProxyDlg::saveInSettings()
{
    Settings& setts = Settings::common();
    const bool useproxy = useproxyfld_->getBoolValue();
    setts.setYN( Network::sKeyUseProxy(), useproxy );

    BufferString host = useproxy ? hostfld_->text() : "";
    setts.set( Network::sKeyProxyHost(), host );

    const int port = useproxy ? portfld_->box()->getIntValue() : 1;
    setts.set( Network::sKeyProxyPort(), port );

    const bool needauth = useproxy ? authenticationfld_->isChecked() : false;
    setts.setYN( Network::sKeyUseAuthentication(), needauth );
    if ( needauth )
    {
	BufferString username = useproxy ? usernamefld_->text() : "";
	setts.set( Network::sKeyProxyUserName(), username );
	BufferString password = useproxy ? pwdfld_->text() : "";
	uiString str = toUiString( password );
	str.getHexEncoded( password );
	setts.removeWithKey( Network::sKeyProxyPassword() );
	setts.set( Network::sKeyCryptProxyPassword(), password );
    }

    return setts.write();
}


void uiProxyDlg::useProxyCB( CallBacker* )
{
    const bool ison = useproxyfld_->getBoolValue();
    hostfld_->setSensitive( ison );
    portfld_->setSensitive( ison );
    authenticationfld_->setSensitive( ison );
    const bool needauth = authenticationfld_->isChecked();
    usernamefld_->setSensitive( ison && needauth );
    pwdfld_->setSensitive( ison && needauth );
    pwdlabel_->setSensitive( ison && needauth );
}


bool uiProxyDlg::acceptOK( CallBacker* )
{
    if ( !saveInSettings() )
    {
	uiMSG().error( tr("Cannot write to settings file") );
	return false;
    }

    Network::setHttpProxyFromSettings();
    return true;
}
