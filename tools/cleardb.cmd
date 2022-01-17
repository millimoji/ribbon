set TARGETDBDIR=e:\lmdata

mkdir %TARGETDBDIR%
copy EmptyDB.mdf     %TARGETDBDIR%\WebUrlDB.mdf
copy EmptyDB_log.ldf %TARGETDBDIR%\WebUrlDB_log.ldf
