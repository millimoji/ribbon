set SCRIPTDIR=%~dp0

set WEBCRAWLEREXE=%SCRIPTDIR%..\WebCrawler\bin\Debug\WebCrawler.exe

:LOOPTOP

%WEBCRAWLEREXE%

timeout /T 60

call %SCRIPTDIR%mkmecab.cmd

goto :LOOPTOP
