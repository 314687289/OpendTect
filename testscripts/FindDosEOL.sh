#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# FindDosEOL.sh - finds files with DOS end-of-lines.
#

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` <listfile>"
  exit 1
fi

listfile=$1

if [ ! -f $listfile ];
then
   echo "File $listfile does not exists."
   exit 1
fi

dtectdir=`dirname $0`

if [ -e "${dtectdir}/GetNrProc" ]; then
    nrcpus=`${dtectdir}/GetNrProc`
else
    nrcpus=8
fi

files=`cat $listfile | grep \\.ico -v | xargs -P ${nrcpus} -n 200 grep -l $'\r'`
if [ -z "$files" ];then
   echo "No DOS EOL found!"
else
   echo "DOS EOL found in: "
   echo $files
   exit 1
fi


