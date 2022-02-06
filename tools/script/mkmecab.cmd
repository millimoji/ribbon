@rem this script should run with Admin
setlocal
pushd

copy /y user.csv "c:\Program Files (x86)\MeCab\dic\ipadic"
copy /y overwrite.csv "c:\Program Files (x86)\MeCab\dic\ipadic"

cd /d  "c:\Program Files (x86)\MeCab\dic\ipadic"

..\..\bin\mecab-dict-index.exe -f SHIFT-JIS -t UTF-8

popd
endlocal
