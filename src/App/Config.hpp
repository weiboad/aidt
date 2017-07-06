#ifndef AIDT_CONFIG_HPP_
#define AIDT_CONFIG_HPP_

#include <adbase/Utility.hpp>

namespace app {
typedef struct watcherFileInfo {
	std::string pathFile;
	bool isDir;
	bool isBinary;
	bool isRecursive;
	int id;
	int timeWait;
	uint32_t deleteTime;
	uint32_t moveTime;
	std::string topicName;
	std::string format;
	std::vector<std::string> excludes;
} WatcherFileInfo;
typedef std::unordered_map<std::string, WatcherFileInfo> WatcherFileMap;

class Config {
public:
	Config();
	~Config();
	void updateFileInfo(const WatcherFileMap& infos);
	bool findFile(std::string & filePath, WatcherFileInfo& fileInfo);
	bool patternPath(std::string origFile, std::string &pathFile);
	bool isHasWatcherFile(std::string & pathFile);
	bool consumerConfig(WatcherFileInfo& info);
	void setMoveTime(std::string &filePath, uint32_t time);

private:
	WatcherFileMap _fileMap;	
	adbase::Queue<std::string> _queue;
	mutable std::mutex _mut;
};
} 
#endif
