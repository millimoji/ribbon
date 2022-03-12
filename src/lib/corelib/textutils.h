#pragma once
#ifndef _RIBBON_TEXTUTILS_H_
#define _RIBBON_TEXTUTILS_H_
namespace Ribbon {

extern std::string to_utf8(const char16_t* src);
extern std::u16string to_utf16(const char* src);
static inline std::string to_utf8(const std::u16string& src) { return to_utf8(src.c_str()); }
static inline std::u16string to_utf16(const std::string& src) { return to_utf16(src.c_str()); }

extern std::tuple<char16_t*, size_t> convert_utf32_to_utf16_char(char16_t* buf, size_t bufSiz, char32_t src);
extern std::tuple<char16_t*, size_t> convert_utf32_to_utf16_text(char16_t* buf, size_t bufSiz, const char32_t* src);

// u16 text support
template <typename T>
size_t textlen(const T* src)
{
	if (src == nullptr) return 0;
	const T* cur = src;
	for (; *cur != 0; ++cur) ;
	return static_cast<size_t>(cur - src);
}

template <typename T>
int textcmp(const T* lhs, const T* rhs)
{
	if (lhs == rhs) return 0;
	if (lhs == nullptr) return -1;
	if (rhs == nullptr) return 1;
	for (; *lhs == *rhs && *lhs != 0; ++lhs, ++rhs) ;
	return (*lhs == *rhs) ? 0 : (*lhs < *rhs) ? -1 : 1;
}

template <typename T>
bool textstartwith(const T* tar, const T* chk)
{
	if (tar == nullptr) return false;
	for (; *chk != 0 && (*tar == *chk); ++tar, ++chk) ;
	return *chk == 0;
}

template <typename T>
struct textCompare {
	bool operator () (const T* lhs, const T* rhs) const {
		return textcmp(lhs, rhs) < 0;
	}
};

template <typename T>
void textrev(T* src) {
	T* end = src + textlen(src) - 1;
	for (; src < end; ++src, --end) {
		T t = *src; *src = *end; *end = t;
	}
}

inline std::string to_text(int n) {
	char buf[64];
	sprintf(buf, "%d", n);
	return std::string(buf);
}

inline std::string to_text(uint64_t n) {
	char buf[64];
	sprintf(buf, "%lld", n);
	return std::string(buf);
}

inline std::string to_text(double f) {
	char buf[64];
	sprintf(buf, "%f", f);
	return std::string(buf);
}

} // Ribbon
#endif // _RIBBON_TEXTUTILS_H_
