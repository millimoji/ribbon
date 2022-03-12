em++ ^
..\lib\corelib\containers.cpp ^
..\lib\corelib\corelib.cpp ^
..\lib\corelib\refstring.cpp ^
..\lib\corelib\settings.cpp ^
..\lib\corelib\sipkeydef.cpp ^
..\lib\corelib\textutils.cpp ^
..\lib\dictionary\Complementer.cpp ^
..\lib\dictionary\Decoder.cpp ^
..\lib\dictionary\DictionaryReader.cpp ^
..\lib\dictionary\Transliterator.cpp ^
..\lib\external\json11\json11wrap.cpp ^
..\lib\history\HistoryClass.cpp ^
..\lib\history\HistoryManager.cpp ^
..\lib\history\HistoryReuser.cpp ^
..\lib\history\HistoryStore.cpp ^
..\lib\inputmodel\EditFunctions.cpp ^
..\lib\inputmodel\EditLine.cpp ^
..\lib\inputmodel\InputModel.cpp ^
..\lib\inputmodel\ja.LiteralConvert.cpp ^
..\lib\inputmodel\KeyEventProcessor.cpp ^
..\lib\inputmodel\SymEmojiList.cpp ^
..\lib\inputmodel\TransliterationAdapter.cpp ^
oswasm.cpp ^
-I ..\lib ^
-std=c++1z ^
-O2 ^
-s WASM=1 ^
-s ASSERTIONS=1 ^
-s DISABLE_EXCEPTION_CATCHING=0 ^
-s TOTAL_MEMORY=67108864 ^
--preload-file config.txt ^
--preload-file ../Ribbon-ja.dic@Ribbon-ja.dic ^
--bind -o imecore.js
