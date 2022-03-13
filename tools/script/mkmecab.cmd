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

copy /y _onlytagging.csv %MECABDICTDIR%
copy /y user.csv      %MECABDICTDIR%
copy /y overwrite.csv %MECABDICTDIR%
copy /y unk.def       %MECABDICTDIR%
copy /y _emoji.csv    %MECABDICTDIR%
copy /y tankanji.csv  %MECABDICTDIR%
copy /y overwrite-auxil.csv %MECABDICTDIR%\auxil.csv
copy /y overwrite-filler.csv %MECABDICTDIR%\filler.csv

cd /d  "c:\Program Files (x86)\MeCab\dic\ipadic"

..\..\bin\mecab-dict-index.exe -f utf-8 -t utf-8

popd
endlocal
