#pragma once
#ifndef _RIBBON_TRANSLITERATIONADAPTER_H_
#define _RIBBON_TRANSLITERATIONADAPTER_H_

enum class SymbolEmojiCategory;
struct IEditContext;

namespace Ribbon {

struct ITransliterationAdapter : public ITaskProcessor
{
};

FACTORYEXTERN(TransliterationAdapter);
} // Ribbon
#endif //_RIBBON_TRANSLITERATIONADAPTER_H_
