#ifndef checksum_h
#define checksum_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2015
 RCS:		$Id: curvature.h 32939 2014-01-07 12:20:06Z bert.bril@dgbes.com $
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"

/*!
Computes the CRC-64 for an array in memory. Algorithm is taken from public
domain code. For very long arrays, code can be called in chunks and putting
int last checksum in the prevsum argument.
*/

mGlobal(Algo) od_uint64 checksum64(const unsigned char*,od_uint64 arrsize,
				   od_uint64 prevsum=0);
mGlobal(Algo) void initChecksum();


#endif
