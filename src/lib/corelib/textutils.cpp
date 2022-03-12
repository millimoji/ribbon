#include "pch.h"

namespace Ribbon {

inline char rshift(char32_t src, int rshift, unsigned char maskbit, unsigned char orbit) {
	return (char)(((src >> rshift) & maskbit) | orbit);
}

inline char32_t lshift(unsigned char src, char32_t maskbit, int lshift) {
	return ((char32_t)src & maskbit) << lshift;
}

#if 0 // reference
char32_t utf8_utf32(const char* src, const char** next)
{
	if (src[0] < 0x80) {
		*next = src + 1;
		return (char32_t)*src;
	}
	if ((src[0] & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80) {
		*next = src + 2;
		return ((char32_t)(src[0] & 0x1F) << 6) + (src[1] & 0x3F);
	}
	if ((src[0] & 0xF0) == 0xE0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80) {
		*next = src + 3;
		return ((char32_t)(src[0] & 0x0F) << 12) + ((char32_t)(src[1] & 0x3F) << 6) + (char32_t)(src[2] & 0x3F);
	}
	if ((src[0] & 0xF8) == 0xF0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80) {
		*next = src + 4;
		return ((char32_t)(src[0] & 0x07) << 18) + ((char32_t)(src[1] & 0x3F) << 12) + ((char32_t)(src[2] & 0x3F) << 6) + (char32_t)(src[3] & 0x3F);
	}
	*next = src + 1;
	return U'?';
}

char32_t utf16_utf32(const char16_t* src, const char16_t** next)
{
	if (src[0] < 0xD800 || src[0] >= 0xE000) {
		*next = src + 1;
		return (char32_t)*src;
	}
	if (src[0] < 0xDC00 && src[1] >= 0xDC00 && src[1] < 0xE000) {
		*next = src + 2;
		return (char32_t)(0x10000 + (src[0] - 0xD800) * 0x400 + (src[1] - 0xDC00));
	}
	*next = src + 1;
	return U'?';
}

void utf32_utf8(char32_t src, std::string& dst)
{
	char buf[8];
	if (src < 0x80) {
		dst += (char)src;
		return;
	}
	if (src < 0x800) {
		buf[0] = rshift(src, 6, 0x1F, 0xC0);
		buf[1] = rshift(src, 0, 0x3F, 0x80);
		buf[2] = 0;
		dst += buf;
		return;
	}
	if (src < 0x1000) {
		buf[0] = rshift(src, 12, 0x0F, 0xE0);
		buf[1] = rshift(src,  6, 0x3F, 0x80);
		buf[2] = rshift(src,  0, 0x3F, 0x80);
		buf[3] = 0;
		dst += buf;
		return;
	}
	if (src < 0x1FFFFF) {
		buf[0] = rshift(src, 18, 0x07, 0xF0);
		buf[1] = rshift(src, 12, 0x3F, 0x80);
		buf[2] = rshift(src,  6, 0x3F, 0x80);
		buf[3] = rshift(src,  0, 0x3F, 0x80);
		buf[4] = 0;
		dst += buf;
		return;
	}
	dst += '?';
}

void utf32_utf16(char32_t src, std::u16string& dst)
{
	if (src < 0x10000) {
		dst += (char16_t)src;
	} else {
		dst += (char16_t)((src - 0x10000) / 0x400 + 0xD800);
		dst += (char16_t)((src - 0x10000) % 0x400 + 0xDC00);
	}
}
#endif

const char* utf8_utf16(const char* _src, std::u16string& dst)
{
	const unsigned char* src = reinterpret_cast<const unsigned char*>(_src);
	if (src[0] < 0x80) {
		dst += (char16_t)src[0];
		++src;
	} else if ((src[0] & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80) {
		dst += (char16_t)(lshift(src[0], 0x1F, 6) + lshift(src[1], 0x3F, 0));
		src += 2;
	} else if ((src[0] & 0xF0) == 0xE0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80) {
		dst += (char16_t)(lshift(src[0], 0x0F, 12) + lshift(src[1], 0x3F, 6) + lshift(src[2], 0x3F, 0));
		src += 3;
	} else if ((src[0] & 0xF8) == 0xF0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80) {
		char32_t utf32 = lshift(src[0], 0x07, 18) + lshift(src[1], 0x3F, 12) + lshift(src[2], 0x3F, 6) + lshift(src[3], 0x3F, 0);
		dst += (char16_t)((utf32 - 0x10000) / 0x400 + 0xD800);
		dst += (char16_t)((utf32 - 0x10000) % 0x400 + 0xDC00);
		src += 4;
	} else {
		dst += U'?';
		++src;
	}
	return reinterpret_cast<const char*>(src);
}

const char16_t* utf16_utf8(const char16_t* src, std::string& dst)
{
	char buf[8];
	if (*src < 0x80) {
		dst += (char)*src;
		return src + 1;
	}
	if (*src < 0x800) {
		buf[0] = rshift(*src, 6, 0x1F, 0xC0);
		buf[1] = rshift(*src, 0, 0x3F, 0x80);
		buf[2] = 0;
		dst += buf;
		return src + 1;
	}
	if (*src < 0xD800 || *src >= 0xE000) {
		buf[0] = rshift(*src, 12, 0x0F, 0xE0);
		buf[1] = rshift(*src,  6, 0x3F, 0x80);
		buf[2] = rshift(*src,  0, 0x3F, 0x80);
		buf[3] = 0;
		dst += buf;
		return src + 1;
	}
	if (src[0] < 0xDC00 && src[1] >= 0xDC00 && src[1] < 0xE000) {
		char32_t utf32 = (char32_t)0x10000 + (src[0] - 0xD800) * 0x400 + (src[1] - 0xDC00);
		buf[0] = rshift(utf32, 18, 0x07, 0xF0);
		buf[1] = rshift(utf32, 12, 0x3F, 0x80);
		buf[2] = rshift(utf32,  6, 0x3F, 0x80);
		buf[3] = rshift(utf32,  0, 0x3F, 0x80);
		buf[4] = 0;
		dst += buf;
		return src + 2;
	}
	dst += '?';
	return src + 1;
}

std::string to_utf8(const char16_t* src)
{
	std::string result;
	for (const char16_t* cur = src; *cur != 0; ) {
		cur = utf16_utf8(cur, result);
	}
	return result;
}

std::u16string to_utf16(const char* src)
{
	std::u16string result;
	for (const char* cur = src; *cur != 0; ) {
		cur = utf8_utf16(cur, result);
	}
	return result;
}

std::tuple<char16_t*, size_t> convert_utf32_to_utf16_char(char16_t* buf, size_t bufSiz, char32_t src)
{
	if (src < 0x10000) {
		THROW_IF_FALSE(bufSiz > 0);
		*buf++ = (char16_t)src;
		bufSiz--;
	} else {
		THROW_IF_FALSE(bufSiz > 1);
		*buf++ = (char16_t)((src - 0x10000) / 0x400 + 0xD800);
		*buf++ = (char16_t)((src - 0x10000) % 0x400 + 0xDC00);
		bufSiz -= 2;
	}
	return std::make_tuple(buf, bufSiz);
}

std::tuple<char16_t*, size_t> convert_utf32_to_utf16_text(char16_t* buf, size_t bufSiz, const char32_t* src)
{
	for (; *src != 0; ++src) {
		std::tie(buf, bufSiz) = convert_utf32_to_utf16_char(buf, bufSiz, *src);
	}
	THROW_IF_FALSE(bufSiz > 0);
	*buf++ = 0;
	--bufSiz;
	return std::make_tuple(buf, bufSiz);
}

} // Ribbonna
