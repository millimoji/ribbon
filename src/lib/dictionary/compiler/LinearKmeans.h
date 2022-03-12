#pragma once
#ifndef _RIBBON_LINEARKMEANS_H_
#define _RIBBON_LINEARKMEANS_H_
namespace Ribbon { namespace Dictionary {
struct ILinearKmeans
{
	virtual void AddValue(double f) = 0;
	virtual void Calculate() = 0;
	virtual void GetTable(float buffer[256]) const = 0;
	virtual uint8_t GetIndex(double) const = 0;

	virtual void SaveSource(const char16_t* fileName) const = 0;
	virtual void LoadSource(const char16_t* fileName) = 0;
};

FACTORYEXTERN(LinearKmeans);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_LINEARKMEANS_H_
