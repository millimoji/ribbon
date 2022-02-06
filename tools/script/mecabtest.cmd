call mkmecab.cmd

set MECAB="c:\Program Files (x86)\MeCab\bin\mecab.exe"

%MECAB% --input-buffer-size=32768 --output=output.txt input.txt

