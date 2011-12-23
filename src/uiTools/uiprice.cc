/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Dec 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprice.cc,v 1.1 2011-12-23 15:23:02 cvskris Exp $";

#include "uiprice.h"

#include "price.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "bufstringset.h"

uiPrice::uiPrice( uiParent* p, const char* label, const Price* price )
    : uiGroup( p )
{
    BufferStringSet currencies;
    Currency::getCurrencyStrings( currencies );
    currencyselfld_ = new uiComboBox( this, currencies, 0 );
    currencyselfld_->setHSzPol( uiObject::SmallVar );

    valuefld_ = new uiLineEdit( this, 0 );
    valuefld_->attach( rightOf, currencyselfld_ );
    valuefld_->setHSzPol( uiObject::SmallVar );

    if ( label )
    {
	uiLabel* thelabel = new uiLabel( this, label );
	thelabel->attach( leftOf, currencyselfld_ );
    }

    setHAlignObj( currencyselfld_ );

    if ( price )
	setPrice( *price );
}


void uiPrice::setPrice( const Price& price )
{
    currencyselfld_->setText( price.currency_->abrevation_ );
    valuefld_->setValue( ((float) price.amount_)/price.currency_->devisor_ );
}


bool uiPrice::getPrice( Price& price ) const
{
    const Currency* currency =
	Currency::getCurrency( currencyselfld_->text() );

    if ( !currency )
	return false;

    price.currency_ = currency;
    if ( mIsUdf(valuefld_->getIntValue()) )
	return false;

    price.amount_ = valuefld_->getIntValue()*currency->devisor_;
    return true;
}
