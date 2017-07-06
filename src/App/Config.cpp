#include "Config.hpp"
#include <adbase/Logging.hpp>
#include <adbase/Metrics.hpp>

namespace app {
// {{{ Config::Config()

Config::Config() {
	adbase::metrics::Metrics::buildGauges("config", "watcher.mapsize", 1000, [this](){
		std::lock_guard<std::mutex> lk(_mut);
		return static_cast<uint64_t>(_fileMap.size());
	});
	adbase::metrics::Metrics::buildGauges("config", "queue.size", 1000, [this](){
		return _queue.getSize();
	});
}

// }}}
// {{{ Config::~Config()

Config::~Config() {
}

// }}}
// {{{ void Config::updateFileInfo()

void Config::updateFileInfo(const WatcherFileMap& infos) {
	std::lock_guard<std::mutex> lk(_mut);
    std::vector<std::string> deleteFileInfo;
    for (auto &t : _fileMap) {
        if (infos.find(t.first) == infos.end()) {
            deleteFileInfo.push_back(t.first); 
        }
    }
    for (auto &t : deleteFileInfo) {
        _fileMap.erase(t);
        LOG_INFO << "Delete `" << t << "` config.";
    }

    for (auto &t : infos) {
        _queue.push(t.first);
        _fileMap[t.first] = t.second;
        
    }
}

// }}}
// {{{ void Config::setMoveTime()

void Config::setMoveTime(std::string &filePath, uint32_t time) {
	std::lock_guard<std::mutex> lk(_mut);
	if (_fileMap.find(filePath) == _fileMap.end()) {
		return;
	}

	WatcherFileInfo info = _fileMap[filePath];
	info.moveTime = time;
	_fileMap[filePath] = info;
}

// }}}
// {{{ bool Config::findFile()

bool Config::findFile(std::string &filePath, WatcherFileInfo& fileInfo) {
	std::lock_guard<std::mutex> lk(_mut);
	if (_fileMap.find(filePath) == _fileMap.end()) {
		return false;
	}

	fileInfo = _fileMap[filePath];
	return true;
}

// }}}
// {{{ bool Config::patternPath()

bool Config::patternPath(std::string origFile, std::string &pathFile) {
	std::lock_guard<std::mutex> lk(_mut);
	for (auto &t : _fileMap) {
		if (t.second.deleteTime) {
			continue;
		}

		if (!t.second.isDir) { // 如果监听的是文件，需要全等
			if (origFile == t.first) {
				pathFile = t.first;
				return true;	
			}
		} else {
			if (t.second.isRecursive) {
				if (origFile.find(t.first) != std::string::npos) {
					pathFile = t.first;
					return true;
				}
			} else {
				std::vector<std::string> pathSplit = adbase::explode(origFile, '/', true);
				if (pathSplit.empty()) {
					return false;
				}
				std::string newOriginFile;
				for (uint32_t i = 0; i < static_cast<uint32_t>(pathSplit.size()) - 1; i++) {
					newOriginFile.append("/");
					newOriginFile.append(pathSplit[i]);
				}
                newOriginFile = adbase::trim(newOriginFile, "/");
                
				if (newOriginFile == adbase::trim(t.first, "/")) {
					pathFile = t.first;
					return true;
				}
			}
		}
	}

	return false;
}

// }}}
// {{{ bool Config::isHasWatcherFile() 

bool Config::isHasWatcherFile(std::string & pathFile) {
	std::vector<std::string> pathSplit = adbase::explode(pathFile, '/', true);
	if (pathSplit.empty()) {
		return false;
	}
	std::string newPathFile;
	for (uint32_t i = 0; i < static_cast<uint32_t>(pathSplit.size()) - 1; i++) {
		newPathFile.append("/");
		newPathFile.append(pathSplit[i]);
	}
	std::lock_guard<std::mutex> lk(_mut);
	for (auto &t : _fileMap) {
		if (t.second.isDir) {
			continue;
		}
		std::vector<std::string> secSplits = adbase::explode(t.first, '/', true);
		if (secSplits.empty()) {
			continue;
		}
		std::string newSecFile;
		for (uint32_t i = 0; i < static_cast<uint32_t>(secSplits.size()) - 1; i++) {
			newSecFile.append("/");
			newSecFile.append(pathSplit[i]);
		}

		if (newSecFile == newPathFile) {
			return true;
		}
	}

	return false;
}

// }}}
// {{{ bool Config::consumerConfig() 

bool Config::consumerConfig(WatcherFileInfo& info) {
	std::string pathFile;
	bool ret = _queue.tryPop(pathFile);
	if (!ret) {
		return false;
	}
	std::lock_guard<std::mutex> lk(_mut);
	if (_fileMap.find(pathFile) == _fileMap.end()) {
		return false;	
	}

	info = _fileMap[pathFile];
	return true;
}

// }}}
}
