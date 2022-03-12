#pragma once
#ifndef _RIBBON_EXCEPTION_H_
#define _RIBBON_EXCEPTION_H_
#include "platformlib.h"

namespace Ribbon {

const uint32_t ERROR_BASE = 0x80000000;

class rbnexception : public std::exception
{
public:
	rbnexception(const char* fileName, int lineNo, const char* message) :
		std::exception(),
		m_message(message),
		m_fileName(fileName),
		m_lineNo(lineNo)
	{}

	std::string m_message;
	std::string m_fileName;
	int m_lineNo;
};

#define THROW_MSG(_m) \
	throw Ribbon::rbnexception(__FILE__, __LINE__, _m)

#define THROW_IF_FALSE(_f) \
	if (!(_f)) { throw Ribbon::rbnexception(__FILE__, __LINE__, #_f); }

#define THROW_IF_FALSE_MSG(_f,_m) \
	if (!(_f)) { throw Ribbon::rbnexception(__FILE__, __LINE__, _m); }

#define THROW_IF_NULL(_p) \
	if ((_p) == nullptr) { throw Ribbon::rbnexception(__FILE__, __LINE__, #_p); }

#define CATCH_LOG() \
	catch (const Ribbon::rbnexception& e) { \
		Platform->Error("rbnexception: %s(%d): %s\n", e.m_fileName.c_str(), e.m_lineNo, e.what()); \
	} catch (const std::exception& e) { \
		Platform->Error("std::Exception: %s\n", e.what()); \
	} catch (...) { \
		Platform->Error("unknown exception\n"); \
	}

} // Ribbon
#endif // _RIBBON_EXCEPTION_H_
