#pragma once

#include <string>
#include <vector>
#include <map>

// string -> map<string, string>*
typedef std::map<std::string, std::map<std::string, std::string>*> STR_MAP;

typedef STR_MAP::iterator STR_MAP_ITER;

// Singleton pattern
class config_file
{
public:
	~config_file();

	std::string GetString(const std::string& section, const std::string& key, const std::string& default_value="");

	std::vector<std::string> GetStringList(const std::string& section, const std::string& key);

	unsigned GetNumber(const std::string& section, const std::string& key, unsigned default_value=0);

	bool GetBool(const std::string& section, const std::string& key, bool default_value=false);

	float GetFloat(const std::string& section, const std::string& key, const float& default_value);

	static bool setPath(const std::string& path);

	static config_file* instance();

private:
	config_file() {}

	bool isSection(std::string line, std::string& section);
	unsigned parseNumber(const std::string& s);
	std::string trimLeft(const std::string& s);
	std::string trimRight(const std::string& s);
	std::string trim(const std::string& s);
	bool Load(const std::string& path);

	static config_file* config;

	STR_MAP _map;
};


