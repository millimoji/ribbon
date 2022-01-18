@set @temp=0/*
@echo off

:: Check for Mandatory Label\High Mandatory Level 
rem whoami /groups | find "S-1-16-12288" > nul
rem if "%errorlevel%"=="0" ( 
rem     echo Running as elevated user.  Continuing script. 
rem ) else ( 
rem     echo Not running as elevated user. 
rem     echo Relaunching Elevated: "%~dpnx0" %*
rem 
rem     if '%1'=='ELEV' (
rem         shift
rem     ) else (
rem         cscript.exe //e:jscript //nologo "%~f0" "%~0"
rem         exit /B
rem     )
rem )

:: Continue script here

set UNIQUESTR0=%DATE:/=%-%TIME::=%
set UNIQUESTR1=%UNIQUESTR0:~0,-3%
set UNIQUESTR2=%UNIQUESTR1:~4%
set UNIQUESTR=%UNIQUESTR2: =0%

set IMENAME=RibbonIME
set TEMPFOLDER=C:\TEMP
set INSTALLFOLDER=C:\TEMP
rem set BUILTDIR=D:\Git\ribbon\x64\Debug
if "%USERNAME%"=="shinmori" (
	set BUILTDIR=\\shinmori-z4401\d$\git\ribbon\x64\Debug
) else (
	set BUILTDIR=\\washitsu\d\git\ribbon\x64\Debug
)

if not exist %INSTALLFOLDER%\%IMENAME% mkdir %INSTALLFOLDER%\%IMENAME%

start /wait regsvr32 /s /u %INSTALLFOLDER%\%IMENAME%\%IMENAME%.dll
move %INSTALLFOLDER%\%IMENAME%\%IMENAME%.dll %TEMPFOLDER%\%UNIQUESTR%-%IMENAME%.dll
move %INSTALLFOLDER%\%IMENAME%\%IMENAME%.pdb %TEMPFOLDER%\%UNIQUESTR%-%IMENAME%.pdb
copy %BUILTDIR%\%IMENAME%.dll %INSTALLFOLDER%\%IMENAME%\%IMENAME%.dll
copy %BUILTDIR%\%IMENAME%.pdb %INSTALLFOLDER%\%IMENAME%\%IMENAME%.pdb
rem xcopy /D %BUILTDIR%\..\..\xamlapp\config.txt %INSTALLFOLDER%\%IMENAME%
rem xcopy /D %BUILTDIR%\..\..\xamlapp\Ribbon-ja.dic %INSTALLFOLDER%\%IMENAME%
regsvr32 /s %INSTALLFOLDER%\%IMENAME%\%IMENAME%.dll

goto :EOF
*/
var UAC = new ActiveXObject("Shell.Application");
UAC.ShellExecute(WScript.Arguments(0), "ELEV", "", "runas", 1);
