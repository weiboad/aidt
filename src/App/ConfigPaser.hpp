#ifndef AIDT_CONFIG_PASER_HPP_
#define AIDT_CONFIG_PASER_HPP_
#include <adbase/Logging.hpp>
#include "yaml-cpp/yaml.h"

namespace app {
class Config;
class ConfigPaser {
public:
	ConfigPaser();
	~ConfigPaser();
	static void paser(std::string configStr, Config *config);
	static void check(std::string configStr);
	
private:
	static std::string trimPath(const char *pathFile);
};
}

#endif
