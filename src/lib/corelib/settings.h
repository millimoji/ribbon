#pragma once
#ifndef _RIBBON_CONFIGURATION_H_
#define _RIBBON_CONFIGURATION_H_
namespace Ribbon {

struct ISetting
{
	virtual void SetFilename(const char* fileName, bool isWritable = false) = 0;
	virtual void CascadeReadonlySetting(const std::shared_ptr<ISetting>& setting) = 0;
	virtual bool HasKey(const char* section, const char* key) const = 0;
	virtual const std::string& GetString(const char* section, const char* key) const = 0;
	virtual std::string  GetExpandedString(const char* section, const char* key) const = 0;
	virtual bool SetString(const char* section, const char* key, const char* value) = 0;

	// Aliases
	int GetInt(const char* section, const char* key) const {
		return atoi(GetString(section, key).c_str()); }
	double GetFloat(const char* section, const char* key) const {
		return atof(GetString(section, key).c_str()); }
	bool GetBool(const char* section, const char* key) const {
		return GetInt(section, key) != 0; }

	void SetInt(const char* section, const char* key, int value) {
		SetString(section, key, to_text(value).c_str()); }
	void SetFloat(const char* section, const char* key, double value) {
		SetString(section, key, to_text(value).c_str()); }
	void SetBool(const char* section, const char* key, bool value) {
		SetString(section, key, value ? "1" : "0"); }
};

FACTORYEXTERN2(ISetting, FileSetting);
} // Ribbon
#endif //_RIBBON_CONFIGURATION_H_
