@if(0)==(0) Echo On

copy /Y ..\config.txt ..\x64\Release

rem start /wait rundll32 ..\x64\Release\RibbonIME.dll,BuildBinaryLMSrc

rem start /wait rundll32 ..\x64\Release\RibbonIME.dll,CreatePhraseList

start /wait rundll32 ..\x64\Release\RibbonIME.dll,CreateSystemDictionary

start /wait rundll32 ..\x64\Release\RibbonIME.dll,DumpSystemDictionary

goto :EOF

rem CScript //NoLogo //E:JScript "%~f0" %*

rem copy Ribbon-ja.zip osandroid\app\src\main\assets\data

exit /b
@end

// JScript to make zip file
var zipfile = "Ribbon-ja.zip";
var srcfile = "Ribbon-ja.dic";

var fso = new ActiveXObject("Scripting.FileSystemObject");
var shell = new ActiveXObject("Shell.Application");

// create empty zip file
var ts = fso.OpenTextFile(zipfile, 2, true);
if (ts == null) {
	WScript.Echo("OpenTextFile() failed");
	WScript.Exit(0);
}
ts.Write(String.fromCharCode(0x50, 0x4B, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
ts.Close();

// copy to zip file
var zip = shell.NameSpace(fso.GetAbsolutePathName(zipfile));
zip.CopyHere(fso.GetAbsolutePathName(srcfile), 4);

WScript.Sleep(1000);

// wait until zip file is readable
while (true) {
	WScript.Sleep(100);
	try {
		fso.OpenTextFile(zipfile, 8, false).Close();
		break;
	} catch (e) {
		// if failed to open file, continue waiting loop
	}
}
