#ifndef viscolorseq_h
#define viscolorseq_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "callback.h"
#include "visdata.h"

namespace ColTab { class Sequence; }


namespace visBase
{

/*!\brief
ColorSequence describes a basic sequence of colors on a scale ranging from zero
to one.
*/

mExpClass(visBase) ColorSequence : public DataObject
{
public:
    static ColorSequence*	create()
				mCreateDataObj(ColorSequence); 

    void			loadFromStorage(const char*);

    ColTab::Sequence&		colors();
    const ColTab::Sequence&	colors() const;
    void			colorsChanged();
				/*!< If you change the Colortable, notify me
				     with this function */

    Notifier<ColorSequence>	change;

protected:
    virtual			~ColorSequence();

    ColTab::Sequence&		coltabsequence_;
};

}; // Namespace


#endif

