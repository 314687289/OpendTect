@echo off
::
:: OpenTect prepare remote script. 
:: This script is used for windows remote batch processing
:: in order to
::
::  1. Set password in order to be able to use rcmd (from master to client)
::  2. Mount the data share on the client before starting processing
::
:: In case of option 1, datadrive and datashare are set to _none_
:: The values of datahost, datadrive, datashare and datapass are 
:: set in the BatchHosts file and are basically just strings.
:: 
::
:: $Id: od_prepare.bat,v 1.1 2004-10-25 14:40:50 dgb Exp $
::______________________________________________________________________________


set datahost=%1
set datadrive=%2
set datashare=%3
set username=%4
set userpass=%5


if "%DTECT_DEBUG%No" == "No" goto setcred
echo net use "\\%datahost%" . "/USER:%datahost%\%username%"

:setcred
net use "\\%datahost%" %userpass% "/USER:%datahost%\%username%" > nul 2>&1

if  "%datadrive%" == "_none_" goto end 
if  "%datashare%" == "_none_" goto end 

if "%DTECT_DEBUG%No" == "No" goto mount
echo net use %datadrive%: "\\%dathost%\%datashare%"

:mount
net use %datadrive%: "\\%datahost%\%datashare%" > nul 2>&1

:end

