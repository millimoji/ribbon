@rem this script should run with Admin
setlocal
pushd

set MECABDIR="c:\Program Files (x86)\MeCab"
set MECABDICTDIR=%MECABDIR%\dic\ipadic

copy /y user.csv      %MECABDICTDIR%
copy /y overwrite.csv %MECABDICTDIR%
copy /y unk.def       %MECABDICTDIR%

cd /d  "c:\Program Files (x86)\MeCab\dic\ipadic"

..\..\bin\mecab-dict-index.exe -f SHIFT-JIS -t UTF-8

popd
endlocal
