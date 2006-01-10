#
#---------------------------------------------------------
# Author: de Groot - Bril Earth Sciences ( A.H.Bril )
# Pmake Environment initialization C-shell script
# $Id: PMinit.csh,v 1.7 2006-01-10 14:41:12 cvsbert Exp $
#---------------------------------------------------------

if ( ! $?HDIR ) then
    
    setenv HDIR `uname`
    if ( "$HDIR" == "SunOS" ) then
	setenv HDIR sun5
    else if ( "$HDIR" == "Linux" ) then
	setenv HDIR lux
    else if ( "$HDIR" == "Darwin" ) then
	setenv HDIR mac
    else if ( "$HDIR" == "IRIX" || "$HDIR" == "IRIX64" ) then
	setenv HDIR sgi
    else if ( $?COMSPEC ) then
	setenv HDIR win
    endif

endif

if ( ! $?PLFSUBDIR ) then
    setenv PLFSUBDIR $HDIR
    if ( $HDIR == sun5 ) then
	setenv PLFSUBDIR sol32
    else if ( $HDIR == lux ) then
	if ( `uname -a | grep _64 | wc -l` == 1 ) then
	    setenv PLFSUBDIR lux64
	else
	    setenv PLFSUBDIR lux32
	endif
    endif
endif

if ( ! $?PMAKE ) then
    if ( $?WORK ) then
	setenv PMAKE $WORK/Pmake/base
    else
	setenv PMAKE /d3/dgb/Pmake/base
    endif
endif

if ( ! $?GNUMAKE ) then
    if ( $HDIR == win ) then
	setenv GNUMAKE	"/usr/bin/make"
    else
	setenv GNUMAKE	"gmake"
    endif
endif

setenv PMAKECOMMAND	'$GNUMAKE -I$WORK/Pmake -I$PMAKE'
alias make		$PMAKECOMMAND

alias wdir		'setenv WORK `pwd`'
alias cdw		'cd $WORK'

set _pmake_dbg_dir_=G
if ( $?DEBUG ) then
    if ( $DEBUG == no ) set _pmake_dbg_dir_=O
else
    set _pmake_dbg_dir_=O
endif

alias cdb		'cd $WORK/bin/$PLFSUBDIR/'$_pmake_dbg_dir_
alias cdl		'cd $WORK/lib/$PLFSUBDIR/'$_pmake_dbg_dir_

unset _pmake_dbg_dir_
