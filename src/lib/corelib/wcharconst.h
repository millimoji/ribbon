// Note: This file should have UTF8 BOM to contain full unicode text
#pragma once
#ifndef _RIBBON_WCHARCONST_H_
#define _RIBBON_WCHARCONST_H_
namespace Ribbon
{

const char TEXT_APPLICATION_NAME_A[] = "RibbonIME";
const char TEXT_SYSTEMCONFIG_FILENAME[] = "config.txt";
const char TEXT_USERCONFIG_FILENAME[] = "userconfig.txt";

const char16_t TEXT_FULL_ASTARISK[] = u"＊";
const char16_t WCHAR_FULL_COMMA = u'，';
const char16_t WCHAR_SMALL_TSU = L'っ';

const char16_t TEXT_POS_JOSHI[] = u"助詞";
const char16_t TEXT_POS_JODOSHI[] = u"助動詞";

const char16_t CHECK_POS_JOSHI[] = u",助詞,";
const char16_t CHECK_POS_JODOSHI[] = u",助動詞,";
const char16_t CHECK_POS_DOSHI_SETSUBI[] = u",動詞,接尾,";

} // Ribbon
#endif // _RIBBON_WCHARCONST_H_
