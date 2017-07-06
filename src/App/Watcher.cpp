#include "Watcher.hpp"
#include <adbase/Logging.hpp>

namespace app {
// {{{ Watcher::Watcher()

Watcher::Watcher(AdbaseConfig* config, std::shared_ptr<app::Config>& watcherConfig) :
    _configure(config),
    _watcherConfig(watcherConfig),
    _running(true) {
}

// }}}
// {{{ void Watcher::start()

void Watcher::start() {
    // 初始化 inotify 实例
    if (!inotifytools_initialize()) {
        LOG_ERROR << strerror(inotifytools_error());
    }

	adbase::metrics::Metrics::buildGauges("watcher", "inotify.size", 1000, [this](){
		std::lock_guard<std::mutex> lk(_mut);
		return _watcherNum;
	});
	adbase::metrics::Metrics::buildGauges("watcher", "inotify.event.size", 1000, [this](){
		return _eventQueue.getSize();
	});

    ThreadPtr watcherThread(new std::thread(std::bind(&Watcher::watcherThread, this, std::placeholders::_1), nullptr), &Watcher::deleteThread);
    LOG_DEBUG << "Create inotify watch thread success";
    Threads.push_back(std::move(watcherThread));
    ThreadPtr eventThread(new std::thread(std::bind(&Watcher::eventThread, this, std::placeholders::_1), nullptr), &Watcher::deleteThread);
    Threads.push_back(std::move(eventThread));
}

// }}}
// {{{ void Watcher::stop()

void Watcher::stop() {
	_running = false;
	InotifyEvent eventItem;		
	eventItem.event = STOP;
	_eventQueue.push(eventItem);
    LOG_DEBUG << "Wacher event stopping";
}

// }}}
// {{{ void Watcher::watcherThread()

void Watcher::watcherThread(void *data) {
    (void)data;

    struct inotify_event* event;
    while (_running) {
        consumerConfigQueue();
		event = inotifytools_next_event(1);
		if (!event) {
			continue;
		}
		
		paserEvent(event);
    }
    LOG_DEBUG << "Watcher watcher thread stop.";
}

// }}}
// {{{ void Watcher::eventThread()

void Watcher::eventThread(void *) {
    while (true) {
		InotifyEvent item;
		_eventQueue.waitPop(item);
		if (item.event == DELETE) { // 删除文件
			_clearHandler(item);
			LOG_DEBUG << "pop event delete, pathfile: " << item.pathfile << ", origfile: " << item.origfile;
		} else if (item.event == MOVE_SELF) {
			item.event = MODIFY;
			_eventQueue.push(item);
			_moveSelfHandler(item);
			LOG_TRACE << "pop event move, pathfile: " << item.pathfile
					  << ", origfile: " << item.origfile;
		} else if (item.event == MODIFY) {
			bool isContinue = false;
			LOG_TRACE << "pop event modify, pathfile: " << item.pathfile
					  << ", origfile: " << item.origfile;
			_readHandler(item, isContinue);
			if (isContinue) {
				_eventQueue.push(item);
			}
		} else if (item.event == MOVE_TO) {
			LOG_TRACE << "pop event move to, pathfile: " << item.pathfile
					  << ", origfile: " << item.origfile;
			item.event = MODIFY;
			_eventQueue.push(item);
			_moveToHandler(item);
		} else {
            break; 
        }
    }
    LOG_DEBUG << "Watcher event thread stop.";
}

// }}}
// {{{ void Watcher::consumerConfigQueue()

void Watcher::consumerConfigQueue() {
    WatcherFileInfo info;
    bool ret = _watcherConfig->consumerConfig(info);
    if (!ret) {
        return;
    }

    LOG_INFO << "Start process pathfile `" << info.pathFile << "` config";
    createWatcher(info.pathFile, info.excludes, info.isDir, info.isRecursive);
}

// }}}
// {{{ void Watcher::createWatcher()

void Watcher::createWatcher(std::string pathFile, std::vector<std::string> excludes, bool isDir, bool isRecursive) {
    if (!isDir) { // 监测单个文件
        struct stat fileStat;
        if (stat(pathFile.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
            LOG_ERROR << "Create Watcher fail, pathfile `" << pathFile << "` is not file or not exists.";
            return;
        }
        std::vector<std::string> pathSplit = adbase::explode(pathFile, '/', true);
        if (pathSplit.empty()) {
            LOG_ERROR << "Create Watcher fail, pathFile is invalid.";
            return;
        }
        std::string newPathFile;
        for (int i = 0; i < static_cast<int>(pathSplit.size()) - 1; i++) {
            newPathFile.append("/");
            newPathFile.append(pathSplit[i]);
        }
        if(!inotifytools_watch_file(newPathFile.c_str(), IN_ALL_EVENTS)) {
            LOG_ERROR << "Create Watcher fail, pathFile: " << pathFile
                      << ", dirPath: " << newPathFile
                      << ", " << strerror(inotifytools_error());
        }
        LOG_INFO << "Create Watcher file success, path: " << newPathFile << " watches:" << inotifytools_get_num_watches();
        paserWatcherFile(pathFile);
		{
			std::lock_guard<std::mutex> lk(_mut);
			_watcherNum = inotifytools_get_num_watches();
		}
        return;
    }

    struct stat fileStat;
    if (stat(pathFile.c_str(), &fileStat) != 0 || !S_ISDIR(fileStat.st_mode)) {
        LOG_WARN << "pathfile `" << pathFile << "` is not dir or not exists, creating..";
		if (!adbase::mkdirRecursive(pathFile, 0777)) {
			LOG_ERROR << "pathfile `" << pathFile << "` create dir fail";
			return;
		}
    }

    if (isRecursive) {
        if (excludes.empty()) {
            if(!inotifytools_watch_recursively(pathFile.c_str(), IN_ALL_EVENTS)) {
                LOG_ERROR << "Create Watcher fail, " << strerror(inotifytools_error());
            }
            LOG_INFO << "Create Watcher recursive dir success, path: " << pathFile << " watches:" << inotifytools_get_num_watches();
        } else {
            uint32_t excludesSize = static_cast<uint32_t>(excludes.size());
            char *p[excludesSize + 1];
            for (uint32_t i = 0; i < static_cast<uint32_t>(excludes.size()); i++) {
                uint32_t excludeLen = static_cast<uint32_t>(excludes[i].size());
                *(p + i) = new char[excludeLen + 1];
                memcpy(*(p + i), excludes[i].c_str(), excludeLen);
                *(*(p + i) + excludeLen) = '\0';
                LOG_INFO << "Excludes: " << excludes[i];
            }
            *(p + excludesSize) = nullptr;

            if(!inotifytools_watch_recursively_with_exclude(pathFile.c_str(), IN_ALL_EVENTS, const_cast<const char**>(p))) {
                LOG_ERROR << "Create Watcher fail, " << strerror(inotifytools_error());
            }

            for (uint32_t i = 0; i < static_cast<uint32_t>(excludesSize); i++) {
                delete [] *(p + i);
            }
            LOG_INFO << "Create Watcher recursive dir and excluses success, path: " << pathFile << " watches:" << inotifytools_get_num_watches();
        }
    } else {
        if(!inotifytools_watch_file(pathFile.c_str(), IN_ALL_EVENTS)) {
            LOG_ERROR << "Create Watcher fail, " << strerror(inotifytools_error());
        }
        LOG_INFO << "Create Watcher dir not recursive success, path: " << pathFile << " watches:" << inotifytools_get_num_watches();
    }
    paserWatcherFile(pathFile);
	{
		std::lock_guard<std::mutex> lk(_mut);
		_watcherNum = inotifytools_get_num_watches();
	}
}

// }}}
// {{{ void Watcher::deleteWatcher()

void Watcher::deleteWatcher(std::string pathFile, bool isDir) {
    if (isDir) { // 监测单个文件
        return;
    }
    struct stat fileStat;
    if (stat(pathFile.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        LOG_ERROR << "Delete Watcher fail, pathfile `" << pathFile << "` is not file or not exists.";
        return;
    }

    if (_watcherConfig->isHasWatcherFile(pathFile)) {
        LOG_INFO << "Have an other file watcher in this dir, not need remove watcher. pathFile: " << pathFile;
    } else {
        std::vector<std::string> pathSplit = adbase::explode(pathFile, '/', true);
        if (pathSplit.empty()) {
            LOG_ERROR << "Remove Watcher fail, pathFile is invalid.";
            return;
        }
        std::string newPathFile;
        for (uint32_t i = 0; i < static_cast<uint32_t>(pathSplit.size()) - 1; i++) {
            newPathFile.append("/");
            newPathFile.append(pathSplit[i]);
        }
        if(!inotifytools_remove_watch_by_filename(newPathFile.c_str())) {
            LOG_ERROR << "Delete Watcher fail, pathFile: " << pathFile << ", dirPath: "
                      << newPathFile << ", " << strerror(inotifytools_error());
        }
    }
	{
		std::lock_guard<std::mutex> lk(_mut);
		_watcherNum = inotifytools_get_num_watches();
	}
    LOG_INFO << "Remove Watcher file success, path: " << pathFile;
    return;
}

// }}}
// {{{ bool Watcher::patternPathFile()

bool Watcher::patternPathFile(std::string &originFile, WatcherFileInfo &info) {
	std::string pathFile;
	// 先查看缓存
	if (static_cast<uint32_t>(_cache.size()) > 1024) {
		int currentTime = static_cast<uint32_t>(time(nullptr));
		std::vector<std::string> keys;
		for (auto &t : _cache) {
			if (currentTime - t.second.lastTime > 1) {
				keys.push_back(t.first);
			}
		}
		for (auto &t : keys) {
			_cache.erase(t);
		}
	}

	if (_cache.find(originFile) == _cache.end()) {	
		bool ret = _watcherConfig->patternPath(originFile, pathFile);
		if (!ret) {
			LOG_DEBUG << "pattern originFile: " << originFile << " pathFile: " << pathFile << " fail.";
		}
		InotifyPaserCacheItem item;
		item.pathfile = pathFile;
		item.lastTime = static_cast<uint32_t>(time(nullptr));
		_cache[originFile] = item;
	} else {
		pathFile = _cache[originFile].pathfile;
		_cache[originFile].lastTime = static_cast<uint32_t>(time(nullptr));
	}

	bool ret = _watcherConfig->findFile(pathFile, info);
	return ret;
}

// }}}
// {{{ void Watcher::paserEvent()

void Watcher::paserEvent(struct inotify_event *event) {
	std::string origFile(inotifytools_filename_from_wd(event->wd));
	origFile.append(event->name);
	WatcherFileInfo info;
	if (!patternPathFile(origFile, info)) {
		return;
	}

	// 新增监控节点
	if (event->mask & IN_CREATE) {
		if (!info.isDir || !info.isRecursive) {
			return;
		}
		struct stat fileStat;
		if (stat(origFile.c_str(), &fileStat) != 0 || !S_ISDIR(fileStat.st_mode)) {
			return;
		}
		std::vector<std::string> excludes;
		createWatcher(origFile, excludes, true, true);
		LOG_INFO << "Push Event DIR CREATE `" << origFile << "` pathFile: " << info.pathFile;
	}

	if (event->mask & IN_DELETE || event->mask & IN_DELETE_SELF) { // 删除事件，用来清除打开的文件
		InotifyEvent eventItem;		
		eventItem.pathfile = info.pathFile;
		eventItem.origfile = origFile;
		eventItem.aliasfile = "";
		eventItem.event = DELETE;
		_eventQueue.push(eventItem);
		LOG_INFO << "Push Event DELETE `" << origFile << "` pathFile: "
				 << eventItem.pathfile;
		return;
	}

	if (event->mask & IN_MODIFY) { // 文件修改事件，通知读取
		InotifyEvent eventItem;		
		eventItem.pathfile = info.pathFile;
		eventItem.origfile = origFile;
		eventItem.aliasfile = "";
		eventItem.event = MODIFY;
		_eventQueue.push(eventItem);
		LOG_TRACE << "Push Event MODIFY `" << origFile << "` pathFile: " << eventItem.pathfile;
		return;
	}

	if (event->mask & IN_MOVE_SELF || event->mask & IN_MOVED_FROM) { // 文件类型的监控生效
		if (info.isDir) {
			LOG_INFO << "Dir type file Event MOVE_SLEF `" << origFile << "` pathFile: " << info.pathFile;
			_dirTypeMoveFrom = origFile;
			return;
		}
		
		// 处理文件
		InotifyEvent eventItem;		
		eventItem.pathfile = info.pathFile;
		eventItem.origfile = origFile;
		eventItem.aliasfile = "";
		eventItem.event = MOVE_SELF;
		_eventQueue.push(eventItem);
		LOG_INFO << "Push Event MOVE_SLEF `" << origFile << "` pathFile: " << eventItem.pathfile;
		return;
	}

	if (event->mask & IN_MOVED_TO) {
		if (info.isDir) {
			LOG_INFO << "Dir type file Event MOVE_TO `" << origFile << "` pathFile: " << info.pathFile;
			if (_dirTypeMoveFrom != "") {
				InotifyEvent eventItem;		
				eventItem.pathfile = info.pathFile;
				eventItem.origfile = origFile;
				eventItem.aliasfile = _dirTypeMoveFrom;
				eventItem.event = MOVE_TO;
				_eventQueue.push(eventItem);
				LOG_INFO << "Push dir type Event MOVE_TO `" << origFile << "` alias `"
						 << _dirTypeMoveFrom << "` pathFile: " << eventItem.pathfile;
			}
			_dirTypeMoveFrom = "";
		}
	}
}

// }}}
// {{{ void Watcher::paserWatcherFile()

void Watcher::paserWatcherFile(std::string pathFile) {
	WatcherFileInfo info;
	bool ret = _watcherConfig->findFile(pathFile, info);	
	if (!ret) {
		return;
	}

	if (!info.isDir) {
		addModifyEvent(pathFile, pathFile);
	} else {
		std::vector<std::string> originFiles;
		if (!info.isRecursive) {
			adbase::recursiveDir(pathFile, false, info.excludes, originFiles);	
		} else {
			adbase::recursiveDir(pathFile, true, info.excludes, originFiles);	
		}

		for (auto &t : originFiles) {
			addModifyEvent(pathFile, t);
		}
	}
}

// }}}
// {{{ void Watcher::addModifyEvent()

void Watcher::addModifyEvent(std::string pathFile, std::string originFile) {
	struct stat fileStat;
	if (0 != stat(originFile.c_str(), &fileStat) || !S_ISREG(fileStat.st_mode)) {
		return;	
	}
	InotifyEvent eventItem;		
	eventItem.pathfile = pathFile;
	eventItem.origfile = originFile;
	eventItem.event = MODIFY;
	_eventQueue.push(eventItem);
	LOG_INFO << "Restart Event MODIFY `" << originFile << "` pathFile: " << eventItem.pathfile;
}

// }}}}
// {{{ void Watcher::deleteThread()

void Watcher::deleteThread(std::thread *t) {
    t->join();
    delete t;
}

// }}}
// {{{ Watcher::~Watcher()

Watcher::~Watcher() {
}

// }}}
}
