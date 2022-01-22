SETLOCAL ENABLEEXTENSIONS

@rem you need to set FTPCONNECTION={FTP_SERVER_NAME},{FTP_USER_NAME},{FTP_PASSWORD}

for /f "tokens=1-3 delims=," %%i in ("%FTPCONNECTION%") do (
	set FTP_SERVER_NAME=%%i
	set FTP_USER_NAME=%%j
	set FTP_PASSWORD=%%k
)

echo rem>%TEMP%\ftpcommand.txt
echo open %FTP_SERVER_NAME%>> %TEMP%\ftpcommand.txt
echo %FTP_USER_NAME%>> %TEMP%\ftpcommand.txt
echo %FTP_PASSWORD%>> %TEMP%\ftpcommand.txt
echo bin>> %TEMP%\ftpcommand.txt
echo cd www/lmsummary>> %TEMP%\ftpcommand.txt
echo mkdir %COMPUTERNAME%>> %TEMP%\ftpcommand.txt
echo cd %COMPUTERNAME%>> %TEMP%\ftpcommand.txt
if exist "%~dp0"summary.html (
	echo del summary.html>> %TEMP%\ftpcommand.txt
	echo put "%~dp0"summary.html>> %TEMP%\ftpcommand.txt
)
echo del topicmodel-summary.json>> %TEMP%\ftpcommand.txt
echo put c:\lmworking\topicmodel-summary.json>> %TEMP%\ftpcommand.txt
echo quit>> %TEMP%\ftpcommand.txt

ftp -s:%TEMP%\ftpcommand.txt

ENDLOCAL
