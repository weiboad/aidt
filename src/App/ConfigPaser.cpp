#include "ConfigPaser.hpp"
#include "App/Config.hpp"

namespace app {
// {{{ ConfigPaser::ConfigPaser()

ConfigPaser::ConfigPaser() {
}

// }}}
// {{{ ConfigPaser::~ConfigPaser()

ConfigPaser::~ConfigPaser() {
}

// }}}
// {{{ bool ConfigPaser::paser()

void ConfigPaser::paser(std::string configFile, Config *configObj) {
    check(configFile);

    YAML::Node config;
    config = YAML::LoadFile(configFile);
    YAML::Node transfers = config["transfers"];
    if (transfers.IsSequence()) {
        WatcherFileMap infos;
        for (std::size_t i = 0; i < transfers.size(); i++) {
            YAML::Node item = transfers[i];  
            std::string path = item["path"].as<std::string>();
            path = trimPath(path.c_str());
            if (path.empty()) {
                continue;
            }
            std::vector<std::string> excludes;
            if (item["excludes"]) {
                YAML::Node excludesYml = item["excludes"];
                for (std::size_t j = 0; j < excludes.size(); j++) {
                    excludes.push_back(excludesYml[i].as<std::string>());
                }
            }
            bool isDir = false; 
            if (item["is_dir"]) {
                isDir = item["is_dir"].as<bool>();
            }

            bool isBinary = false; 
            if (item["is_binary"]) {
                isBinary = item["is_binary"].as<bool>();
            }
            bool isRecursive = false; 
            if (item["is_recursive"]) {
                isRecursive = item["is_recursive"].as<bool>();
            }
            uint32_t timeWait = 3;
            if (item["time_wait"]) {
                timeWait = item["time_wait"].as<uint32_t>();
            }
            
            std::string format = "%s";
            if (item["format"]) {
                format = item["format"].as<std::string>();
            }
            std::string topicName = item["topic_name"].as<std::string>();
            WatcherFileInfo info;
            info.isDir     = isDir;
            info.pathFile  = path;
            info.isBinary  = isBinary;
            info.timeWait  = timeWait;
            info.topicName = topicName;
            info.format    = format;
            info.excludes  = excludes;
            info.deleteTime  = 0;
            info.moveTime    = 0;
            info.isRecursive = isRecursive;
            infos[path] = info; 
        }
        configObj->updateFileInfo(infos);
    }
}

// }}}
// {{{ bool ConfigPaser::check()

void ConfigPaser::check(std::string configFile) {
    YAML::Node config;
    try {
        config = YAML::LoadFile(configFile);
    } catch (std::exception& e) {
        LOG_SYSFATAL << "Paser yaml config :" <<  configFile << " fail, err:" <<  e.what();
    } catch (...) {
        LOG_SYSFATAL << "Paser yaml config :" <<  configFile << " fail";
    }

    if (config["transfers"]) {
        YAML::Node transfers = config["transfers"];
        if (!transfers.IsSequence() && !transfers.IsNull()) {
            LOG_SYSFATAL << "`transfers` config item must is array or null";
        }

        if (transfers.IsSequence()) {
            for (std::size_t i = 0; i < transfers.size(); i++) {
                YAML::Node item = transfers[i];  
                if (!item.IsMap()) {
                    LOG_SYSFATAL << "`transfers->item` must is map";
                }
                if (!item["path"]) {
                    LOG_SYSFATAL << "`transfers->item[ " << i << " ]->path` must is define";
                }
                item["path"].as<std::string>();

                if (item["excludes"]) {
                    YAML::Node excludes = item["excludes"];
                    if (!excludes.IsSequence()) {
                        LOG_SYSFATAL << "`transfers->item[ " << i << " ]->excludes` must is array";
                    }
                    for (std::size_t j = 0; j < excludes.size(); j++) {
                        excludes[i].as<std::string>();
                    }
                }
                if (item["is_dir"]) {
                    item["is_dir"].as<bool>();
                }
                if (item["is_binary"]) {
                    item["is_binary"].as<bool>();
                }
                if (item["is_recursive"]) {
                    item["is_recursive"].as<bool>();
                }
                if (item["time_wait"]) {
                    item["time_wait"].as<uint32_t>();
                }
                if (item["format"]) {
                    item["format"].as<std::string>();
                }
                if (!item["topic_name"]) {
                    LOG_SYSFATAL << "`transfers->item[ " << i << " ]->topic_name` must is define";
                }
                item["topic_name"].as<std::string>();
            }
        }
    } else {
        LOG_SYSFATAL << "`transfers` config item must is define, if not file transfer you can set it `~`";
    }
}

// }}}
// {{{ std::string ConfigPaser::trimPath()

std::string ConfigPaser::trimPath(const char *path) {
	std::string pathFile(path);
	std::string::size_type n = pathFile.find_last_not_of("/");
	if (n == std::string::npos) { // 不遍历根目录
		return "";
	}
	return pathFile.substr(0, n + 1);
}

// }}}
}
