#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id: uirgbarray.h,v 1.8 2010-08-09 14:53:50 cvskris Exp $
________________________________________________________________________

-*/

#include "odimage.h"

class QImage;


mClass uiRGBArray : public OD::RGBImage
{
public:
                        uiRGBArray(bool withalpha);
			uiRGBArray(const uiRGBArray&);
    virtual		~uiRGBArray();

    char		nrComponents() const { return withalpha_ ? 4 : 3; }
    bool                setSize(int,int);
    int			getSize(bool xdir) const;
    Color		get(int,int) const;
    void		set(int,int,const Color&);
    void		clear(const Color&);

    const QImage&	qImage() const		{ return *qimg_; } ;
    QImage&		qImage()		{ return *qimg_; } ;

protected:

    QImage*		qimg_;
    bool		withalpha_;

};

#endif
