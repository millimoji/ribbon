set SCRIPTDIR=%~dp0

set WEBCRAWLEREXE=%SCRIPTDIR%..\WebCrawler\bin\Debug\WebCrawler.exe
set POSTPROCESSOREXE=%SCRIPTDIR%..\PostProcessor\bin\Debug\PostProcessor.exe

git pull
msbuild.exe %SCRIPTDIR%..\RibbonTools.sln /t:rebuild /p:Configuration=Debug /p:Platform="Any CPU"
call %SCRIPTDIR%mkmecab.cmd

:LOOPTOP

%WEBCRAWLEREXE%

git pull
msbuild.exe %SCRIPTDIR%..\RibbonTools.sln /t:rebuild /p:Configuration=Debug /p:Platform="Any CPU"
call %SCRIPTDIR%mkmecab.cmd

start %POSTPROCESSOREXE%

@echo ====================== cool down ==============================
timeout /T 60

goto :LOOPTOP
