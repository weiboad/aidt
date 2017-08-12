#ifndef AIDT_WATCHER_HPP_
#define AIDT_WATCHER_HPP_

#include <adbase/Utility.hpp>
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "inotify/inotifytools.h"
#include "inotify/inotify.h"
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wold-style-cast"
#include "AdbaseConfig.hpp"
#include "App/Config.hpp"

namespace app {
typedef enum EventType {
	DELETE=1,
	MOVE_SELF=2,
	MODIFY=3,
	MOVE_TO=4,
	STOP = 999,
} EventType;

typedef struct InotifyEvent {
	std::string pathfile; // 配置监听的文件或目录
	std::string origfile; // 真正的事件文件
	std::string aliasfile; // 移动前的名称
	EventType event;
} InotifyEvent;
// InotifyPaser Cache 管理
typedef struct InotifyPaserCacheItem {
	std::string pathfile;
	int lastTime;
} InotifyPaserCacheItem;
typedef std::unordered_map<std::string, InotifyPaserCacheItem> InotifyPaserCache;

typedef std::function<void (const InotifyEvent& event, bool& isContinue)> ReaderCallbackRead;
typedef std::function<void (const InotifyEvent& event)> ReaderCallbackClear;
typedef std::function<void (const InotifyEvent& event)> ReaderCallbackMoveSelf;
typedef std::function<void (const InotifyEvent& event)> ReaderCallbackMoveTo;

class Watcher {
public:
    Watcher(AdbaseConfig* configure, std::shared_ptr<app::Config>& watcherConfig);
    ~Watcher();
    void start();
    void stop();
    void watcherThread(void *data);
    void eventThread(void *data);
    void createWatcher(std::string pathFile, std::vector<std::string> excludes, bool isDir, bool isRecursive);
    void deleteWatcher(std::string pathFile, bool isDir);
	void setReadHandler(const ReaderCallbackRead& readHandler) {
		_readHandler = readHandler;
	}
	void setClearHandler(const ReaderCallbackClear& clearHandler) {
		_clearHandler = clearHandler;
	}
	void setMoveSelfHandler(const ReaderCallbackMoveSelf& moveSelfHandler) {
		_moveSelfHandler = moveSelfHandler;
	}
	void setMoveToHandler(const ReaderCallbackMoveTo& moveToHandler) {
		_moveToHandler = moveToHandler;
	}
    static void deleteThread(std::thread *t);
private:
    typedef std::unique_ptr<std::thread, decltype(&Watcher::deleteThread)> ThreadPtr;
    typedef std::vector<ThreadPtr> ThreadPool;
    ThreadPool Threads;

    AdbaseConfig *_configure;
    std::shared_ptr<app::Config>& _watcherConfig;
	bool _running;
	InotifyPaserCache _cache;
	adbase::Queue<InotifyEvent> _eventQueue;
	std::string _dirTypeMoveFrom;
	int _watcherNum;
	mutable std::mutex _mut;
    std::condition_variable _dataCond;
    int _eventThreadNumber;
	ReaderCallbackRead _readHandler;
	ReaderCallbackClear _clearHandler;
	ReaderCallbackMoveSelf _moveSelfHandler;
	ReaderCallbackMoveTo _moveToHandler;

    void consumerConfigQueue();
	void paserEvent(struct inotify_event *event);
	bool patternPathFile(std::string &origfile, WatcherFileInfo& info);
	void paserWatcherFile(std::string pathFile);
	void addModifyEvent(std::string pathFile, std::string originFile);
};

}

#endif
