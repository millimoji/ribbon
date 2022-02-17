@rem this script should run with Admin
@rem
@rem pos-id.def needs to be SHIFT-JIS
@rem char.def is required, but it is written in ascii
@rem unk.def use -f option
@rem

setlocal
pushd

set MECABDIR="c:\Program Files (x86)\MeCab"
set MECABDICTDIR=%MECABDIR%\dic\ipadic

copy /y user.csv      %MECABDICTDIR%
copy /y overwrite.csv %MECABDICTDIR%
copy /y unk.def       %MECABDICTDIR%
copy /y all-emoji.csv %MECABDICTDIR%

cd /d  "c:\Program Files (x86)\MeCab\dic\ipadic"

..\..\bin\mecab-dict-index.exe -f utf-8 -t utf-8

popd
endlocal
