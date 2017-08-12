#ifndef AIDT_READER_HPP_
#define AIDT_READER_HPP_

#include <adbase/Utility.hpp>
#include "AdbaseConfig.hpp"
#include "App/Config.hpp"
#include <cstdio>
#include <fstream>

class App;
namespace app {

typedef struct FdItem {
	std::string pathfile;
	std::string origfile;
	uint64_t offset;
	int lastTime;
	bool moveProtected; // 是否处于保护期，如果处于文件保护状态在gc过程中保留
    std::fstream *fs;
} FdItem;
typedef std::unordered_map<std::string, FdItem> FdMap;

class Message;
class InotifyEvent;
class Reader {
public:
	Reader(std::shared_ptr<app::Config>& watcherConfig, App* app, AdbaseConfig *configure, std::shared_ptr<app::Message>& message);
	~Reader();
	void read(const InotifyEvent& event, bool& isContinue);
	void clear(const InotifyEvent& event);
	void moveself(const InotifyEvent& event);
	void moveto(const InotifyEvent& event);
	void save();
	void load();
	void stop();

private:
	AdbaseConfig *_configure;
    std::shared_ptr<app::Config>& _watcherConfig;
    App *_app;
    std::shared_ptr<app::Message>& _message;
	mutable std::mutex _mut;
	FdMap _fdMap; 
	std::unordered_map<std::string, std::string> _aliasMap;
	int _currentFdNum;
    bool _running;

	void readString(const InotifyEvent& event, const WatcherFileInfo& info, bool& isContinue);
	void readBinary(const InotifyEvent& event, const WatcherFileInfo& info, bool& isContinue);
	bool sendMessage(adbase::Buffer &msg, const WatcherFileInfo& info);
    bool initStream(const InotifyEvent& event, FdItem& item, const WatcherFileInfo& info, bool& isContinue);
	void gcFstreamNonLock(int lifeTime);
	void setFileOffset(std::string &pathFile, uint64_t offset);
    void closeStream(std::fstream* fs);
    std::fstream* openStream(const std::string& file, const WatcherFileInfo& info, std::fstream* stream);

};
}

#endif
