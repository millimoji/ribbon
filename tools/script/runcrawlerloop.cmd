set SCRIPTDIR=%~dp0

set WEBCRAWLEREXE=%SCRIPTDIR%..\WebCrawler\bin\Debug\WebCrawler.exe

:LOOPTOP

git pull

call %SCRIPTDIR%mkmecab.cmd

msbuild.exe %SCRIPTDIR%..\RibbonTools.sln /t:rebuild /p:Configuration=Debug /p:Platform="Any CPU"

%WEBCRAWLEREXE%

@echo ====================== cool down ==============================
timeout /T 60

goto :LOOPTOP
