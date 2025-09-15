#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>

class ConfigReader
{
private:
    std::map<std::string, std::string> config;

    // helper function to trim whitespace
    std::string trim(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t");
        size_t end = s.find_last_not_of(" \t");
        if (start == std::string::npos || end == std::string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }

public:
    ConfigReader(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error: could not open config file " << filename << std::endl;
            return;
        }

        std::string line;
        while (getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::string key, value;
            std::stringstream ss(line);
            if (getline(ss, key, '=') && getline(ss, value))
            {
                config[trim(key)] = trim(value);
            }
        }
    }

    // get as string
    std::string getString(const std::string &key, const std::string &def = "")
    {
        return (config.count(key) ? config[key] : def);
    }

    // get as int
    int getInt(const std::string &key, int def = 0)
    {
        return (config.count(key) ? std::stoi(config[key]) : def);
    }

    // get as double
    double getDouble(const std::string &key, double def = 0.0)
    {
        return (config.count(key) ? std::stod(config[key]) : def);
    }

    // get as float
    float getFloat(const std::string &key, float def = 0.0f)
    {
        return (config.count(key) ? std::stof(config[key]) : def);
    }

    // get as vector<int> (comma separated list)
    std::vector<int> getIntVector(const std::string& key) {
        std::vector<int> vec;
        if (!config.count(key)) return vec;

        std::stringstream ss(config[key]);
        std::string item;
        while (getline(ss, item, ',')) {
            item = trim(item);
            if (!item.empty()) vec.push_back(std::stoi(item));
        }
        return vec;
    }
};

#endif
