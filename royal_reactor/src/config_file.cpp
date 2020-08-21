#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <string.h>

#include "config_file.h"

config_file* config_file::config = nullptr;

//dtor
config_file::~config_file()
{
	for (STR_MAP_ITER it = _map.begin(); it != _map.end(); ++it)
		delete it->second;
}

std::string config_file::GetString(const std::string& section, const std::string& key, const std::string& default_value)
{
	STR_MAP_ITER it = _map.find(section);
	if (it != _map.end())
	{
		std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
		if (it1 != it->second->end())
			return it1->second;
	}
	return default_value;
}

float config_file::GetFloat(const std::string& section, const std::string& key, const float& default_value)
{
	std::ostringstream convert1;
	convert1 << default_value;
	
	// float -> string, then turn to GetString
	std::string default_value_str = convert1.str();
	std::string text = GetString(section, key, default_value_str);

	std::istringstream convert2(text);
	float fresult;
	if (!(convert2 >> fresult))
		fresult = 0;
	return fresult;
}

bool config_file::Load(const std::string& path)
{
	std::ifstream ifs(path.c_str()); // c_str: string -> C style string


	if (!ifs.good()) 	//stream normal ?
		return false;

	std::string line;
	std::map<std::string, std::string>* m = nullptr;

	while (!ifs.eof() && ifs.good())  //reach end? and normal?
	{	
		getline(ifs, line);		//read a line
		std::string section;

		if (isSection(line, section))
		{
			STR_MAP_ITER it = _map.find(section);
			if (it == _map.end())
			{
				m = new std::map<std::string, std::string>();
				_map.insert(STR_MAP::value_type(section, m));
			}
			else
			{
				m = it->second;
			}
			continue;
		}

		size_t equ_pos = line.find('=');	//find the '=' first occurrence postion
		if (equ_pos == std::string::npos)	//end?
			continue;

		std::string key = line.substr(0, equ_pos);
		std::string value = line.substr(equ_pos + 1);

		key = trim(key);	//remove blank
		value = trim(value);

		if (key.empty())
			continue;
		if (key[0] == '#' || key[0] == ';')	 //skip comment
			continue;

		std::map<std::string, std::string>::iterator it1 = m->find(key);
		if (it1 != m->end())
			it1->second = value;
		else
			m->insert(std::map<std::string, std::string>::value_type(key, value));

	}
	ifs.close();
	return true;
}

std::vector<std::string> config_file::GetStringList(const std::string& section, const std::string& key)
{
	std::vector<std::string> v;
	std::string str = GetString(section, key, "");
	std::string sep = ", \t";
	std::string substr;
	std::string::size_type start = 0;
	std::string::size_type index;

	while ((index = str.find_first_of(sep, start)) != std::string::npos)
	{
		substr = str.substr(start, index - start);
		v.push_back(substr);

		start = str.find_first_not_of(sep, index);
		if (start == std::string::npos)
			return v;
	}

	substr = str.substr(start);
	v.push_back(substr);
	return v;
}

unsigned config_file::GetNumber(const std::string& section, const std::string& key, unsigned default_value)
{
	STR_MAP_ITER it = _map.find(section);
	if (it != _map.end())
	{
		std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
		if (it1 != it->second->end())
			return parseNumber(it1->second);
	}
	return default_value;
}

bool config_file::GetBool(const std::string& section, const std::string& key, bool default_value)
{
	STR_MAP_ITER it = _map.find(section);
	if (it != _map.end())
	{
		std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
		if (it1 != it->second->end())
		{
			if (strcasecmp(it1->second.c_str(), "true") == 0)
				return true;
		}
	}
	return default_value;
}

bool config_file::isSection(std::string line, std::string& section)
{
	section = trim(line);

	if (section.empty() || section.length() <= 2)
		return false;
	if (section.at(0) != '[' || section.at(section.length()-1) != ']')
		return false;

	section = section.substr(1, section.length() - 2);
	section = trim(section);

	return true;
}

unsigned config_file::parseNumber(const std::string& s)
{
	std::istringstream iss(s);
	long long v = 0;
	iss >> v;
	return v;
}

std::string config_file::trimLeft(const std::string& s)
{
	size_t first_not_empty = 0;

	std::string::const_iterator beg = s.begin();

	while (beg != s.end())
	{
		if (!isspace(*beg))
			break;
		++first_not_empty;
		++beg;
	}

	return s.substr(first_not_empty);
}

std::string config_file::trimRight(const std::string& s)
{
	size_t last_not_empty = s.length();
	std::string::const_iterator end = s.end();
	while (end != s.begin())
	{
		--end;
		if (!isspace(*end))
			break;
		--last_not_empty;
	}

	return s.substr(0, last_not_empty);
}

std::string config_file::trim(const std::string& s)
{
	return trimLeft(trimRight(s));
}

config_file* config_file::instance()
{
	assert(config != nullptr);
	return config;
}

bool config_file::setPath(const std::string& path)
{
	assert(config == nullptr);

	config = new config_file();

	return config->Load(path);
}
