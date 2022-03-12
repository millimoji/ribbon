rd /s /q ..\osandroid\app\src\main\assets\js
rd /s /q ..\osandroid\app\src\main\assets\css

xcopy /Y /e dist\*.html ..\osandroid\app\src\main\assets
xcopy /Y /e dist\*.js   ..\osandroid\app\src\main\assets
xcopy /Y /e dist\*.css  ..\osandroid\app\src\main\assets

xcopy /Y /e dist\*.html ..\xamlapp
xcopy /Y /e dist\*.js   ..\xamlapp
xcopy /Y /e dist\*.css  ..\xamlapp

rem xcopy /Y dist\*.html ..\osios\iOSKeyboardTemplate\Resources
xcopy /Y /e dist\js\*.js   ..\osios\iOSKeyboardTemplate\Resources
xcopy /Y /e dist\css\*.css  ..\osios\iOSKeyboardTemplate\Resources

rem xcopy /Y /e dist\*.html     \\linksta8t\public\trashbox\20180503
rem xcopy /Y /e dist\js\*.js    \\linksta8t\public\trashbox\20180503
rem xcopy /Y /e dist\css\*.css  \\linksta8t\public\trashbox\20180503
